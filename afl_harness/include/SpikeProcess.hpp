/**
 * @file SpikeProcess.hpp
 * @brief Wrapper for the Spike RISC-V ISA simulator used as golden reference model
 * 
 * This file provides SpikeProcess, a robust interface to the Spike simulator that:
 * 1. Launches Spike as a subprocess with logging enabled
 * 2. Streams and archives Spike's textual output for debugging
 * 3. Parses commit lines to extract instruction-level execution traces
 * 4. Detects and reports fatal traps and exceptions
 * 
 * SpikeProcess enables differential testing by providing a cycle-accurate
 * golden reference against which the DUT (Device Under Test) can be compared.
 */

#pragma once

#include "Trace.hpp"
#include <boost/process.hpp>
#include <cstdio>
#include <memory>
#include <optional>
#include <regex>
#include <string>

/**
 * @class SpikeProcess
 * @brief Subprocess wrapper for Spike RISC-V simulator with log parsing
 * 
 * SpikeProcess manages the lifecycle of a Spike simulator instance, capturing
 * and parsing its output to extract instruction-level execution information.
 * This enables differential testing between Spike (golden model) and the DUT.
 * 
 * Key features:
 * - **Process management**: Spawns, monitors, and terminates Spike subprocess
 * - **Log archival**: Streams raw output to file for post-mortem analysis
 * - **Commit parsing**: Extracts PC, instruction, register writes, memory ops
 * - **Trap detection**: Identifies fatal exceptions in Spike's output
 * - **Best-effort parsing**: Handles variations in Spike log format gracefully
 * 
 * Architecture:
 * @code
 *   [SpikeProcess] ──spawn──> [spike -l --isa=rv32im test.elf]
 *          │                           │
 *          │◄──────stdout/stderr───────┘
 *          │
 *          ├──> [spike.log file]   (raw archive)
 *          └──> [CommitRec stream] (parsed commits)
 * @endcode
 * 
 * Typical workflow:
 * @code
 *   SpikeProcess spike;
 *   spike.set_log_path("workdir/spike.log");
 *   
 *   if (!spike.start("/opt/riscv/bin/spike", "test.elf", "rv32im")) {
 *     std::cerr << "Spike failed: " << spike.last_error() << "\n";
 *     return false;
 *   }
 *   
 *   CommitRec golden;
 *   while (spike.next_commit(golden)) {
 *     // Compare golden.pc_r against DUT's PC...
 *     if (dut_pc != golden.pc_r) {
 *       std::cerr << "Divergence at PC 0x" << std::hex << golden.pc_r << "\n";
 *       break;
 *     }
 *   }
 *   
 *   spike.stop();
 *   if (spike.exited()) {
 *     std::cout << "Spike exit code: " << spike.exit_code() << "\n";
 *   }
 * @endcode
 * 
 * @note Spike must be compiled and accessible at the specified path
 * @note Log format parsing is best-effort and may need adjustment for different Spike versions
 * @see CommitRec for the structure of parsed instruction records
 */
class SpikeProcess {
public:
  /**
   * @brief Default constructor creates an inactive SpikeProcess
   * 
   * No external resources are acquired until start() is called. This allows
   * deferred initialization and configuration before launching Spike.
   * 
   * Example:
   * @code
   *   SpikeProcess spike;  // No Spike process running yet
   *   spike.set_log_path("logs/spike.log");
   *   spike.start("/opt/riscv/bin/spike", "test.elf", "rv32imc");
   * @endcode
   */
  SpikeProcess() = default;
  
  /**
   * @brief Destructor ensures cleanup of Spike process and log file
   * 
   * Automatically calls stop() to wait for the Spike subprocess (if running)
   * and close the log file. This ensures no zombie processes or file handle
   * leaks even if stop() wasn't called explicitly.
   */
  ~SpikeProcess() { stop(); }

