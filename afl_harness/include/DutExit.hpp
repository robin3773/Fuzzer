/**
 * @file DutExit.hpp
 * @brief Exit condition detection and classification for the Device Under Test
 * 
 * This file defines the types and utilities for detecting and categorizing
 * how a DUT (Device Under Test) terminates execution. Exit detection is
 * critical for distinguishing between successful test completion, timeout,
 * and various forms of CPU exceptions or deliberate exit mechanisms.
 */

#pragma once

#include "HarnessConfig.hpp"
#include <cstdint>
#include <string>
#include <vector>

// =============================================================================
// DUT Exit Handling — Clean termination and exit stub generation
// =============================================================================

/**
 * @enum ExitReason
 * @brief Classification of DUT termination conditions
 * 
 * Enumerates the different ways a DUT can exit or terminate during fuzzing.
 * Each reason indicates a specific exit mechanism or condition that ended
 * the test case execution.
 */
enum class ExitReason {
  /**
   * @brief No exit condition detected yet (still executing)
   * 
   * The DUT is actively executing and has not signaled any form of completion
   * or error condition. Execution should continue.
   */
  None,
  
  /**
   * @brief DUT signaled completion via finish signal
   * 
   * The DUT raised its finish signal (implementation-specific hardware signal
   * indicating successful program completion). This typically comes from an
   * exit stub or explicit finish instruction embedded in the test program.
   * 
   * Example: PicoRV32's `picorv32_valid` && finish condition
   */
  Finish,
  
  /**
   * @brief DUT wrote to tohost address (RISC-V test convention)
   * 
   * The DUT performed a memory write to the configured tohost address,
   * following the RISC-V test program convention for signaling completion
   * to the host environment. A non-zero write indicates program exit.
   * 
   * Common tohost address: 0x80001000
   * @see HarnessConfig::tohost_addr
   */
  Tohost,
  
  /**
   * @brief DUT executed an ECALL instruction
   * 
   * The DUT encountered an ECALL (environment call) instruction, which
   * typically requests services from a higher privilege level or operating
   * system. In bare-metal fuzzing, this may indicate an intentional exit
   * or an unexpected system call.
   * 
   * RISC-V opcode: 0x00000073
   */
  Ecall,
  
  /**
   * @brief Golden model (Spike) completed first
   * 
   * The Spike simulator (golden reference model) finished execution before
   * the DUT signaled completion. When STOP_ON_SPIKE_DONE is enabled, DUT
   * execution is terminated at this point to maintain synchronization with
   * the golden model for differential testing.
   * 
   * @see HarnessConfig::stop_on_spike_done
   */
  SpikeDone
};

/**
 * @brief Convert ExitReason enum to human-readable string
 * 
 * Provides a textual description of the exit reason for logging, crash
 * reports, and debugging output.
 * 
 * @param reason The exit reason to convert
 * @return Constant string describing the reason (never returns nullptr)
 * 
 * Example output strings:
 * - ExitReason::None → "None"
 * - ExitReason::Finish → "Finish"
 * - ExitReason::Tohost → "Tohost"
 * - ExitReason::Ecall → "Ecall"
 * - ExitReason::SpikeDone → "SpikeDone"
 * 
 * Example usage:
 * @code
 *   ExitReason reason = ExitReason::Finish;
 *   printf("DUT exited: %s\n", exit_reason_text(reason));
 *   // Output: "DUT exited: Finish"
 * @endcode
 * 
 * @code
 *   // In crash logging:
 *   std::string crash_name = std::string("exit_") + exit_reason_text(exit_condition);
 *   // Creates filenames like: "crash_exit_Finish_20250111T123456.bin"
 * @endcode
 */
const char* exit_reason_text(ExitReason reason);
