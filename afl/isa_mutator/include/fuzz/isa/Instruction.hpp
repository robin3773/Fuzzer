#pragma once

#include <cstddef>
#include <cstdint>

namespace fuzz::isa {

enum class Fmt : uint8_t {
  R,
  I,
  S,
  B,
  U,
  J,
  R4,
  A,
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

inline uint16_t get_u16_le(const unsigned char *p, size_t i) {
  return static_cast<uint16_t>(p[i] | (static_cast<uint16_t>(p[i + 1]) << 8));
}

inline void put_u16_le(unsigned char *p, size_t i, uint16_t v) {
  p[i] = static_cast<unsigned char>(v & 0xFFu);
  p[i + 1] = static_cast<unsigned char>((v >> 8) & 0xFFu);
}

inline uint32_t get_u32_le(const unsigned char *p, size_t i) {
  return static_cast<uint32_t>(p[i]) |
         (static_cast<uint32_t>(p[i + 1]) << 8) |
         (static_cast<uint32_t>(p[i + 2]) << 16) |
         (static_cast<uint32_t>(p[i + 3]) << 24);
}

inline void put_u32_le(unsigned char *p, size_t i, uint32_t v) {
  p[i] = static_cast<unsigned char>(v & 0xFFu);
  p[i + 1] = static_cast<unsigned char>((v >> 8) & 0xFFu);
  p[i + 2] = static_cast<unsigned char>((v >> 16) & 0xFFu);
  p[i + 3] = static_cast<unsigned char>((v >> 24) & 0xFFu);
}

struct IR32 {
  Fmt fmt = Fmt::UNKNOWN;
  uint32_t raw = 0;
  uint8_t rd = 0;
  uint8_t rs1 = 0;
  uint8_t rs2 = 0;
  int32_t imm = 0;
  uint8_t funct3 = 0;
  uint8_t funct7 = 0;
  uint8_t opcode = 0;
};

} // namespace fuzz::isa
