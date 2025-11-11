/**
 * @file CrashDetection.cpp
 * @brief Implementation of runtime crash detection checks
 */

#include "CrashDetection.hpp"
#include <sstream>

namespace crash_detection {

bool check_x0_write(const CpuIface* cpu, const CrashLogger& logger, 
                    unsigned cyc, const std::vector<unsigned char>& input) {
  if (!cpu->rvfi_valid()) return false;
  uint32_t rd = cpu->rvfi_rd_addr();
  uint32_t w  = cpu->rvfi_rd_wdata();
  if (rd == 0 && w != 0) {
    logger.writeCrash("x0_write", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  return false;
}

bool check_pc_misaligned(const CpuIface* cpu, const CrashLogger& logger,
                         unsigned cyc, const std::vector<unsigned char>& input) {
  if (!cpu->rvfi_valid()) return false;
  uint32_t pcw = cpu->rvfi_pc_wdata();
  if ((pcw & 0x1) != 0) {
    logger.writeCrash("pc_misaligned", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  return false;
}

bool check_mem_align_load(const CpuIface* cpu, const CrashLogger& logger,
                          unsigned cyc, const std::vector<unsigned char>& input) {
  if (!cpu->rvfi_valid()) return false;
  uint32_t addr = cpu->rvfi_mem_addr();
  uint32_t mask = cpu->rvfi_mem_rmask() & 0xFu;
  if (!mask) return false;

  uint32_t off = addr & 0x3u;
  uint32_t contiguous = 0;
  for (uint32_t i = off; i < 4 && (mask & (1u << i)); ++i) contiguous++;
  bool is_contig = (mask == (0x1u << off)) ||
                   (contiguous == 2 && (mask == (0x3u << off))) ||
                   (contiguous == 4 && mask == 0xFu);
  if (!is_contig) {
    logger.writeCrash("mem_mask_irregular_load", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  if (contiguous >= 2 && (addr & 0x1)) {
    logger.writeCrash("mem_unaligned_load", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  if (contiguous >= 4 && (addr & 0x3)) {
    logger.writeCrash("mem_unaligned_load", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  return false;
}

bool check_mem_align_store(const CpuIface* cpu, const CrashLogger& logger,
                           unsigned cyc, const std::vector<unsigned char>& input) {
  if (!cpu->rvfi_valid()) return false;
  uint32_t addr = cpu->rvfi_mem_addr();
  uint32_t mask = cpu->rvfi_mem_wmask() & 0xFu;
  if (!mask) return false;

  uint32_t off = addr & 0x3u;
  uint32_t contiguous = 0;
  for (uint32_t i = off; i < 4 && (mask & (1u << i)); ++i) contiguous++;
  bool is_contig = (mask == (0x1u << off)) ||
                   (contiguous == 2 && (mask == (0x3u << off))) ||
                   (contiguous == 4 && mask == 0xFu);
  if (!is_contig) {
    logger.writeCrash("mem_mask_irregular_store", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  if (contiguous >= 2 && (addr & 0x1)) {
    logger.writeCrash("mem_unaligned_store", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  if (contiguous >= 4 && (addr & 0x3)) {
    logger.writeCrash("mem_unaligned_store", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  return false;
}

bool check_timeout(unsigned cyc, unsigned max_cycles, const CpuIface* cpu,
                   const CrashLogger& logger, const std::vector<unsigned char>& input) {
  if (cyc >= max_cycles) {
    uint32_t pc   = cpu->rvfi_pc_rdata();
    uint32_t insn = cpu->rvfi_insn();
    logger.writeCrash("timeout", pc, insn, cyc, input);
    return true;
  }
  return false;
}

bool check_pc_stagnation(const CpuIface* cpu, const CrashLogger& logger,
                         unsigned cyc, const std::vector<unsigned char>& input,
                         unsigned stagnation_limit, uint32_t& last_pc,
                         bool& last_pc_valid, unsigned& stagnation_count) {
  if (!cpu->rvfi_valid()) return false;
  if (stagnation_limit == 0) return false;  // Disabled
  
  uint32_t pc_w = cpu->rvfi_pc_wdata();
  
  if (last_pc_valid && pc_w == last_pc) {
    if (++stagnation_count > stagnation_limit) {
      std::ostringstream oss;
      oss << "PC stagnation detected after " << stagnation_count 
          << " commits at PC=0x" << std::hex << pc_w << std::dec << "\n";
      oss << "Last instruction: 0x" << std::hex << cpu->rvfi_insn() << std::dec << "\n";
      logger.writeCrash("pc_stagnation", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), 
                       cyc, input, oss.str());
      return true;
    }
  } else {
    last_pc = pc_w;
    last_pc_valid = true;
    stagnation_count = 0;
  }
  
  return false;
}

bool check_trap(const CpuIface* cpu, const CrashLogger& logger,
                unsigned cyc, const std::vector<unsigned char>& input) {
  if (cpu->trap()) {
    uint32_t pc   = cpu->rvfi_pc_rdata();
    uint32_t insn = cpu->rvfi_insn();
    logger.writeCrash("trap", pc, insn, cyc, input);
    return true;
  }
  return false;
}

} // namespace crash_detection
