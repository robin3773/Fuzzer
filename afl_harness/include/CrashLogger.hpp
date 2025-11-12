/**
 * @file CrashLogger.hpp
 * @brief Crash artifact generation and triage support for AFL++ fuzzer
 * 
 * This file provides the CrashLogger class which handles the creation of
 * comprehensive crash reports when bugs are discovered during fuzzing.
 * Each crash produces multiple artifacts: the raw binary input, a formatted
 * log with hexdump and disassembly, and optionally detailed diagnostic
 * information for complex failures.
 */

#pragma once
#include "HarnessConfig.hpp"
#include "Utils.hpp"
#include <cstdint>
#include <exception>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

/**
 * @class CrashLogger
 * @brief Generates crash artifacts for bug triage and reproduction
 * 
 * CrashLogger creates comprehensive crash reports when the fuzzer discovers
 * bugs. For each crash, it generates exactly two files:
 * - Binary input file (.bin) for exact reproduction
 * - Text log (.log) with hexdump, disassembly, context, and optional details
 * 
 * All artifacts share a common basename derived from the crash type, timestamp,
 * and cycle count, making it easy to correlate related files.
 * 
 * Filename format:
 * @code
 *   crash_<reason>_<timestamp>_cyc<cycle>.<ext>
 *   Example: crash_trap_20250111T143052_cyc1234.bin
 *            crash_trap_20250111T143052_cyc1234.log
 * @endcode
 * 
 * Benefits:
 * - **Reproducibility**: Binary inputs can be re-run through the harness
 * - **Triage efficiency**: Disassembly immediately shows problematic code
 * - **Context preservation**: Cycle count and reason aid debugging
 * - **Batch analysis**: Consistent naming enables scripted processing
 * - **Consolidated logs**: All crash information in a single text file
 * 
 * Example usage:
 * @code
 *   HarnessConfig cfg;
 *   cfg.loadconfig();
 *   CrashLogger logger(cfg);
 *   
 *   // When a bug is found:
 *   if (cpu->trap()) {
 *     logger.writeCrash("illegal_instruction",
 *                       cpu->rvfi_pc_rdata(),
 *                       cpu->rvfi_insn(),
 *                       cycle_count,
 *                       fuzzer_input);
 *   }
 *   
 *   // With extended diagnostic details:
 *   std::string details = "Register state:\n  x1: 0x" + to_hex(x1) + "\n";
 *   logger.writeCrash("divergence", pc, insn, cycle, input, details);
 * @endcode
 */
class CrashLogger {
public:
  /**
   * @brief Construct a CrashLogger with configuration
   * 
   * Creates a crash logger instance and ensures the crash output directory
   * exists. The directory is created immediately if it doesn't exist.
   * 
   * @param cfg Harness configuration containing crash_dir and objdump paths
   * 
   * Example:
   * @code
   *   HarnessConfig cfg;
   *   cfg.crash_dir = "/tmp/fuzzer/crashes";
   *   cfg.objdump = "/opt/riscv/bin/riscv32-unknown-elf-objdump";
   *   CrashLogger logger(cfg);
   *   // /tmp/fuzzer/crashes is created if it doesn't exist
   * @endcode
   */
  explicit CrashLogger(const HarnessConfig &cfg) : cfg_(cfg) {
    utils::ensure_dir(cfg_.crash_dir);
  }

  /**
   * @brief Write crash artifacts (binary input + comprehensive log)
   * 
   * Creates a complete crash report consisting of:
   * 1. Binary file (.bin): Raw fuzzer input for reproduction
   * 2. Log file (.log): Human-readable report with:
   *    - Crash reason and context (PC, instruction, cycle)
   *    - Hexdump of the input bytes
   *    - Disassembly of the instruction stream
   *    - Optional extended diagnostic details
   * 
   * Files are written atomically using temp files and rename to prevent
   * corruption if the fuzzer crashes during write.
   * 
   * @param reason Human-readable crash classification (e.g., "trap", "divergence")
   * @param pc Program counter at crash point
   * @param insn Instruction encoding that triggered the crash
   * @param cycle Execution cycle number when crash occurred
   * @param input Fuzzer-generated input that triggered the crash
   * @param details Optional extended diagnostic information (appended to log)
   * 
   * Generated files:
   * @code
   *   crash_trap_20250111T143052_cyc1234.bin  // Raw input bytes
   *   crash_trap_20250111T143052_cyc1234.log  // Complete crash report
   * @endcode
   * 
   * Log file format:
   * @code
   *   Reason: trap
   *   Cycle: 1234
   *   PC: 0x80000010
   *   Instruction: 0x00000000
   *   
   *   Hexdump:
   *   00000000  00 00 00 00 13 00 00 00  93 80 00 00 ...
   *   
   *   Disassembly:
   *      0:   00000000                (illegal)
   *      4:   00000013                nop
   *      8:   00008093                addi x1, x1, 0
   *   
   *   Details:
   *   [Extended diagnostic info if provided]
   * @endcode
   * 
   * Example usage (simple):
   * @code
   *   if (cpu->trap()) {
   *     logger.writeCrash("illegal_instruction",
   *                       0x80000004,
   *                       0x00000000,
   *                       1250,
   *                       input_bytes);
   *   }
   * @endcode
   * 
   * Example usage (with details):
   * @code
   *   // Differential testing divergence:
   *   std::string details;
   *   details += "Golden Model (Spike):\n";
   *   details += "  PC: 0x" + to_hex(spike_pc) + "\n";
   *   details += "  rd: 0x" + to_hex(spike_rd) + "\n\n";
   *   details += "DUT (PicoRV32):\n";
   *   details += "  PC: 0x" + to_hex(dut_pc) + "\n";
   *   details += "  rd: 0x" + to_hex(dut_rd) + " <-- MISMATCH\n";
   *   
   *   logger.writeCrash("divergence", dut_pc, dut_insn, cycle, input, details);
   * @endcode
   * 
   * @code
   *   // Timeout with context:
   *   if (cycle >= cfg.max_cycles) {
   *     std::string details = "Stuck at PC 0x" + to_hex(last_pc) + 
   *                           " for " + std::to_string(stagnation) + " cycles";
   *     logger.writeCrash("timeout", cpu->rvfi_pc_rdata(),
   *                       cpu->rvfi_insn(), cycle, input, details);
   *   }
   * @endcode
   */
  void writeCrash(const std::string &reason,
                  uint32_t pc,
                  uint32_t insn,
                  unsigned cycle,
                  const std::vector<unsigned char> &input,
                  const std::string &details = "") const {

    const std::string base = makeBaseName(reason, cycle);
    const std::string bin_path = base + ".bin";
    const std::string log_path = base + ".log";

    writeFile(bin_path, input);

    std::string log;
    log.reserve(4096);
    log += "Reason: " + reason + "\n";
    log += "Cycle: " + std::to_string(cycle) + "\n";
    log += "PC: 0x" + hex32(pc) + "\n";
    log += "Instruction: 0x" + hex32(insn) + "\n\n";

    log += "Hexdump:\n";
    log += utils::hexdump(input);
    log += "\n";

    std::string dasm = utils::disassemble(input, cfg_.objdump, cfg_.xlen);
    if (!dasm.empty()) {
      log += "Disassembly:\n";
      log += dasm;
    }

    if (!details.empty()) {
      log += "\n";
      log += "Details:\n";
      log += details;
      if (details.back() != '\n') {
        log += "\n";
      }
    }

    writeTextAtomically(log_path, log);
  }

private:
  /**
   * @brief Harness configuration (crash directory, objdump path, etc.)
   */
  HarnessConfig cfg_;