  /**
   * @brief Configure where to archive Spike's raw output
   * 
   * Sets the path for the log file where Spike's complete textual output
   * will be written. The file is opened in append mode (never truncates)
   * and uses line-buffering for live updates during execution.
   * 
   * If the log path is empty or not set, Spike's output is still processed
   * but not archived to disk. This is useful for performance-critical
   * scenarios where log I/O overhead must be minimized.
   * 
   * @param p Full path to log file (e.g., "workdir/logs/spike.log")
   * 
   * @note Must be called before start() to take effect
   * @note File is opened in append mode, preserving previous content
   * @note Line-buffered to ensure entries appear immediately
   * 
   * Example:
   * @code
   *   spike.set_log_path("/tmp/fuzzing/spike_20250111T143052.log");
   *   spike.start(...);
   *   // All Spike output now appends to the specified file
   * @endcode
   * 
   * @code
   *   spike.set_log_path("");  // Disable file logging (parse only)
   * @endcode
   */
  void set_log_path(const std::string& p) { log_path_ = p; }

  /**
   * @brief Get the exact shell command used to launch Spike
   * 
   * Returns the complete command string that was used to spawn Spike,
   * including all arguments and redirections. This is invaluable for:
   * - Manual reproduction of divergences
   * - Debugging Spike invocation issues
   * - Generating reproduction scripts
   * 
   * @return Full shell command (empty if start() not yet called)
   * 
   * Example output:
   * @code
   *   "/opt/riscv/bin/spike -l --isa=rv32im /tmp/test_12345.elf 2>&1"
   * @endcode
   * 
   * Example usage:
   * @code
   *   if (!spike.start(...)) {
   *     std::cerr << "Failed to start: " << spike.command() << "\n";
   *   }
   *   
   *   // In crash report:
   *   details += "Reproduction command:\n";
   *   details += spike.command() + "\n";
   * @endcode
   */
  const std::string& command() const { return spike_cmd_; }
  
  /**
   * @brief Get the configured log file path
   * 
   * Returns the path previously set by set_log_path(), regardless of whether
   * the file was successfully opened.
   * 
   * @return Log file path (empty if not configured)
   */
  const std::string& log_path() const { return log_path_; }
  
  /**
   * @brief Check if a fatal trap was detected in Spike's output
   * 
   * Returns true if Spike encountered a trap or exception that terminated
   * execution abnormally (illegal instruction, misaligned access, page fault,
   * etc.). This indicates the input triggered undefined behavior in the ISA.
   * 
   * @return true if fatal trap detected, false if Spike exited cleanly
   * 
   * @see fatal_trap_summary() for trap details
   * @see detect_spike_fatal_trap() for trap detection logic
   */
  bool saw_fatal_trap() const { return fatal_trap_seen_; }
  
  /**
   * @brief Get human-readable description of the fatal trap
   * 
   * Returns a string describing the trap that Spike encountered, extracted
   * from Spike's log output. Only valid when saw_fatal_trap() returns true.
   * 
   * @return Trap description (e.g., "illegal_instruction at epc 0x80000004")
   * 
   * Example:
   * @code
   *   if (spike.saw_fatal_trap()) {
   *     std::cerr << "Spike trapped: " << spike.fatal_trap_summary() << "\n";
   *     // Output: "Spike trapped: illegal_instruction at epc 0x80000004"
   *   }
   * @endcode
   */
  const std::string& fatal_trap_summary() const { return fatal_trap_summary_; }

  /**
   * @brief Check if process status is available
   * 
   * Returns true after stop() has been called or after Spike has terminated,
   * indicating that wait status information is valid. Before stop() or if
   * start() never succeeded, this returns false.
   * 
   * @return true if status information is available
   * 
   * @see raw_status(), exited(), exit_code(), signaled()
   */
  bool has_status() const { return status_valid_; }
  
