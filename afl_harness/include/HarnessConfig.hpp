/**
 * @file HarnessConfig.hpp
 * @brief Configuration management for AFL++ fuzzing harness
 * 
 * Loads settings from harness.conf file and environment variables.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

/**
 * @brief Central configuration for fuzzing harness runtime parameters
 * 
 * Configuration is loaded from {PROJECT_ROOT}/afl_harness/harness.conf
 * and environment variables. Call loadconfig() after construction.
 */
struct HarnessConfig {
  std::string crash_dir;              ///< Crash log directory: {PROJECT_ROOT}/workdir/logs/crash
  std::string trace_dir;              ///< Trace output directory: {PROJECT_ROOT}/workdir/traces
  std::string objdump;                ///< Path to RISC-V objdump binary (from OBJDUMP in harness.conf)
  int xlen = 32;                      ///< ISA register width - 32 or 64 bits (from XLEN in harness.conf)
  unsigned max_cycles = 10000;        ///< Maximum clock cycles per test case (from MAX_CYCLES in harness.conf)
  bool stop_on_spike_done = true;     ///< Stop execution when Spike exits (from STOP_ON_SPIKE_DONE in harness.conf)
  bool use_tohost = true;             ///< Always enabled - monitors tohost address for program completion
  uint32_t tohost_addr = 0;           ///< Memory address for tohost register (from TOHOST_ADDR environment variable)
  unsigned pc_stagnation_limit = 512; ///< Max instructions at same PC before timeout (from PC_STAGNATION_LIMIT in harness.conf)
  unsigned max_program_words = 256;   ///< Maximum program size in 32-bit words (from MAX_PROGRAM_WORDS in harness.conf)

  /**
   * @brief Parse .conf file (KEY=value format) into map
   * 
   * Reads a .conf file with KEY=value format (one per line).
   * Supports comments starting with # and section headers in [brackets].
   * 
   * @param conf_path Path to configuration file
   * @return Map of key-value pairs from the config file
   */
  static std::unordered_map<std::string, std::string> parse_conf_file(const std::string& conf_path);

  /**
   * @brief Load configuration from harness.conf and environment variables
   * 
   * Reads from {PROJECT_ROOT}/afl_harness/harness.conf and environment.
   * PROJECT_ROOT and TOHOST_ADDR come from environment variables.
   * Other settings loaded from harness.conf file.
   * Logs all configuration values for debugging.
   */
  void loadconfig();
};