  /**
   * @brief Convert uint32_t to zero-padded 8-digit hex string
   * 
   * @param v Value to convert
   * @return Hex string (e.g., "8000004" for 0x80000004)
   * 
   * Example:
   * @code
   *   std::string pc_hex = hex32(0x80000004);  // "80000004"
   * @endcode
   */
  static std::string hex32(uint32_t v) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%08x", v);
    return std::string(buf);
  }
  
  /**
   * @brief Generate base filename for crash artifacts
   * 
   * Creates the common filename prefix used for all files related to a
   * single crash (binary, log, details).
   * 
   * @param reason Crash type/reason
   * @param cycle Execution cycle number
   * @return Full path without extension (e.g., "/path/crash_trap_20250111T143052_cyc1234")
   * 
   * Example:
   * @code
   *   std::string base = makeBaseName("trap", 1024);
   *   // => "/workdir/logs/crash/crash_trap_20250111T143052_cyc1024"
   * @endcode
   */
  std::string makeBaseName(const std::string &reason, unsigned cycle) const {
    std::string ts = utils::timestamp_now();
    return cfg_.crash_dir + "/crash_" + reason + "_" + ts + "_cyc" +
           std::to_string(cycle);
  }

  /**
   * @brief Atomically write binary data to file
   * 
   * Writes to a temporary file first, then renames to the target path.
   * This prevents corruption if the process crashes during write.
   * 
   * @param path Target file path
   * @param data Binary data to write
   * 
   * @note Logs errors to stderr but does not throw
   * 
   * Example:
   * @code
   *   std::vector<unsigned char> bytes = {0x13, 0x00, 0x00, 0x00};
   *   writeFile("/tmp/test.bin", bytes);
   * @endcode
   */
  static void writeFile(const std::string &path, const std::vector<unsigned char> &data) {
    std::string tmp = path + ".tmp";
    try {
      utils::safe_write_all(tmp, data.data(), data.size());
      std::error_code ec;
      std::filesystem::rename(tmp, path, ec);
      if (ec) {
        std::cerr << "[HARNESS/CRASH] Failed to rename " << tmp << " -> "
                  << path << ": " << ec.message() << "\n";
      }
    } catch (const std::exception &e) {
      std::cerr << "[HARNESS/CRASH] Failed to write crash bin '" << path
                << "': " << e.what() << "\n";
    }
  }

  /**
   * @brief Atomically write text data to file
   * 
   * Writes to a temporary file first, then renames to the target path.
   * This prevents corruption if the process crashes during write.
   * 
   * @param path Target file path
   * @param text Text content to write
   * 
   * @note Logs errors to stderr but does not throw
   * 
   * Example:
   * @code
   *   writeTextAtomically("/tmp/report.log", "Reason: trap\\n");
   * @endcode
   */
  static void writeTextAtomically(const std::string &path, const std::string &text) {
    std::string tmp = path + ".tmp";
    try {
      utils::safe_write_all(tmp, text.data(), text.size());
      std::error_code ec;
      std::filesystem::rename(tmp, path, ec);
      if (ec) {
        std::cerr << "[HARNESS/CRASH] Failed to rename " << tmp << " -> "
                  << path << ": " << ec.message() << "\n";
      }
    } catch (const std::exception &e) {
      std::cerr << "[HARNESS/CRASH] Failed to write crash log '" << path
                << "': " << e.what() << "\n";
    }
  }
};
