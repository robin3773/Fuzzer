#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace spike_helpers {

/// @brief Print last N lines of a log file to debug output
/// @param path Path to log file
/// @param max_lines Maximum number of lines to print (default 50)
void print_log_tail(const char* path, int max_lines = 50);

/// @brief Build a temporary ELF file from raw binary input for Spike execution
/// @param input Raw binary input data
/// @param ram_base RAM base address for linking
/// @param ram_size RAM size for linking
/// @return Path to temporary ELF file, or empty string on failure
std::string build_spike_elf(const std::vector<unsigned char>& input);

/// @brief Format a command line argument for safe display (escape spaces/quotes)
/// @param arg Raw argument string
/// @return Escaped/quoted argument string
std::string format_arg(const std::string& arg);

} // namespace spike_helpers
