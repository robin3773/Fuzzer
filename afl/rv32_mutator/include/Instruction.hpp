#pragma once
#include <cstddef>
#include <cstdint>

namespace mut {

// Unified formats
enum class Fmt : uint8_t {
  R,
  I,
  S,
  B,
  U,
  J,
  R4,
  A, // 32-bit classes
  C16,
  C_CR,
  C_CI,
  C_CSS,
  C_CIW,
  C_CL,
  C_CS,
  C_CB,
  C_CJ,
  UNKNOWN
};

// Little-endian IO helpers
static inline uint16_t get_u16_le(const unsigned char *p, size_t i) {
  return (uint16_t)(p[i] | ((uint16_t)p[i + 1] << 8));
}
static inline void put_u16_le(unsigned char *p, size_t i, uint16_t v) {
  p[i] = (unsigned char)(v & 0xFFu);
  p[i + 1] = (unsigned char)((v >> 8) & 0xFFu);
}
static inline uint32_t get_u32_le(const unsigned char *p, size_t i) {
  return (uint32_t)p[i] | ((uint32_t)p[i + 1] << 8) |
         ((uint32_t)p[i + 2] << 16) | ((uint32_t)p[i + 3] << 24);
}
static inline void put_u32_le(unsigned char *p, size_t i, uint32_t v) {
  p[i] = (unsigned char)(v & 0xFFu);
  p[i + 1] = (unsigned char)((v >> 8) & 0xFFu);
  p[i + 2] = (unsigned char)((v >> 16) & 0xFFu);
  p[i + 3] = (unsigned char)((v >> 24) & 0xFFu);
}

// Minimal IR for RV32 scalar ops
struct IR32 {
  Fmt fmt = Fmt::UNKNOWN;
  uint32_t raw = 0; // original raw word (for fallback)
  uint8_t rd = 0, rs1 = 0, rs2 = 0;
  int32_t imm = 0; // sign-extended immediate when meaningful
  uint8_t funct3 = 0;
  uint8_t funct7 = 0;
  uint8_t opcode = 0;
};

} // namespace mut