  /**
   * @brief Get raw wait status from the subprocess
   * 
   * Returns the raw integer status from wait(), which can be decoded using
   * WIFEXITED, WEXITSTATUS, WIFSIGNALED, etc. Only valid when has_status()
   * returns true.
   * 
   * @return Raw wait status (platform-specific encoding)
   * 
   * @note Use exited()/exit_code()/signaled() for portable status checking
   */
  int raw_status() const { return last_status_; }
  
  /**
   * @brief Check if Spike exited normally (not signaled)
   * 
   * Returns true if Spike terminated by calling exit() or returning from
   * main(), rather than being killed by a signal. When true, exit_code()
   * contains the exit value.
   * 
   * @return true if process exited normally
   * 
   * @see exit_code()
   */
  bool exited() const { return status_valid_ && WIFEXITED(last_status_); }
  
  /**
   * @brief Get the exit code from a normal exit
   * 
   * Returns the value passed to exit() or returned from main(). Only valid
   * when exited() returns true; otherwise returns -1.
   * 
   * Common exit codes:
   * - 0: Success
   * - 1: General error
   * - 127: Command not found (Spike binary doesn't exist)
   * - >128: Usually indicates signal termination
   * 
   * @return Exit code (0-255), or -1 if not applicable
   * 
   * Example:
   * @code
   *   spike.stop();
   *   if (spike.exited()) {
   *     if (spike.exit_code() == 0) {
   *       std::cout << "Spike completed successfully\n";
   *     } else {
   *       std::cerr << "Spike failed with code " << spike.exit_code() << "\n";
   *     }
   *   }
   * @endcode
   */
  int exit_code() const { return exited() ? WEXITSTATUS(last_status_) : -1; }
  
  /**
   * @brief Check if Spike was terminated by a signal
   * 
   * Returns true if Spike was killed by a signal (SIGKILL, SIGSEGV, etc.)
   * rather than exiting normally. When true, term_signal() contains the
   * signal number.
   * 
   * @return true if process was terminated by signal
   * 
   * @see term_signal()
   */
  bool signaled() const { return status_valid_ && WIFSIGNALED(last_status_); }
  
  /**
   * @brief Get the termination signal number
   * 
   * Returns the signal that killed Spike. Only valid when signaled()
   * returns true; otherwise returns -1.
   * 
   * Common signals:
   * - SIGKILL (9): Forceful termination
   * - SIGSEGV (11): Segmentation fault
   * - SIGABRT (6): Abort signal
   * 
   * @return Signal number, or -1 if not applicable
   * 
   * Example:
   * @code
   *   if (spike.signaled()) {
   *     std::cerr << "Spike killed by signal " << spike.term_signal();
   *     if (spike.term_signal() == SIGSEGV) {
   *       std::cerr << " (segmentation fault)\n";
   *     }
   *   }
   * @endcode
   */
  int term_signal() const { return signaled() ? WTERMSIG(last_status_) : -1; }
  
  /**
   * @brief Get description of the last start() failure
   * 
   * Returns an error message describing why start() failed, if applicable.
   * Empty if start() succeeded or hasn't been called.
   * 
   * Typical errors:
   * - "Spike binary not found"
   * - "Failed to spawn process"
   * - "ELF file not accessible"
   * 
   * @return Error description (empty on success)
   * 
   * Example:
   * @code
   *   if (!spike.start("/bad/path/spike", "test.elf", "rv32i")) {
   *     std::cerr << "Start failed: " << spike.last_error() << "\n";
   *     // Output: "Start failed: Spike binary not found"
   *   }
   * @endcode
   */
  const std::string& last_error() const { return start_error_; }
  
  /**
   * @brief Get human-readable one-line status summary
   * 
   * Generates a descriptive string summarizing how Spike terminated:
   * - "exited with code N"
   * - "killed by signal N"
   * - "no status available"
   * 
   * @return Status description string
   * 
   * Example:
   * @code
   *   spike.stop();
   *   std::cout << "Spike " << spike.status_string() << "\n";
   *   // Output: "Spike exited with code 0"
   *   // Or: "Spike killed by signal 11"
   * @endcode
   */
  std::string status_string() const;

