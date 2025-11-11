#pragma once

#include "CpuIface.hpp"
#include "CrashLogger.hpp"
#include "Trace.hpp"
#include <vector>
#include <cstdint>

/// @brief Differential testing checker for DUT vs Golden model
class DifferentialChecker {
public:
  DifferentialChecker();

  /// @brief Reset shadow state (regfiles, CSRs)
  void reset();

  /// @brief Update DUT shadow state after a commit
  /// @param rec DUT commit record
  void update_dut_state(const CommitRec& rec);

  /// @brief Update Golden shadow state after a commit
  /// @param rec Golden commit record
  void update_golden_state(const CommitRec& rec);

  /// @brief Update DUT CSR state from RVFI interface
  /// @param cpu CPU interface for reading CSR changes
  void update_dut_csrs(CpuIface* cpu);

  /// @brief Compare DUT vs Golden state and crash if mismatch detected
  /// @param dut_rec DUT commit record
  /// @param gold_rec Golden commit record
  /// @param logger Crash logger for writing divergence reports
  /// @param cyc Current cycle count
  /// @param input Input data for crash reproduction
  /// @return True if divergence detected (will abort)
  bool check_divergence(const CommitRec& dut_rec, const CommitRec& gold_rec,
                        CrashLogger& logger, unsigned cyc,
                        const std::vector<unsigned char>& input);

private:
  // Shadow regfiles for comparison (x0..x31)
  uint32_t dut_regs_[32];
  uint32_t gold_regs_[32];

  // Shadow CSRs (best-effort tracking)
  uint64_t dut_mcycle_;
  uint64_t gold_mcycle_;
  uint64_t dut_minstret_;
  uint64_t gold_minstret_;

  bool check_pc_divergence(const CommitRec& dut, const CommitRec& gold,
                           CrashLogger& logger, unsigned cyc,
                           const std::vector<unsigned char>& input);

  bool check_regfile_divergence(const CommitRec& dut, const CommitRec& gold,
                                CrashLogger& logger, unsigned cyc,
                                const std::vector<unsigned char>& input);

  bool check_memory_divergence(const CommitRec& dut, const CommitRec& gold,
                               CrashLogger& logger, unsigned cyc,
                               const std::vector<unsigned char>& input);

  bool check_csr_divergence(const CommitRec& dut, const CommitRec& gold,
                            CrashLogger& logger, unsigned cyc,
                            const std::vector<unsigned char>& input);
};
