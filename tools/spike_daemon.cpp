/**
 * @file spike_daemon.cpp
 * @brief Persistent Spike golden model daemon for differential testing
 * 
 * This daemon spawns Spike once per test case but maintains a persistent
 * socket server to avoid process overhead. Communicates via Unix socket.
 * 
 * Protocol:
 *   LOAD <len>\n<binary>       - Load program binary and run Spike
 *   QUIT\n                     - Shutdown daemon
 * 
 * Responses:
 *   OK\n                       - Command succeeded  
 *   COMMIT <pc> <insn> <rd> <wdata>\n  - Instruction commit
 *   DONE <exit_code>\n         - Execution finished
 *   ERROR <message>\n          - Command failed
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>

static volatile sig_atomic_t g_shutdown = 0;
static void sig_handler(int) { g_shutdown = 1; }

std::string build_elf(const std::vector<uint8_t>& binary) {
    // Create temp files for binary -> ELF conversion
    char tmpbin[] = "/tmp/spike_daemon_bin_XXXXXX";
    int fd = mkstemp(tmpbin);
    if (fd < 0) return "";
    
    write(fd, binary.data(), binary.size());
    close(fd);
    
    std::string tmpobj = std::string(tmpbin) + ".o";
    std::string tmpelf = std::string(tmpbin) + ".elf";
    
    // Use objcopy to convert binary to object
    std::string cmd = "/opt/riscv/bin/riscv32-unknown-elf-objcopy -I binary -O elf32-littleriscv -B riscv:rv32 ";
    cmd += tmpbin;
    cmd += " ";
    cmd += tmpobj;
    cmd += " 2>/dev/null";
    
    if (system(cmd.c_str()) != 0) {
        unlink(tmpbin);
        return "";
    }
    
    // Link object to create ELF
    cmd = "/opt/riscv/bin/riscv32-unknown-elf-ld -T /home/robin/HAVEN/Fuzz/tools/link.ld ";
    cmd += tmpobj;
    cmd += " -o ";
    cmd += tmpelf;
    cmd += " 2>/dev/null";
    
    if (system(cmd.c_str()) != 0) {
        unlink(tmpbin);
        unlink(tmpobj.c_str());
        return "";
    }
    
    unlink(tmpbin);
    unlink(tmpobj.c_str());
    
    return tmpelf;
}

class SpikeDaemon {
public:
    SpikeDaemon(const std::string& socket_path, const std::string& spike_bin, const std::string& isa)
        : socket_path_(socket_path), spike_bin_(spike_bin), isa_(isa), 
          server_fd_(-1), client_fd_(-1) {
    }
    
    ~SpikeDaemon() {
        cleanup();
        if (sim_) delete sim_;
    }
    
    bool start() {
        // Create Unix domain socket
        server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "[DAEMON] Failed to create socket: " << strerror(errno) << std::endl;
            return false;
        }
        
        // Remove stale socket file
        unlink(socket_path_.c_str());
        
        // Bind to socket path
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);
        
        if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "[DAEMON] Failed to bind socket: " << strerror(errno) << std::endl;
            return false;
        }
        
        // Listen for connections
        if (listen(server_fd_, 1) < 0) {
            std::cerr << "[DAEMON] Failed to listen: " << strerror(errno) << std::endl;
            return false;
        }
        
        std::cerr << "[DAEMON] Listening on " << socket_path_ << std::endl;
        return true;
    }
    
    void run() {
        while (!g_shutdown) {
            // Accept client connection
            client_fd_ = accept(server_fd_, nullptr, nullptr);
            if (client_fd_ < 0) {
                if (errno == EINTR) continue;
                std::cerr << "[DAEMON] Accept failed: " << strerror(errno) << std::endl;
                break;
            }
            
            std::cerr << "[DAEMON] Client connected" << std::endl;
            
            // Handle client session
            handle_client();
            
            close(client_fd_);
            client_fd_ = -1;
            std::cerr << "[DAEMON] Client disconnected" << std::endl;
        }
    }
    
private:
    void handle_client() {
        std::string line_buffer;
        char buf[4096];
        
        while (!g_shutdown) {
            // Read command line
            ssize_t n = read(client_fd_, buf, sizeof(buf) - 1);
            if (n <= 0) break;
            
            buf[n] = '\0';
            line_buffer += buf;
            
            // Process complete lines
            size_t pos;
            while ((pos = line_buffer.find('\n')) != std::string::npos) {
                std::string cmd = line_buffer.substr(0, pos);
                line_buffer.erase(0, pos + 1);
                
                if (!handle_command(cmd)) {
                    return;  // Client requested quit or error
                }
            }
        }
    }
    
    bool handle_command(const std::string& cmd) {
        std::istringstream iss(cmd);
        std::string verb;
        iss >> verb;
        
        if (verb == "QUIT") {
            send_response("OK\n");
            return false;
        }
        else if (verb == "RESET") {
            return cmd_reset();
        }
        else if (verb == "LOAD") {
            size_t len;
            iss >> len;
            return cmd_load(len);
        }
        else if (verb == "RUN") {
            size_t max_insns;
            iss >> max_insns;
            return cmd_run(max_insns);
        }
        else {
            send_response("ERROR Unknown command\n");
            return true;
        }
    }
    
    bool cmd_reset() {
        // Reset Spike processor state
        proc_->reset();
        send_response("OK\n");
        return true;
    }
    
    bool cmd_load(size_t len) {
        // Read binary data
        std::vector<uint8_t> data(len);
        size_t total = 0;
        
        while (total < len) {
            ssize_t n = read(client_fd_, data.data() + total, len - total);
            if (n <= 0) {
                send_response("ERROR Failed to read binary data\n");
                return false;
            }
            total += n;
        }
        
        // Load into Spike memory at 0x80000000
        try {
            mmu_t* mmu = proc_->get_mmu();
            for (size_t i = 0; i < len; ++i) {
                mmu->store_uint8(0x80000000 + i, data[i]);
            }
            
            // Set PC to start address
            proc_->get_state()->pc = 0x80000000;
            
            send_response("OK\n");
            return true;
        } catch (const std::exception& e) {
            std::string msg = std::string("ERROR Load failed: ") + e.what() + "\n";
            send_response(msg);
            return false;
        }
    }
    
    bool cmd_run(size_t max_insns) {
        // Execute instructions and stream commits
        size_t count = 0;
        
        try {
            while (count < max_insns) {
                // Single-step Spike
                proc_->step(1);
                
                // Get commit info from processor state
                state_t* state = proc_->get_state();
                uint32_t pc = state->pc;
                
                // Get last instruction (this is simplified - real Spike API may differ)
                // For now, send basic commit format
                // Format: COMMIT <pc> <insn> <rd> <wdata>
                std::ostringstream oss;
                oss << "COMMIT " << std::hex 
                    << pc << " 0 0 0\n";  // Simplified - need to extract actual rd/wdata
                
                send_response(oss.str());
                
                count++;
                
                // Check for exit conditions
                if (check_exit_condition()) {
                    break;
                }
            }
            
            send_response("DONE 0\n");
            return true;
        } catch (const std::exception& e) {
            std::string msg = std::string("ERROR Execution failed: ") + e.what() + "\n";
            send_response(msg);
            return false;
        }
    }
    
    bool check_exit_condition() {
        // Check for tohost write (common exit mechanism)
        // This is simplified - implement based on your exit detection needs
        return false;
    }
    
    void send_response(const std::string& msg) {
        if (client_fd_ >= 0) {
            write(client_fd_, msg.c_str(), msg.size());
        }
    }
    
    void cleanup() {
        if (client_fd_ >= 0) {
            close(client_fd_);
            client_fd_ = -1;
        }
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
        unlink(socket_path_.c_str());
    }
    
    std::string socket_path_;
    std::string isa_;
    int server_fd_;
    int client_fd_;
    sim_t* sim_;
    processor_t* proc_;
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <socket_path> [isa]" << std::endl;
        std::cerr << "Example: " << argv[0] << " /tmp/spike_daemon.sock rv32im" << std::endl;
        return 1;
    }
    
    std::string socket_path = argv[1];
    std::string isa = (argc > 2) ? argv[2] : "rv32im";
    
    // Install signal handlers
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);
    
    SpikeDaemon daemon(socket_path, isa);
    
    if (!daemon.start()) {
        return 1;
    }
    
    std::cerr << "[DAEMON] Ready" << std::endl;
    daemon.run();
    std::cerr << "[DAEMON] Shutdown" << std::endl;
    
    return 0;
}
