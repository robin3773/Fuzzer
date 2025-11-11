#pragma once

#include "SpikeProcess.hpp"
#include "Trace.hpp"
#include <vector>
#include <string>
#include <cstdint>

/// @brief Manages Spike golden model integration for differential testing
class GoldenModel {
public:
  GoldenModel();
  ~GoldenModel();

  /// @brief Initialize golden model from environment configuration
  /// @param input Raw binary input data
  /// @param trace_dir Trace directory for golden.trace output
  /// @return True if golden model is ready for use
  bool initialize(const std::vector<unsigned char>& input, const char* trace_dir);

  /// @brief Check if golden model is active and ready
  bool is_ready() const { return golden_ready_; }

  /// @brief Get next commit from Spike golden model
  /// @param rec Output commit record
  /// @return True if commit was retrieved successfully
  bool next_commit(CommitRec& rec);

  /// @brief Stop the golden model process
  void stop();

  /// @brief Get the Spike process for direct access
  SpikeProcess& spike() { return spike_; }

  /// @brief Write golden trace if enabled
  void write_trace(const CommitRec& rec);

  /// @brief Get temporary ELF path (for debugging)
  const std::string& elf_path() const { return tmp_elf_; }

private:
  SpikeProcess spike_;
  TraceWriter golden_tracer_;
  std::string tmp_elf_;
  bool golden_ready_;
  bool trace_enabled_;
  std::string golden_mode_;
};
