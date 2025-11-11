/**
 * @file SpikeExit.hpp
 * @brief Detection and parsing of Spike simulator fatal trap conditions
 * 
 * This file provides utilities for detecting fatal exceptions and traps in
 * Spike's textual log output. It parses error messages to identify CPU
 * exceptions (illegal instructions, misaligned access, etc.) during golden
 * model execution.
 */

#pragma once

#include <string>

/**
 * @brief Detect and parse fatal trap conditions from Spike log output
 * 
 * Scans a line of Spike's log output for fatal trap indicators (exceptions,
 * illegal instructions, alignment faults, etc.). If a trap is detected,
 * extracts a human-readable summary of the failure condition.
 * 
 * Detectable conditions include:
 * - Illegal instructions
 * - Misaligned memory accesses
 * - Invalid CSR operations
 * - Page faults
 * - Access violations
 * - Other RISC-V exceptions
 * 
 * @param line A single line of text from Spike's log output
 * @param summary Output parameter filled with trap description if detected
 * 
 * @return true if a fatal trap was detected, false if line is benign
 * 
 * Example usage:
 * @code
 *   std::string line = "core   0: trap illegal_instruction, epc 0x80000004";
 *   std::string summary;
 *   if (detect_spike_fatal_trap(line, summary)) {
 *     std::cout << "Spike trapped: " << summary << std::endl;
 *     // Output: "Spike trapped: illegal_instruction at epc 0x80000004"
 *   }
 * @endcode
 * 
 * @code
 *   // During Spike parsing:
 *   while (std::getline(spike_stream, line)) {
 *     std::string trap_info;
 *     if (detect_spike_fatal_trap(line, trap_info)) {
 *       log_error("Golden model trapped: " + trap_info);
 *       break;  // Stop execution, report divergence
 *     }
 *   }
 * @endcode
 * 
 * @note This function only detects traps that appear in Spike's log output
 * @note The exact log format may vary between Spike versions
 * @see SpikeProcess::next_commit() for integration with Spike log parsing
 */
bool detect_spike_fatal_trap(const std::string& line, std::string& summary);
