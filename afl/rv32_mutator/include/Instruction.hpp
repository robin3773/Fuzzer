
#pragma once
#include <cstdint>
#include <cstddef>

namespace mut {

// Unified formats
enum class Fmt : uint8_t {
  R, I, S, B, U, J, R4, A,      // 32-bit classes
  C16, C_CR, C_CI, C_CSS, C_CIW, C_CL, C_CS, C_CB, C_CJ,
  UNKNOWN
};

// Little-endian IO helpers
// read/write 32-bit values from byte buffers. Assumes little-endian layout.
inline uint32_t get_u32_le(const unsigned char* b, size_t i) {
  return (uint32_t)b[i] | ((uint32_t)b[i+1] << 8) | ((uint32_t)b[i+2] << 16) | ((uint32_t)b[i+3] << 24);
}

inline void put_u32_le(unsigned char* b, size_t i, uint32_t v) {
  b[i]   = v & 0xff;
  b[i+1] = (v >> 8) & 0xff;
  b[i+2] = (v >> 16) & 0xff;
  b[i+3] = (v >> 24) & 0xff;
}

// for 16-bit values
inline uint16_t get_u16_le(const unsigned char* b, size_t i) {
  return (uint16_t)b[i] | ((uint16_t)b[i+1] << 8);
}
inline void put_u16_le(unsigned char* b, size_t i, uint16_t v) {
  b[i]   = v & 0xff;
  b[i+1] = (v >> 8) & 0xff;
}

struct IR32 {
  // Minimal IR for RV32 scalar ops (extend later as needed)
  Fmt       fmt = Fmt::UNKNOWN;
  uint32_t  raw = 0;   // original raw word (for fallback)
  uint8_t   rd = 0, rs1 = 0, rs2 = 0;
  int32_t   imm = 0;   // sign-extended immediate when meaningful
  uint8_t   funct3 = 0;
  uint8_t   funct7 = 0;
  uint8_t   opcode = 0;
};

} // namespace mut
