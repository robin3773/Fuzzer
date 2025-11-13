/**
 * @file Feedback.cpp
 * @brief Implementation of hardware coverage feedback for AFL++
 */

#include "Feedback.hpp"
#include <hwfuzz/Debug.hpp>
#include <cstdlib>
#include <sys/shm.h>

// AFL++ shared memory environment variable
#define SHM_ENV_VAR "__AFL_SHM_ID"

Feedback::Feedback() 
  : afl_area_(nullptr), afl_map_size_(0), prev_pc_(0) {
}

Feedback::~Feedback() {
  // AFL++ manages the shared memory lifecycle
}

void Feedback::initialize() {
  // Connect to AFL++'s shared memory segment
  const char* shm_id_str = getenv(SHM_ENV_VAR);
  if (!shm_id_str) {
    hwfuzz::debug::logInfo("[FEEDBACK] Running without AFL++ (standalone mode)\n");
    return;
  }

  int shm_id = atoi(shm_id_str);
  afl_area_ = (unsigned char*)shmat(shm_id, nullptr, 0);
  if (afl_area_ == (void*)-1) {
    hwfuzz::debug::logInfo("[FEEDBACK] Failed to attach to AFL++ shared memory\n");
    afl_area_ = nullptr;
    return;
  }

  afl_map_size_ = 65536;  // Standard AFL++ map size (64KB)
  hwfuzz::debug::logInfo("[FEEDBACK] AFL++ bitmap attached, hardware coverage enabled\n");
  
  prev_pc_ = 0;
}

void Feedback::report_instruction(uint32_t pc, uint32_t insn) {
  if (!afl_area_) return;
  
  // Hash PC edge transition (prev_pc -> pc)
  // This tracks control flow through the hardware
  uint32_t edge = ((prev_pc_ >> 1) ^ pc) * 0x9E3779B1;  // Knuth multiplicative hash
  uint32_t idx = (edge >> 16) & 0xFFFF;  // Map to 16-bit index (0-65535)
  
  // Update AFL++ bitmap with hit count
  // AFL++ uses hit counts (not just binary coverage) for better fuzzing
  if (afl_area_[idx] < 255) {
    afl_area_[idx]++;
  }
  
  prev_pc_ = pc;
}

void Feedback::report_memory_access(uint32_t addr, bool is_write) {
  if (!afl_area_) return;
  
  // Hash memory access pattern
  // Different hash for reads vs writes to distinguish access types
  uint32_t access_hash = (addr ^ (is_write ? 0xAAAAAAAA : 0x55555555)) * 0x9E3779B1;
  uint32_t idx = (access_hash >> 16) & 0xFFFF;
  
  if (afl_area_[idx] < 255) {
    afl_area_[idx]++;
  }
}

void Feedback::report_register_write(uint32_t reg_num, uint32_t value) {
  if (!afl_area_) return;
  
  // Hash register write pattern
  // Combines register number with value to track data flow
  uint32_t write_hash = ((reg_num << 24) ^ value) * 0x9E3779B1;
  uint32_t idx = (write_hash >> 16) & 0xFFFF;
  
  if (afl_area_[idx] < 255) {
    afl_area_[idx]++;
  }
}

void Feedback::report_verilator_coverage(uint32_t lines, uint32_t toggles, uint32_t fsm_states) {
  if (!afl_area_) return;
  
  // Feed Verilator structural coverage to AFL++
  // Each metric gets mapped to a different part of the bitmap
  
  // Line coverage: Which RTL lines were executed
  uint32_t line_idx = (lines * 0x9E3779B1) & 0xFFFF;
  if (afl_area_[line_idx] < 255) afl_area_[line_idx]++;
  
  // Toggle coverage: Which signals changed state
  uint32_t toggle_idx = (toggles * 0xDEADBEEF) & 0xFFFF;
  if (afl_area_[toggle_idx] < 255) afl_area_[toggle_idx]++;
  
  // FSM coverage: Which state machine states were visited
  uint32_t fsm_idx = (fsm_states * 0xCAFEBABE) & 0xFFFF;
  if (afl_area_[fsm_idx] < 255) afl_area_[fsm_idx]++;
}
