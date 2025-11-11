/**
 * @file CrashDetection.hpp
 * @brief Runtime crash detection checks for CPU execution anomalies
 * 
 * This module provides various sanity checks that detect illegal or 
 * unexpected CPU behavior during execution. Each check examines RVFI
 * (RISC-V Formal Interface) signals and logs crashes when violations
 * are detected.
 * 
 * All checks return true if a crash was detected and logged, allowing
 * the caller to terminate execution immediately.
 */

#pragma once

#include "CpuIface.hpp"
#include "CrashLogger.hpp"
#include <cstdint>
#include <vector>

namespace crash_detection {

/**
 * @brief Check for illegal writes to x0 register
 * 
 * RISC-V mandates that x0 must always read as zero. Any attempt to
 * write a non-zero value to x0 indicates a hardware bug.
 * 
 * @param cpu CPU interface providing RVFI signals
 * @param logger Crash logger for recording violations
 * @param cyc Current execution cycle number
 * @param input Fuzzer input that triggered the violation
 * @return true if violation detected and logged, false otherwise
 */
bool check_x0_write(const CpuIface* cpu, const CrashLogger& logger, 
                    unsigned cyc, const std::vector<unsigned char>& input);

/**
 * @brief Check for misaligned PC values
 * 
 * RISC-V requires PC to be aligned based on instruction encoding:
 * - Standard instructions: 4-byte aligned (PC[1:0] = 0)
 * - Compressed instructions: 2-byte aligned (PC[0] = 0)
 * 
 * This check enforces 2-byte minimum alignment (odd PC is always illegal).
 * 
 * @param cpu CPU interface providing RVFI signals
 * @param logger Crash logger for recording violations
 * @param cyc Current execution cycle number
 * @param input Fuzzer input that triggered the violation
 * @return true if violation detected and logged, false otherwise
 */
bool check_pc_misaligned(const CpuIface* cpu, const CrashLogger& logger,
                         unsigned cyc, const std::vector<unsigned char>& input);

/**
 * @brief Check for misaligned or irregular memory loads
 * 
 * Validates that memory load operations have proper alignment and
 * contiguous byte masks:
 * - Byte loads (1 byte): any alignment allowed
 * - Halfword loads (2 bytes): must be 2-byte aligned
 * - Word loads (4 bytes): must be 4-byte aligned
 * 
 * Also checks that the byte mask (rmask) is contiguous. Non-contiguous
 * masks indicate hardware bugs.
 * 
 * @param cpu CPU interface providing RVFI signals
 * @param logger Crash logger for recording violations
 * @param cyc Current execution cycle number
 * @param input Fuzzer input that triggered the violation
 * @return true if violation detected and logged, false otherwise
 */
bool check_mem_align_load(const CpuIface* cpu, const CrashLogger& logger,
                          unsigned cyc, const std::vector<unsigned char>& input);

/**
 * @brief Check for misaligned or irregular memory stores
 * 
 * Validates that memory store operations have proper alignment and
 * contiguous byte masks:
 * - Byte stores (1 byte): any alignment allowed
 * - Halfword stores (2 bytes): must be 2-byte aligned
 * - Word stores (4 bytes): must be 4-byte aligned
 * 
 * Also checks that the byte mask (wmask) is contiguous. Non-contiguous
 * masks indicate hardware bugs.
 * 
 * @param cpu CPU interface providing RVFI signals
 * @param logger Crash logger for recording violations
 * @param cyc Current execution cycle number
 * @param input Fuzzer input that triggered the violation
 * @return true if violation detected and logged, false otherwise
 */
bool check_mem_align_store(const CpuIface* cpu, const CrashLogger& logger,
                           unsigned cyc, const std::vector<unsigned char>& input);

/**
 * @brief Check for execution timeout
 * 
 * Detects when the program has executed for too many cycles without
 * completing. This prevents infinite loops from hanging the fuzzer.
 * 
 * @param cyc Current execution cycle number
 * @param max_cycles Maximum allowed cycles before timeout
 * @param cpu CPU interface providing RVFI signals for crash logging
 * @param logger Crash logger for recording timeout
 * @param input Fuzzer input that triggered the timeout
 * @return true if timeout detected and logged, false otherwise
 */
bool check_timeout(unsigned cyc, unsigned max_cycles, const CpuIface* cpu,
                   const CrashLogger& logger, const std::vector<unsigned char>& input);

/**
 * @brief Check for PC stagnation (infinite loop detection)
 * 
 * Detects when the CPU is stuck executing at the same PC address for
 * too many consecutive commits. This catches tight loops that would
 * otherwise consume the full cycle budget.
 * 
 * This function maintains internal state to track the last PC and
 * consecutive count. It should be called on every valid commit.
 * 
 * @param cpu CPU interface providing RVFI signals
 * @param logger Crash logger for recording stagnation
 * @param cyc Current execution cycle number
 * @param input Fuzzer input that triggered the stagnation
 * @param stagnation_limit Max consecutive commits at same PC (0 = disabled)
 * @param last_pc Reference to last PC value (maintains state between calls)
 * @param last_pc_valid Reference to validity flag for last_pc
 * @param stagnation_count Reference to consecutive count (maintains state)
 * @return true if stagnation detected and logged, false otherwise
 */
bool check_pc_stagnation(const CpuIface* cpu, const CrashLogger& logger,
                         unsigned cyc, const std::vector<unsigned char>& input,
                         unsigned stagnation_limit, uint32_t& last_pc,
                         bool& last_pc_valid, unsigned& stagnation_count);

/**
 * @brief Check for CPU trap condition
 * 
 * Detects when the CPU signals a trap condition via RVFI. Traps indicate
 * exceptions, illegal instructions, or other abnormal execution events.
 * 
 * @param cpu CPU interface providing RVFI signals
 * @param logger Crash logger for recording trap
 * @param cyc Current execution cycle number
 * @param input Fuzzer input that triggered the trap
 * @return true if trap detected and logged, false otherwise
 */
bool check_trap(const CpuIface* cpu, const CrashLogger& logger,
                unsigned cyc, const std::vector<unsigned char>& input);

} // namespace crash_detection