  /**
   * @brief Launch Spike simulator as a subprocess
   * 
   * Spawns Spike with the specified ISA and ELF file, redirecting its output
   * for parsing. The exact command executed is:
   * @code
   *   spike_bin -l --isa=<isa> <elf_path>        (without Proxy Kernel)
   *   spike_bin -l --isa=<isa> <pk_bin> <elf_path>   (with Proxy Kernel)
   * @endcode
   * 
   * The -l flag enables detailed logging of every instruction commit, which
   * this class parses to extract CommitRec structures.
   * 
   * On success:
   * - Spike is running as a child process
   * - Output stream is connected for reading
   * - Log file is opened (if configured)
   * - Regular expressions are initialized for parsing
   * 
   * On failure:
   * - No process is spawned
   * - last_error() contains failure reason
   * - Any previous Spike instance is terminated first
   * 
   * @param spike_bin Absolute path to Spike executable (e.g., /opt/riscv/bin/spike)
   * @param elf_path Path to ELF binary to execute (DUT input wrapped as ELF)
   * @param isa ISA string for Spike (e.g., "rv32imc", "rv64imac")
   * @param pk_bin Optional path to Proxy Kernel; if provided, Spike runs pk which loads the ELF
   * 
   * @return true if Spike started successfully, false on error (check last_error())
   * 
   * @note Terminates any previously running Spike instance
   * @note Stderr is redirected to stdout so all output appears in one stream
   * @note The ELF file must exist and be readable
   * 
   * Example (bare metal):
   * @code
   *   if (!spike.start("/opt/riscv/bin/spike",
   *                    "test_input.elf",
   *                    "rv32im")) {
   *     std::cerr << "Failed: " << spike.last_error() << "\n";
   *     return false;
   *   }
   * @endcode
   * 
   * Example (with Proxy Kernel):
   * @code
   *   spike.start("/opt/riscv/bin/spike",
   *               "user_program.elf",
   *               "rv64imac",
   *               "/opt/riscv/riscv64-unknown-elf/bin/pk");
   *   // Runs: spike -l --isa=rv64imac pk user_program.elf
   * @endcode
   */
  bool start(const std::string& spike_bin, const std::string& elf_path,
             const std::string& isa = "rv32imc",
             const std::string& pk_bin = std::string());

  /**
   * @brief Stop Spike and wait for it to exit
   * 
   * Waits for the Spike subprocess to terminate (if running), records its
   * wait status, releases the output stream, and closes the log file.
   * After stop(), status methods (exited(), exit_code(), etc.) become valid.
   * 
   * If Spike is not running, this is a no-op. It's safe to call stop()
   * multiple times.
   * 
   * @note Blocking call - waits for Spike to exit
   * @note Automatically called by destructor
   * 
   * Example:
   * @code
   *   spike.start(...);
   *   CommitRec rec;
   *   while (spike.next_commit(rec)) {
   *     // Process commits...
   *   }
   *   spike.stop();  // Wait for Spike to fully exit
   *   
   *   if (spike.exit_code() != 0) {
   *     std::cerr << "Spike failed\n";
   *   }
   * @endcode
   */
  void stop();

