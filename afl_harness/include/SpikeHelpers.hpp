#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace spike_helpers {

/**
 * @brief Print the tail of a log file to hwfuzz debug output
 * 
 * Reads up to @p max_lines from the end of the specified log file and writes
 * them to the harness debug log. Handy when reporting Spike failures.
 * 
 * @param path Path to the log file on disk (e.g., workdir/logs/spike.log)
 * @param max_lines Maximum number of lines to print (default: 50)
 * 
 * Example:
 * @code
 *   spike_helpers::print_log_tail("/tmp/spike.log", 100);
 * @endcode
 */
void print_log_tail(const char* path, int max_lines = 50);

/**
 * @brief Build a temporary ELF from raw input using the specified linker
 * 
 * Converts the fuzzer's raw byte input into an object file via objcopy,
 * links it with the provided linker script, and returns the path to the
 * resulting ELF that Spike can execute.
 * 
 * @param input Raw binary input data from the fuzzer
 * @param ld_bin Path to the linker executable (e.g., riscv32-unknown-elf-ld)
 * @param linker_script Linker script describing memory layout
 * @return Absolute path to the generated ELF, or empty string on failure
 * 
 * Example:
 * @code
 *   std::string elf = spike_helpers::build_spike_elf(bytes, "/opt/riscv/bin/ld", "link.ld");
 *   if (elf.empty()) { // handle error
 *   }
 * @endcode
 */
std::string build_spike_elf(const std::vector<unsigned char>& input,
                            const std::string& ld_bin,
                            const std::string& linker_script);

/**
 * @brief Quote/escape an argument for safe logging
 * 
 * Returns the argument unchanged if it contains no whitespace or quotes,
 * otherwise wraps it in double quotes and escapes embedded quotes/backslashes.
 * 
 * @param arg Raw argument string
 * @return Escaped/quoted argument suitable for log output
 * 
 * Example:
 * @code
 *   std::string display = spike_helpers::format_arg("--isa=rv32imc");
 *   // "--isa=rv32imc"
 *   std::string path = spike_helpers::format_arg("/tmp/spike log.txt");
 *   // "\"/tmp/spike log.txt\""
 * @endcode
 */
std::string format_arg(const std::string& arg);

} // namespace spike_helpers
