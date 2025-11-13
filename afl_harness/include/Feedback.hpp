/**
 * @file Feedback.hpp
 * @brief Hardware coverage feedback for AFL++
 * 
 * Provides coverage information from hardware execution back to AFL++
 * to guide fuzzing toward interesting inputs.
 */

#pragma once

#include <cstdint>
#include <cstring>

/**
 * @brief Hardware coverage feedback tracker
 * 
 * Tracks hardware execution state and reports it to AFL++ for
 * coverage-guided fuzzing. Uses edge coverage (PC transitions)
 * as the primary feedback mechanism.
 */
class Feedback {
public:
  Feedback();
  ~Feedback();

  /**
   * @brief Initialize feedback with AFL++ shared memory
   * 
   * Connects to AFL++'s coverage bitmap to report execution feedback.
   * Must be called before any coverage reporting.
   */
  void initialize();

  /**
   * @brief Report instruction execution for coverage tracking
   * 
   * Records PC and instruction for AFL++ coverage feedback.
   * Uses edge coverage (prev_pc -> current_pc) to track control flow.
   * 
   * @param pc Program counter of executed instruction
   * @param insn Instruction word that was executed
   */
  void report_instruction(uint32_t pc, uint32_t insn);

  /**
   * @brief Report memory access for coverage tracking
   * 
   * Records memory access patterns as additional feedback.
   * 
   * @param addr Memory address accessed
   * @param is_write True if write, false if read
   */
  void report_memory_access(uint32_t addr, bool is_write);

  /**
   * @brief Report register write for coverage tracking
   * 
   * Records register modifications as feedback.
   * 
   * @param reg_num Register number (0-31)
   * @param value Value written to register
   */
  void report_register_write(uint32_t reg_num, uint32_t value);

  /**
   * @brief Report Verilator structural coverage to AFL++
   * 
   * Feeds RTL-level coverage metrics (line, toggle, FSM) into AFL++'s
   * coverage bitmap for hardware-guided fuzzing.
   * 
   * @param lines Number of RTL lines executed
   * @param toggles Number of signal toggles
   * @param fsm_states Number of FSM states visited
   */
  void report_verilator_coverage(uint32_t lines, uint32_t toggles, uint32_t fsm_states);

  /**
   * @brief Check if feedback is enabled
   * 
   * @return True if connected to AFL++ shared memory
   */
  bool is_enabled() const { return afl_area_ != nullptr; }

private:
  unsigned char* afl_area_;     ///< AFL++ shared memory bitmap
  uint32_t afl_map_size_;       ///< Size of AFL++ coverage map
  uint32_t prev_pc_;            ///< Previous PC for edge coverage
  
  // Hash function for coverage map indexing
  inline uint32_t hash_edge(uint32_t from, uint32_t to) const {
    return ((from >> 1) ^ to) % afl_map_size_;
  }
  
  inline uint32_t hash_state(uint32_t value) const {
    return (value * 0x5bd1e995) % afl_map_size_;
  }
};
