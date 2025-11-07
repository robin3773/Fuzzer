#pragma once

#include <cstdint>

namespace fuzz::mutator {

// =============================================================================
// RV32I Exit Stub Encoder
// =============================================================================
//
// Provides encoding functions for the 5-instruction exit stub sequence that
// the mutator appends to every fuzzed program.
//
// Exit stub sequence:
//   lui  t0, hi20(TOHOST_ADDR)
//   addi t0, t0, lo12(TOHOST_ADDR)
//   addi t1, x0, 1
//   sw   t1, 0(t0)
//   ebreak
//
// Usage:
//   After mutation completes, call append_exit_stub() to add the sequence.
//
// =============================================================================

namespace exit_stub {

// Magic MMIO address for clean exit signaling (must match harness config)
constexpr uint32_t TOHOST_ADDR = 0x80001000;

// Number of 32-bit instructions in exit stub
constexpr size_t EXIT_STUB_INSN_COUNT = 5;

// RV32I instruction encoders
namespace rv32i {

/// Encode LUI (Load Upper Immediate)
/// Opcode: 0b0110111, Format: U-type
inline uint32_t encode_lui(uint32_t rd, uint32_t upper20) {
  return (upper20 << 12) | (rd << 7) | 0x37u;
}

/// Encode ADDI (Add Immediate)
/// Opcode: 0b0010011, funct3: 0b000, Format: I-type
inline uint32_t encode_addi(uint32_t rd, uint32_t rs1, int32_t imm12) {
  uint32_t uimm = static_cast<uint32_t>(imm12) & 0xFFFu;
  return (uimm << 20) | (rs1 << 15) | (0x0u << 12) | (rd << 7) | 0x13u;
}

/// Encode SW (Store Word)
/// Opcode: 0b0100011, funct3: 0b010, Format: S-type
inline uint32_t encode_sw(uint32_t rs2, uint32_t rs1, int32_t imm12) {
  uint32_t uimm = static_cast<uint32_t>(imm12) & 0xFFFu;
  uint32_t imm_lo = uimm & 0x1Fu;
  uint32_t imm_hi = (uimm >> 5) & 0x7Fu;
  return (imm_hi << 25) | (rs2 << 20) | (rs1 << 15) | (0x2u << 12) | (imm_lo << 7) | 0x23u;
}

/// EBREAK instruction (fixed encoding)
constexpr uint32_t EBREAK = 0x00100073u;

} // namespace rv32i

/// Split 32-bit address into LUI upper20 and ADDI lower12 (sign-correct)
inline void split_address(uint32_t addr, uint32_t& hi20, int32_t& lo12) {
  // Round up if bit[11] is set (to account for sign-extension in ADDI)
  hi20 = (addr + 0x800u) >> 12;
  
  // Calculate signed offset
  int64_t base = static_cast<int64_t>(hi20) << 12;
  lo12 = static_cast<int32_t>(static_cast<int64_t>(addr) - base);
}

/// Append 5-instruction exit stub to buffer
/// @param buf Output buffer (must have space for 5 words = 20 bytes)
/// @param offset Byte offset where stub should be written
/// @param tohost_addr Magic MMIO address (default: TOHOST_ADDR)
inline void append_exit_stub(unsigned char* buf, size_t offset, uint32_t tohost_addr = TOHOST_ADDR) {
  // Split address for LUI + ADDI materialization
  uint32_t hi20;
  int32_t lo12;
  split_address(tohost_addr, hi20, lo12);
  
  // Register allocation: t0 = x5, t1 = x6
  constexpr uint32_t T0 = 5;
  constexpr uint32_t T1 = 6;
  
  // Generate 5-instruction sequence
  uint32_t stub[EXIT_STUB_INSN_COUNT] = {
    rv32i::encode_lui(T0, hi20),         // lui  t0, hi20(TOHOST_ADDR)
    rv32i::encode_addi(T0, T0, lo12),    // addi t0, t0, lo12(TOHOST_ADDR)
    rv32i::encode_addi(T1, 0, 1),        // addi t1, x0, 1
    rv32i::encode_sw(T1, T0, 0),         // sw   t1, 0(t0)
    rv32i::EBREAK                        // ebreak
  };
  
  // Write words in little-endian order
  for (size_t i = 0; i < EXIT_STUB_INSN_COUNT; ++i) {
    uint32_t word = stub[i];
    buf[offset++] = static_cast<unsigned char>(word & 0xFFu);
    buf[offset++] = static_cast<unsigned char>((word >> 8) & 0xFFu);
    buf[offset++] = static_cast<unsigned char>((word >> 16) & 0xFFu);
    buf[offset++] = static_cast<unsigned char>((word >> 24) & 0xFFu);
  }
}

/// Calculate total size needed for payload + stub
inline size_t total_size_with_stub(size_t payload_bytes) {
  return payload_bytes + (EXIT_STUB_INSN_COUNT * 4);
}

/// Check if index is in the protected tail region
inline bool is_tail_locked(size_t word_index, size_t total_words) {
  return word_index >= (total_words - EXIT_STUB_INSN_COUNT);
}

/// Check if buffer contains exit stub at the given offset
/// @param buf Buffer to check (must contain at least 20 bytes from offset)
/// @param tohost_addr Magic MMIO address (default: TOHOST_ADDR)
/// @return true if the 5-word sequence matches exit stub pattern
inline bool has_exit_stub(const unsigned char* buf, uint32_t tohost_addr = TOHOST_ADDR) {
  // Generate expected exit stub
  uint32_t hi20;
  int32_t lo12;
  split_address(tohost_addr, hi20, lo12);
  
  constexpr uint32_t T0 = 5;
  constexpr uint32_t T1 = 6;
  
  uint32_t expected[EXIT_STUB_INSN_COUNT] = {
    rv32i::encode_lui(T0, hi20),
    rv32i::encode_addi(T0, T0, lo12),
    rv32i::encode_addi(T1, 0, 1),
    rv32i::encode_sw(T1, T0, 0),
    rv32i::EBREAK
  };
  
  // Compare words in little-endian format
  for (size_t i = 0; i < EXIT_STUB_INSN_COUNT; ++i) {
    size_t offset = i * 4;
    uint32_t word = static_cast<uint32_t>(buf[offset]) |
                    (static_cast<uint32_t>(buf[offset + 1]) << 8) |
                    (static_cast<uint32_t>(buf[offset + 2]) << 16) |
                    (static_cast<uint32_t>(buf[offset + 3]) << 24);
    
    if (word != expected[i])
      return false;
  }
  
  return true;
}

} // namespace exit_stub
} // namespace fuzz::mutator
