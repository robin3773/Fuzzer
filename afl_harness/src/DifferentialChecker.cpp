#include "DifferentialChecker.hpp"
#include <hwfuzz/Debug.hpp>
#include <sstream>
#include <cstring>
#include <cstdlib>

DifferentialChecker::DifferentialChecker() {
  reset();
}

void DifferentialChecker::reset() {
  std::memset(dut_regs_, 0, sizeof(dut_regs_));
  std::memset(gold_regs_, 0, sizeof(gold_regs_));
  dut_mcycle_ = 0;
  gold_mcycle_ = 0;
  dut_minstret_ = 0;
  gold_minstret_ = 0;
}

void DifferentialChecker::update_dut_state(const CommitRec& rec) {
  // Update regfile
  if (rec.rd_addr != 0) {
    dut_regs_[rec.rd_addr] = rec.rd_wdata;
  }
  dut_regs_[0] = 0;  // Enforce x0 == 0 invariant
}

void DifferentialChecker::update_golden_state(const CommitRec& rec) {
  // Update regfile
  if (rec.rd_addr != 0) {
    gold_regs_[rec.rd_addr] = rec.rd_wdata;
  }
  gold_regs_[0] = 0;  // Enforce x0 == 0 invariant

  // Increment golden CSR counters (single-issue assumption)
  gold_minstret_ += 1;
  gold_mcycle_ += 1;
}

void DifferentialChecker::update_dut_csrs(CpuIface* cpu) {
  uint64_t msk, dat;
  msk = cpu->rvfi_csr_mcycle_wmask();
  dat = cpu->rvfi_csr_mcycle_wdata();
  if (msk) dut_mcycle_ = (dut_mcycle_ & ~msk) | (dat & msk);
  
  msk = cpu->rvfi_csr_minstret_wmask();
  dat = cpu->rvfi_csr_minstret_wdata();
  if (msk) dut_minstret_ = (dut_minstret_ & ~msk) | (dat & msk);
}

bool DifferentialChecker::check_divergence(const CommitRec& dut_rec, const CommitRec& gold_rec,
                                           CrashLogger& logger, unsigned cyc,
                                           const std::vector<unsigned char>& input) {
  // Check PC divergence first (fastest check)
  if (check_pc_divergence(dut_rec, gold_rec, logger, cyc, input)) return true;

  // Check regfile divergence
  if (check_regfile_divergence(dut_rec, gold_rec, logger, cyc, input)) return true;

  // Check memory access divergence
  if (check_memory_divergence(dut_rec, gold_rec, logger, cyc, input)) return true;

  // Check CSR divergence
  if (check_csr_divergence(dut_rec, gold_rec, logger, cyc, input)) return true;

  return false;
}

bool DifferentialChecker::check_pc_divergence(const CommitRec& dut, const CommitRec& gold,
                                              CrashLogger& logger, unsigned cyc,
                                              const std::vector<unsigned char>& input) {
  if (dut.pc_w != gold.pc_w) {
    std::ostringstream oss;
    oss << "Golden vs DUT mismatch: pc_mismatch\n";
    oss << "DUT: pc=0x" << std::hex << dut.pc_w << "\n";
    oss << "GOLD: pc=0x" << std::hex << gold.pc_w << "\n";
    logger.writeCrash("golden_divergence_pc", dut.pc_r, dut.insn, cyc, input, oss.str());
    hwfuzz::debug::logError("[CRASH] %s", oss.str().c_str());
    std::abort();
  }
  return false;
}

bool DifferentialChecker::check_regfile_divergence(const CommitRec& dut, const CommitRec& gold,
                                                   CrashLogger& logger, unsigned cyc,
                                                   const std::vector<unsigned char>& input) {
  int first_diff = -1;
  for (int i = 0; i < 32; ++i) {
    if (dut_regs_[i] != gold_regs_[i]) {
      first_diff = i;
      break;
    }
  }

  if (first_diff != -1) {
    std::ostringstream oss;
    oss << "Golden vs DUT mismatch: regfile_mismatch at x" << first_diff << "\n";
    oss << std::hex;
    oss << "PC: dut=0x" << dut.pc_w << " gold=0x" << gold.pc_w << "\n";
    oss << "RD this step: dut x" << std::dec << (int)dut.rd_addr << "=0x" << std::hex << dut.rd_wdata
        << ", gold x" << std::dec << (int)gold.rd_addr << "=0x" << std::hex << gold.rd_wdata << "\n";
    
    // Dump first 8 differing registers
    oss << "Diffs: ";
    int shown = 0;
    for (int i = 0; i < 32 && shown < 8; ++i) {
      if (dut_regs_[i] != gold_regs_[i]) {
        oss << "x" << std::dec << i << "=dut:0x" << std::hex << dut_regs_[i]
            << ",gold:0x" << std::hex << gold_regs_[i] << "; ";
        ++shown;
      }
    }
    oss << "\nRepro: run harness binary with same input file.";
    
    logger.writeCrash("golden_divergence_regfile", dut.pc_r, dut.insn, cyc, input, oss.str());
    hwfuzz::debug::logError("[CRASH] %s", oss.str().c_str());
    std::abort();
  }
  return false;
}