  /**
   * @brief Read and parse the next instruction commit from Spike
   * 
   * Blocking read operation that consumes Spike's output until a commit line
   * is found, then parses instruction details into a CommitRec structure.
   * 
   * Parsing process:
   * 1. Read lines until a commit line is encountered
   * 2. Extract PC and instruction encoding from commit line
   * 3. Read a short window of subsequent lines for register/memory operations
   * 4. Populate CommitRec with all available information
   * 5. Write grouped instruction chunk to log file (if configured)
   * 6. Check for fatal trap indicators
   * 
   * Log file format:
   * Each instruction is written as a grouped chunk enclosed by markers:
   * @code
   *   ========== INSTRUCTION #42 [PC=0x80000010, INSN=0x00108093] ==========
   *   <raw Spike output lines for this instruction>
   *   ========== END #42 ==========
   * @endcode
   * 
   * CommitRec fields populated:
   * - pc_r, pc_w: Extracted from commit line
   * - insn: Instruction encoding from commit line
   * - rd_addr, rd_wdata: Parsed from register write lines (best-effort)
   * - mem_addr, mem_rmask, mem_wmask: Basic memory info when detectable
   * - trap: Set if exception detected
   * 
   * @param rec Output parameter filled with parsed commit data
   * 
   * @return true if commit was successfully parsed, false on EOF or error
   * 
   * @note Returns false when Spike terminates; stop() is called internally
   * @note Parsing is best-effort; some fields may be zero if not in log
   * @note Blocking call - waits for Spike to produce output
   * 
   * Example:
   * @code
   *   CommitRec golden;
   *   size_t insn_count = 0;
   *   
   *   while (spike.next_commit(golden)) {
   *     insn_count++;
   *     
   *     // Compare against DUT:
   *     if (golden.pc_r != dut.pc_r) {
   *       std::cerr << "PC divergence at instruction #" << insn_count << "\n";
   *       std::cerr << "  Spike: 0x" << std::hex << golden.pc_r << "\n";
   *       std::cerr << "  DUT:   0x" << std::hex << dut.pc_r << "\n";
   *       break;
   *     }
   *     
   *     if (golden.rd_addr != 0 && golden.rd_wdata != dut.rd_wdata) {
   *       std::cerr << "Register x" << golden.rd_addr << " mismatch\n";
   *       break;
   *     }
   *   }
   *   
   *   if (spike.saw_fatal_trap()) {
   *     std::cerr << "Spike trapped: " << spike.fatal_trap_summary() << "\n";
   *   }
   * @endcode
   */
  bool next_commit(CommitRec& rec);

private:
  /**
   * @brief Exact shell command used to launch Spike (for reproduction)
   */
  std::string spike_cmd_;
  
  /**
   * @brief Child process handle (boost::process)
   */
  std::optional<boost::process::child> child_;
  
  /**
   * @brief Merged stdout/stderr stream from Spike
   */
  std::unique_ptr<boost::process::ipstream> child_stream_;
  
  /**
   * @brief Path to raw log file (empty if logging disabled)
   */
  std::string log_path_;
  
  /**
   * @brief FILE* handle for log file (nullptr if not logging)
   */
  FILE* log_file_ = nullptr;
  
  /**
   * @brief Flag indicating a fatal trap was detected in Spike's output
   */
  bool fatal_trap_seen_ = false;
  
  /**
   * @brief Human-readable description of detected trap
   */
  std::string fatal_trap_summary_;
  
  /**
   * @brief Regex to detect commit lines in Spike's log
   * Format: "core   0: ... commit ..."
   */
  std::regex commit_re_;
  
  /**
   * @brief Regex to detect register writes
   * Format: "x1 0x00000001" or similar
   */
  std::regex reg_write_re_;
  
  /**
   * @brief Simplified regex for register writes (fallback)
   */
  std::regex simple_reg_re_;
  
  /**
   * @brief Last observed wait status from the subprocess
   */
  int last_status_ = 0;
  
  /**
   * @brief Flag indicating status validity (true after stop())
   */
  bool status_valid_ = false;
  
  /**
   * @brief Description of start() failure (empty on success)
   */
  std::string start_error_;
  
  /**
   * @brief Line buffered between calls when next commit already seen
   */
  std::string pending_line_;
  
  /**
   * @brief Monotonic instruction counter for log headers
   */
  size_t instr_index_ = 0;
};
