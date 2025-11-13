#pragma once

#include "CpuIface.hpp"
#include "CrashLogger.hpp"
#include "Trace.hpp"
#include <vector>
#include <cstdint>

/// @brief Differential testing checker for DUT vs Golden model
class DifferentialChecker {
public:
  /**
   * @brief Construct a checker instance with clean shadow state
   * 
   * Initializes the internal register file and CSR mirrors used to
   * compare DUT commits against the golden model. Construction does
   * not perform any heavy work; call reset() before first use.
   * 
   * Example:
   * @code
   *   DifferentialChecker diff;
   *   diff.reset();
   *   // Ready to consume DUT/golden commits.
   * @endcode
   */
  DifferentialChecker();

  /**
   * @brief Destructor to clean up shadow memory
   */
  ~DifferentialChecker();

  /**
   * @brief Reset shadow state (register file + CSRs) to zeros
   * 
   * Clears all mirrored architectural state to prepare for a new test
   * case. Call this before processing the first commit of each input.
   * 
   * Example:
   * @code
   *   diff.reset();
   *   while (execute_test_case()) { ... }
   * @endcode
   */
  void reset();

  /**
   * @brief Update DUT shadow register file with latest commit record
   * 
   * Applies a DUT commit (register write, memory info, trap flag) to
   * the internal shadow state, ensuring subsequent comparisons use the
   * most recent architectural state.
   * 
   * @param rec DUT commit record obtained from TraceWriter/CPU
   * 
   * Example:
   * @code
   *   CommitRec dut;
   *   if (cpu->rvfi_valid()) {
   *     populate_commit_from_cpu(cpu, dut);
   *     diff.update_dut_state(dut);
   *   }
   * @endcode
   */
  void update_dut_state(const CommitRec& rec);

  /**
   * @brief Update golden (Spike) shadow register file
   * 
   * Mirrors the latest golden commit so that register/memory deltas can
   * be compared against the DUT. Typically called after Spike provides
   * a matching CommitRec.
   * 
   * @param rec Golden commit record from SpikeProcess
   * 
   * Example:
   * @code
   *   CommitRec golden;
   *   if (golden_model.next_commit(golden)) {
   *     diff.update_golden_state(golden);
   *   }
   * @endcode
   */
  void update_golden_state(const CommitRec& rec);

  /**
   * @brief Mirror DUT CSR writes (mcycle/minstret) from the CPU interface
   * 
   * Uses CpuIface optional CSR accessors to keep local copies of performance
   * counters that are not encoded directly in CommitRec. This allows CSR
   * comparisons when the golden model produces the same information.
   * 
   * @param cpu CPU interface providing rvfi_csr_* helpers
   * 
   * Example:
   * @code
   *   diff.update_dut_csrs(cpu.get());
   * @endcode
   */
  void update_dut_csrs(CpuIface* cpu);

  /**
   * @brief Compare DUT vs golden commit and log crash on divergence
   * 
   * Performs a sequence of checks (PC, register write, memory side effects,
   * and CSR updates). On the first mismatch, emits a detailed crash report
   * via CrashLogger so the fuzzer records the divergent input.
   * 
   * @param dut_rec Latest DUT commit record
   * @param gold_rec Matching golden commit record
   * @param logger Crash logger used to write divergence artifacts
   * @param cyc Current cycle count (for metadata)
   * @param input Input bytes that produced the divergence
   * @return true if a mismatch was detected (crash recorded), false otherwise
   * 
   * Example:
   * @code
   *   if (diff.check_divergence(dut_commit, golden_commit, logger, cycle, input)) {
   *     break; // Crash already written; stop executing this test case.
   *   }
   * @endcode
   */
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

  // Shadow memory for store/load tracking (256KB RAM + 256KB ROM = 512KB total)
  // We track the full address space to catch any store bugs
  static constexpr size_t MEM_SIZE = 512 * 1024;  // 512KB
  static constexpr uint32_t MEM_BASE = 0x80000000;
  uint8_t* dut_mem_;
  uint8_t* gold_mem_;

  /**
   * @brief Detect PC mismatches between DUT and golden commits
   * 
   * Checks both pc_r (fetch) and pc_w (next PC) fields. On mismatch,
   * emits a crash showing the divergent control flow.
   * 
   * @return true if divergence found and logged
   * 
   * Example:
   * @code
   *   if (check_pc_divergence(dut_commit, golden_commit, logger, cyc, input)) {
   *     return true;
   *   }
   * @endcode
   */
  bool check_pc_divergence(const CommitRec& dut, const CommitRec& gold,
                           CrashLogger& logger, unsigned cyc,
                           const std::vector<unsigned char>& input);

  /**
   * @brief Detect register write mismatches between DUT and golden
   * 
   * Compares the destination register index/value recorded in the commits
   * and the internal shadow register files.
   * 
   * @return true if divergence found and logged
   * 
   * Example:
   * @code
   *   if (check_regfile_divergence(dut_commit, golden_commit, logger, cyc, input)) {
   *     return true;
   *   }
   * @endcode
   */
  bool check_regfile_divergence(const CommitRec& dut, const CommitRec& gold,
                                CrashLogger& logger, unsigned cyc,
                                const std::vector<unsigned char>& input);

  /**
   * @brief Detect memory address or mask mismatches
   * 
   * Validates memory address, read/write masks, and associated data between
   * DUT and golden commits, logging on inconsistencies.
   * 
   * @return true if divergence found and logged
   * 
   * Example:
   * @code
   *   if (check_memory_divergence(dut_commit, golden_commit, logger, cyc, input)) {
   *     return true;
   *   }
   * @endcode
   */
  bool check_memory_divergence(const CommitRec& dut, const CommitRec& gold,
                               CrashLogger& logger, unsigned cyc,
                               const std::vector<unsigned char>& input);

  /**
   * @brief Detect CSR value mismatches (mcycle/minstret)
   * 
   * Compares the mirrored CSR state accumulated via update_dut_csrs() and
   * golden commits, logging if counters advance differently.
   * 
   * @return true if divergence found and logged
   * 
   * Example:
   * @code
   *   if (check_csr_divergence(dut_commit, golden_commit, logger, cyc, input)) {
   *     return true;
   *   }
   * @endcode
   */
  bool check_csr_divergence(const CommitRec& dut, const CommitRec& gold,
                            CrashLogger& logger, unsigned cyc,
                            const std::vector<unsigned char>& input);
};
