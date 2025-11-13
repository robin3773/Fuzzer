/**
 * @file spike_daemon.cpp
 * @brief Simple Spike wrapper daemon for differential testing
 * 
 * Receives binary programs via Unix socket, spawns Spike, parses output,
 * and streams commits back to client.
 * 
 * Protocol:
 *   LOAD <len>\n<binary>       - Load and execute program
 *   QUIT\n                     - Shutdown
 * 
 * Responses:
 *   COMMIT <pc> <insn>\n       - Instruction commit
 *   DONE\n                     - Execution finished
 *   ERROR <msg>\n              - Error occurred
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
#include <cstdint>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <signal.h>

static volatile sig_atomic_t g_shutdown = 0;
static void sig_handler(int) { g_shutdown = 1; }

// Build ELF from binary using objcopy + ld
std::string build_elf(const std::vector<uint8_t>& binary) {
    char tmpbin[] = "/tmp/spike_daemon_XXXXXX.bin";
    int fd = mkstemps(tmpbin, 4);
    if (fd < 0) return "";
    
    write(fd, binary.data(), binary.size());
    close(fd);
    
    std::string tmpbin_str(tmpbin);
    std::string tmpobj = tmpbin_str + ".o";
    std::string tmpelf = tmpbin_str + ".elf";
    
    // Convert binary to object
    std::string cmd = "/opt/riscv/bin/riscv32-unknown-elf-objcopy -I binary -O elf32-littleriscv -B riscv:rv32 " + 
                      tmpbin_str + " " + tmpobj + " 2>/dev/null";
    
    if (system(cmd.c_str()) != 0) {
        unlink(tmpbin);
        return "";
    }
    
    // Link to ELF
    cmd = "/opt/riscv/bin/riscv32-unknown-elf-ld -T /home/robin/HAVEN/Fuzz/tools/link.ld " +
          tmpobj + " -o " + tmpelf + " 2>/dev/null";
    
    if (system(cmd.c_str()) != 0) {
        unlink(tmpbin);
        unlink(tmpobj.c_str());
        return "";
    }
    
    unlink(tmpbin);
    unlink(tmpobj.c_str());
    return tmpelf;
}

// Run Spike and parse output
bool run_spike(const std::string& elf_path, int client_fd) {
    std::string cmd = "/opt/riscv/bin/spike -l --isa=rv32im " + elf_path + " 2>&1";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::string err = "ERROR Failed to spawn Spike\n";
        write(client_fd, err.c_str(), err.size());
        return false;
    }
    
    // Parse Spike output line by line
    char line[1024];
    std::regex commit_re("core\\s+0:\\s+0x([0-9a-fA-F]+)\\s+\\(0x([0-9a-fA-F]+)\\)");
    
    while (fgets(line, sizeof(line), pipe)) {
        std::smatch m;
        std::string line_str(line);
        
        if (std::regex_search(line_str, m, commit_re)) {
            // Extract PC and instruction
            std::string pc_hex = m[1].str();
            std::string insn_hex = m[2].str();
            
            // Send commit to client
            std::string msg = "COMMIT " + pc_hex + " " + insn_hex + "\n";
            write(client_fd, msg.c_str(), msg.size());
        }
    }
    
    int status = pclose(pipe);
    
    // Send done message
    std::string done_msg = "DONE\n";
    write(client_fd, done_msg.c_str(), done_msg.size());
    
    return true;
}

class SpikeDaemon {
public:
    SpikeDaemon(const std::string& socket_path)
        : socket_path_(socket_path), server_fd_(-1) {}
    
    ~SpikeDaemon() {
        if (server_fd_ >= 0) close(server_fd_);
        unlink(socket_path_.c_str());
    }
    
    bool start() {
        server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd_ < 0) return false;
        
        unlink(socket_path_.c_str());
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);
        
        if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) return false;
        if (listen(server_fd_, 5) < 0) return false;
        
        std::cerr << "[DAEMON] Listening on " << socket_path_ << std::endl;
        return true;
    }
    
    void run() {
        while (!g_shutdown) {
            int client_fd = accept(server_fd_, nullptr, nullptr);
            if (client_fd < 0) {
                if (errno == EINTR) continue;
                break;
            }
            
            handle_client(client_fd);
            close(client_fd);
        }
    }
    
private:
    void handle_client(int client_fd) {
        std::string buffer;
        char buf[4096];
        
        while (true) {
            ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
            if (n <= 0) break;
            
            buf[n] = '\0';
            buffer += buf;
            
            // Process commands
            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string cmd = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                
                std::istringstream iss(cmd);
                std::string verb;
                iss >> verb;
                
                if (verb == "QUIT") {
                    std::string ok = "OK\n";
                    write(client_fd, ok.c_str(), ok.size());
                    return;
                }
                else if (verb == "LOAD") {
                    size_t len;
                    iss >> len;
                    
                    // Read binary data
                    std::vector<uint8_t> binary(len);
                    size_t total = 0;
                    while (total < len) {
                        n = read(client_fd, binary.data() + total, len - total);
                        if (n <= 0) return;
                        total += n;
                    }
                    
                    // Build ELF
                    std::string elf_path = build_elf(binary);
                    if (elf_path.empty()) {
                        std::string err = "ERROR Failed to build ELF\n";
                        write(client_fd, err.c_str(), err.size());
                        continue;
                    }
                    
                    // Run Spike
                    run_spike(elf_path, client_fd);
                    
                    // Cleanup
                    unlink(elf_path.c_str());
                }
            }
        }
    }
    
    std::string socket_path_;
    int server_fd_;
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <socket_path>" << std::endl;
        return 1;
    }
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);
    
    SpikeDaemon daemon(argv[1]);
    
    if (!daemon.start()) {
        std::cerr << "[DAEMON] Failed to start" << std::endl;
        return 1;
    }
    
    daemon.run();
    std::cerr << "[DAEMON] Shutdown" << std::endl;
    
    return 0;
}
