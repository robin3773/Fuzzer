/**
 * @file VerilatorCoverage.hpp
 * @brief Integration of Verilator structural coverage with AFL++
 * 
 * Provides hardware-specific coverage metrics:
 * - Line coverage: RTL lines executed
 * - Toggle coverage: Signal bit transitions (0->1, 1->0)
 * - FSM coverage: State machine state transitions
 * - Trace coverage: Waveform trace data (when tracing enabled)
 * 
 * These metrics complement AFL++'s control-flow coverage to guide
 * fuzzing towards hardware corner cases.
 */

#pragma once

#include <cstdint>
#include <string>
#include <memory>

// Forward declaration - Feedback is in global namespace
class Feedback;

namespace fuzz {

/**
 * @brief Verilator structural coverage feedback for AFL++
 * 
 * Integrates Verilator's coverage data (line, toggle, FSM) to provide
 * AFL++ with hardware-specific feedback that guides input generation
 * towards better RTL coverage.
 */
class VerilatorCoverage {
public:
  VerilatorCoverage();
  ~VerilatorCoverage();

  /**
   * @brief Initialize coverage tracking
   * @param coverage_file Path to Verilator coverage output file
   * @return true if coverage enabled, false if disabled/unavailable
   */
  bool initialize(const std::string& coverage_file);

  /**
   * @brief Report coverage metrics to AFL++ after each test case
   * 
   * Extracts coverage deltas and maps them to AFL++ bitmap to guide
   * fuzzing. This makes AFL++ aware of:
   * - New RTL lines executed
   * - New signal toggles
   * - New FSM states reached
   * - New trace events captured
   * 
   * @param feedback Reference to Feedback object for AFL++ integration
   */
  void report_to_afl(Feedback& feedback);

  /**
   * @brief Write coverage data to file and reset for next iteration
   * 
   * Called at the end of each fuzzing iteration to:
   * 1. Flush Verilator coverage to disk
   * 2. Parse coverage deltas
   * 3. Update AFL++ feedback bitmap
   * 4. Reset for next test case
   */
  void write_and_reset();

  /**
   * @brief Get current line coverage percentage
   * @return Percentage of RTL lines executed (0-100)
   */
  double get_line_coverage() const;

  /**
   * @brief Get current toggle coverage percentage
   * @return Percentage of signal bits toggled (0-100)
   */
  double get_toggle_coverage() const;

  /**
   * @brief Get current FSM coverage percentage
   * @return Percentage of FSM states reached (0-100)
   */
  double get_fsm_coverage() const;

  /**
   * @brief Get current trace coverage metric
   * @return Number of unique trace events captured
   */
  uint32_t get_trace_coverage() const;

  /**
   * @brief Check if coverage tracking is active
   */
  bool is_enabled() const { return enabled_; }

private:
  bool enabled_;
  std::string coverage_file_;
  uint32_t prev_line_count_;
  uint32_t prev_toggle_count_;
  uint32_t prev_fsm_count_;
  uint32_t prev_trace_count_;
  
  /**
   * @brief Parse Verilator coverage data file
   * @param new_lines Output: number of new lines covered
   * @param new_toggles Output: number of new toggles
   * @param new_fsm Output: number of new FSM transitions
   * @param new_trace Output: number of new trace events
   */
  void parse_coverage_deltas(uint32_t& new_lines, uint32_t& new_toggles, 
                             uint32_t& new_fsm, uint32_t& new_trace);

  /**
   * @brief Hash coverage metric into AFL++ bitmap index
   */
  uint32_t hash_coverage_metric(uint32_t metric) const;
};

} // namespace fuzz
