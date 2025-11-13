#pragma once

#include "SpikeProcess.hpp"
#include "Trace.hpp"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <cstdint>

struct HarnessConfig;

/**
 * @class GoldenModel
 * @brief Manages Spike golden model integration for differential testing
 * 
 * Wraps SpikeProcess and trace utilities to provide an on-demand golden
 * reference during fuzzing. The class owns the temporary ELF generated
 * from fuzzer input, spawns Spike, consumes commits, and emits golden
 * traces when enabled.
 */
class GoldenModel {
public:
  /**
   * @brief Construct a golden model wrapper (process initially stopped)
   * 
   * Construction only prepares member fields; call initialize() with a
   * specific input to build the ELF and launch Spike.
   * 
   * Example:
   * @code
   *   GoldenModel golden;
   *   // Later: golden.initialize(input, cfg);
   * @endcode
   */
  GoldenModel();

  /**
   * @brief Destructor ensures Spike process is terminated and temp files removed
   * 
   * Automatically calls stop(), then unlinks the temporary ELF produced for
   * the previous test case.
   * 
   * Example:
   * @code
   *   {
   *     GoldenModel golden;
   *     golden.initialize(input, cfg);
   *     // Scope exit triggers ~GoldenModel(), stopping Spike automatically.
   *   }
   * @endcode
   */
  ~GoldenModel();

  /**
   * @brief Initialize golden model using harness configuration
   * 
   * Builds a temporary ELF from the provided input, launches Spike with the
   * configured ISA/PK, configures log paths, and opens the golden trace file
   * when tracing is enabled.
   * 
   * @param input Raw binary input data
   * @param cfg Loaded harness configuration (contains Spike paths/settings)
   * @return true if Spike was started successfully; false disables golden mode
   * 
   * Example:
   * @code
   *   if (!golden.initialize(input_bytes, cfg)) {
   *     hwfuzz::debug::logWarn("Golden model disabled for this test case");
   *   }
   * @endcode
   */
  bool initialize(const std::vector<unsigned char>& input, const HarnessConfig& cfg);

  /**
   * @brief Check if golden model is active and ready
   * 
   * Returns true only after initialize() succeeds and before Spike exits.
   * Callers should query this before requesting commits.
   * 
   * Example:
   * @code
   *   if (golden.is_ready()) {
   *     CommitRec ref;
   *     if (golden.next_commit(ref)) { ... }
   *   }
   * @endcode
   */
  bool is_ready() const { return golden_ready_; }

  /**
   * @brief Fetch the next golden commit record from Spike
   * 
   * Blocks until Spike produces another commit or terminates. When Spike
   * stops producing data, this returns false and marks the golden model as
   * inactive (call initialize() again for the next test case).
   * 
   * @param rec Output commit record populated on success
   * @return true if a commit was retrieved, false if Spike stopped
   * 
   * Example:
   * @code
   *   CommitRec golden_rec;
   *   while (golden.next_commit(golden_rec)) {
   *     diff.update_golden_state(golden_rec);
   *   }
   * @endcode
   */
  bool next_commit(CommitRec& rec);

  /**
   * @brief Stop the golden model process and clean up resources
   * 
   * Stops Spike if it is still running and deletes the temporary ELF.
   * Safe to call multiple times.
   * 
   * Example:
   * @code
   *   golden.stop();  // invoked on harness shutdown
   * @endcode
   */
  void stop();

  /**
   * @brief Direct access to the underlying SpikeProcess object
   * 
   * Allows callers to query command lines, log paths, or status strings.
   * Should not be used to manage lifecycle directly (use initialize/stop).
   * 
   * Example:
   * @code
   *   hwfuzz::debug::logInfo("Spike command: %s\n", golden.spike().command().c_str());
   * @endcode
   */
  SpikeProcess& spike() { return spike_; }

  /**
   * @brief Write golden trace entry when tracing is enabled
   * 
   * Appends the commit record to golden.trace. No-op if tracing disabled.
   * 
   * @param rec Commit record to append
   * 
   * Example:
   * @code
   *   if (golden.is_ready()) {
   *     golden.write_trace(commit);
   *   }
   * @endcode
   */
  void write_trace(const CommitRec& rec);

  /**
   * @brief Get the path to the temporary ELF generated for Spike
   * 
   * Useful when debugging golden model issues; path becomes empty after stop().
   * 
   * Example:
   * @code
   *   hwfuzz::debug::logInfo("Golden ELF: %s\n", golden.elf_path().c_str());
   * @endcode
   */
  const std::string& elf_path() const { return tmp_elf_; }

private:
  SpikeProcess spike_;
  TraceWriter golden_tracer_;
  std::string tmp_elf_;
  bool golden_ready_;
  bool trace_enabled_;
  std::string golden_mode_;
  std::string spike_log_path_;
};