bool DifferentialChecker::check_memory_divergence(const CommitRec& dut, const CommitRec& gold,
                                                  CrashLogger& logger, unsigned cyc,
                                                  const std::vector<unsigned char>& input) {
  bool dut_store = (dut.mem_wmask & 0xF) != 0;
  bool dut_load = (dut.mem_rmask & 0xF) != 0;
  bool gold_store = (gold.mem_is_store != 0);
  bool gold_load = (gold.mem_is_load != 0);

  // Check memory operation type mismatch
  if (dut_store != gold_store || dut_load != gold_load) {
    std::ostringstream oss;
    oss << "Golden vs DUT mismatch: mem_kind\n";
    oss << "DUT: load=" << (dut_load ? "1" : "0") << " store=" << (dut_store ? "1" : "0") 
        << " addr=0x" << std::hex << dut.mem_addr << "\n";
    oss << "GOLD: load=" << (gold_load ? "1" : "0") << " store=" << (gold_store ? "1" : "0") 
        << " addr=0x" << std::hex << gold.mem_addr << "\n";
    logger.writeCrash("golden_divergence_mem_kind", dut.pc_r, dut.insn, cyc, input, oss.str());
    hwfuzz::debug::logError("[CRASH] %s", oss.str().c_str());
    std::abort();
  }

  // Check store address mismatch
  if (dut_store && gold_store && (dut.mem_addr != gold.mem_addr)) {
    std::ostringstream oss;
    oss << "Golden vs DUT mismatch: mem_store_addr\n";
    oss << "DUT: addr=0x" << std::hex << dut.mem_addr << " wmask=0x" << dut.mem_wmask << "\n";
    oss << "GOLD: addr=0x" << std::hex << gold.mem_addr << " data=0x" << gold.mem_wdata << "\n";
    logger.writeCrash("golden_divergence_mem_store_addr", dut.pc_r, dut.insn, cyc, input, oss.str());
    hwfuzz::debug::logError("[CRASH] %s", oss.str().c_str());
    std::abort();
  }

  // Check load address mismatch
  if (dut_load && gold_load && (dut.mem_addr != gold.mem_addr)) {
    std::ostringstream oss;
    oss << "Golden vs DUT mismatch: mem_load_addr\n";
    oss << "DUT: addr=0x" << std::hex << dut.mem_addr << " rmask=0x" << dut.mem_rmask << "\n";
    oss << "GOLD: addr=0x" << std::hex << gold.mem_addr << " data=0x" << gold.mem_rdata << "\n";
    logger.writeCrash("golden_divergence_mem_load_addr", dut.pc_r, dut.insn, cyc, input, oss.str());
    hwfuzz::debug::logError("[CRASH] %s", oss.str().c_str());
    std::abort();
  }

  return false;
}

bool DifferentialChecker::check_csr_divergence(const CommitRec& dut, const CommitRec& gold,
                                               CrashLogger& logger, unsigned cyc,
                                               const std::vector<unsigned char>& input) {
  // Check minstret divergence
  if (dut_minstret_ != gold_minstret_) {
    std::ostringstream oss;
    oss << "Golden vs DUT mismatch: csr_minstret\n";
    oss << "DUT: minstret=" << std::dec << dut_minstret_ << " GOLD: " << gold_minstret_ << "\n";
    logger.writeCrash("golden_divergence_csr_minstret", dut.pc_r, dut.insn, cyc, input, oss.str());
    hwfuzz::debug::logError("[CRASH] %s", oss.str().c_str());
    std::abort();
  }

  // Check mcycle divergence
  if (dut_mcycle_ != gold_mcycle_) {
    std::ostringstream oss;
    oss << "Golden vs DUT mismatch: csr_mcycle\n";
    oss << "DUT: mcycle=" << std::dec << dut_mcycle_ << " GOLD: " << gold_mcycle_ << "\n";
    logger.writeCrash("golden_divergence_csr_mcycle", dut.pc_r, dut.insn, cyc, input, oss.str());
    hwfuzz::debug::logError("[CRASH] %s", oss.str().c_str());
    std::abort();
  }

  return false;
}
