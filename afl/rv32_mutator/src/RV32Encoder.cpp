
#include "RV32Encoder.hpp"

namespace mut {

uint32_t RV32Encoder::encode(const IR32& ir) {
  uint32_t v = ir.raw; // start from original as conservative baseline

  // Re-pack common fields
  v &= ~(((uint32_t)0x1Fu << 7) | ((uint32_t)0x7u << 12) |
         ((uint32_t)0x1Fu << 15) | ((uint32_t)0x1Fu << 20) |
         ((uint32_t)0x7Fu << 25));

  v |= ((uint32_t)(ir.rd & 0x1F)  << 7);
  v |= ((uint32_t)(ir.funct3 & 7) << 12);
  v |= ((uint32_t)(ir.rs1 & 0x1F) << 15);
  v |= ((uint32_t)(ir.rs2 & 0x1F) << 20);
  v |= ((uint32_t)(ir.funct7 & 0x7F) << 25);

  // Re-pack immediates based on fmt
  switch (ir.fmt) {
    case Fmt::I: {
      uint32_t imm = (uint32_t)(ir.imm & 0xFFF);
      v &= ~((uint32_t)0xFFFu << 20);
      v |= (imm << 20);
      break;
    }
    case Fmt::S: {
      uint32_t imm = (uint32_t)(ir.imm & 0xFFF);
      uint32_t hi = (imm >> 5) & 0x7F, lo = imm & 0x1F;
      v &= ~(((uint32_t)0x7Fu << 25) | ((uint32_t)0x1Fu << 7));
      v |= (hi << 25) | (lo << 7);
      break;
    }
    case Fmt::B: {
      uint32_t imm = (uint32_t)(ir.imm & 0x1FFF);
      imm &= ~1u; // even
      uint32_t b31 = (imm >> 12) & 1, b30_25 = (imm >> 5) & 0x3F;
      uint32_t b11_8 = (imm >> 1) & 0xF, b7 = (imm >> 11) & 1;
      v &= ~(((uint32_t)1 << 31) | ((uint32_t)0x3F << 25) | ((uint32_t)0xF << 8) | ((uint32_t)1 << 7));
      v |= (b31 << 31) | (b30_25 << 25) | (b11_8 << 8) | (b7 << 7);
      break;
    }
    case Fmt::U: {
      uint32_t imm = (uint32_t)(ir.imm & 0xFFFFF);
      v &= 0x00000FFFu;
      v |= (imm << 12);
      break;
    }
    case Fmt::J: {
      uint32_t imm = (uint32_t)(ir.imm & 0x1FFFFF);
      imm &= ~1u;
      uint32_t bit20 = (imm >> 20) & 1, bit11 = (imm >> 11) & 1;
      uint32_t bits10_1 = (imm >> 1) & 0x3FF, bits19_12 = (imm >> 12) & 0xFF;
      v &= ~(((uint32_t)1 << 31) | ((uint32_t)0x3FF << 21) | ((uint32_t)1 << 20) | ((uint32_t)0xFF << 12));
      v |= (bit20 << 31) | (bits19_12 << 12) | (bit11 << 20) | (bits10_1 << 21);
      break;
    }
    default:
      break;
  }

  return v;
}

} // namespace mut
