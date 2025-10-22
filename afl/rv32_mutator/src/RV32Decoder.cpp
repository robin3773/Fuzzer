
#include "RV32Decoder.hpp"

namespace mut {

static inline uint32_t opcode32(uint32_t insn) { return insn & 0x7f; }
static inline uint32_t rd32(uint32_t insn)     { return (insn >> 7) & 0x1f; }
static inline uint32_t funct332(uint32_t insn) { return (insn >> 12) & 0x7; }
static inline uint32_t rs132(uint32_t insn)    { return (insn >> 15) & 0x1f; }
static inline uint32_t rs232(uint32_t insn)    { return (insn >> 20) & 0x1f; }
static inline uint32_t funct732(uint32_t insn) { return (insn >> 25) & 0x7f; }

Fmt RV32Decoder::getFormat(uint32_t insn32) {
  uint16_t low16 = (uint16_t)(insn32 & 0xFFFFu);
  if ((low16 & 0x3u) != 0x3u) { // compressed 16-bit
    // coarse subformat inference
    uint8_t op_lo = low16 & 0x3u;
    uint8_t funct3 = (uint8_t)((low16 >> 13) & 0x7u);
    switch (op_lo) {
      case 0x0:
        switch (funct3) {
          case 0b000: return Fmt::C_CIW;
          case 0b010: return Fmt::C_CL;
          case 0b110: return Fmt::C_CS;
          default:    return Fmt::C16;
        }
      case 0x1:
        switch (funct3) {
          case 0b000: case 0b010: case 0b011: return Fmt::C_CI;
          case 0b100: case 0b110: case 0b111: return Fmt::C_CB;
          case 0b001: case 0b101:              return Fmt::C_CJ;
          default: return Fmt::C16;
        }
      case 0x2:
        switch (funct3) {
          case 0b000: return Fmt::C_CI;   // slli
          case 0b010: return Fmt::C_CL;   // lwsp
          case 0b100: return Fmt::C_CR;   // mv/add/jr/jalr/ebreak
          case 0b110: return Fmt::C_CSS;  // swsp
          default:    return Fmt::C16;
        }
      default: return Fmt::C16;
    }
  }

  uint32_t op = opcode32(insn32);
  switch (op) {
    case 0x33: return Fmt::R; // OP
    case 0x13: return Fmt::I; // OP-IMM
    case 0x03: return Fmt::I; // LOAD
    case 0x23: return Fmt::S; // STORE
    case 0x63: return Fmt::B; // BRANCH
    case 0x37: return Fmt::U; // LUI
    case 0x17: return Fmt::U; // AUIPC
    case 0x6F: return Fmt::J; // JAL
    case 0x67: return Fmt::I; // JALR
    case 0x0F: return Fmt::I; // FENCE
    case 0x73: return Fmt::I; // SYSTEM/CSR
    case 0x2F: return Fmt::A; // ATOMIC

    // FP:
    case 0x07: return Fmt::I; // FLW/FLD
    case 0x27: return Fmt::S; // FSW/FSD
    case 0x53: return Fmt::R; // FP arith
    case 0x43: case 0x47: case 0x4B: case 0x4F: return Fmt::R4; // fused

    // RV64 slots / vectors treated as R for safety
    case 0x1B: return Fmt::I;
    case 0x3B: return Fmt::R;
    case 0x57: return Fmt::R;

    default: return Fmt::UNKNOWN;
  }
}

IR32 RV32Decoder::decode(uint32_t insn32) {
  IR32 ir;
  ir.raw    = insn32;
  ir.opcode = (uint8_t)(insn32 & 0x7f);
  ir.fmt    = getFormat(insn32);
  ir.rd     = (uint8_t)((insn32 >> 7) & 0x1f);
  ir.funct3 = (uint8_t)((insn32 >> 12) & 0x7);
  ir.rs1    = (uint8_t)((insn32 >> 15) & 0x1f);
  ir.rs2    = (uint8_t)((insn32 >> 20) & 0x1f);
  ir.funct7 = (uint8_t)((insn32 >> 25) & 0x7f);
  ir.imm    = 0;

  // Immediates
  switch (ir.fmt) {
    case Fmt::I: {
      int32_t imm = (int32_t)((insn32 >> 20) & 0xFFFu);
      if (imm & 0x800) imm |= ~0xFFF;
      ir.imm = imm;
      break;
    }
    case Fmt::S: {
      int32_t imm = (int32_t)(((insn32 >> 25) << 5) | ((insn32 >> 7) & 0x1F));
      if (imm & 0x800) imm |= ~0xFFF;
      ir.imm = imm;
      break;
    }
    case Fmt::B: {
      uint32_t imm = (((insn32 >> 31) & 1) << 12) |
                     (((insn32 >> 25) & 0x3F) << 5) |
                     (((insn32 >> 8)  & 0xF)  << 1) |
                     (((insn32 >> 7)  & 1)    << 11);
      int32_t simm = (int32_t)imm;
      if (simm & 0x1000) simm |= ~0x1FFF;
      ir.imm = simm;
      break;
    }
    case Fmt::U: {
      int32_t imm = (int32_t)((insn32 >> 12) & 0xFFFFF);
      if (imm & 0x80000) imm |= ~0xFFFFF;
      ir.imm = imm;
      break;
    }
    case Fmt::J: {
      uint32_t imm = (((insn32 >> 31) & 1) << 20) |
                     (((insn32 >> 21) & 0x3FF) << 1) |
                     (((insn32 >> 20) & 1) << 11) |
                     (((insn32 >> 12) & 0xFF) << 12);
      int32_t simm = (int32_t)imm;
      if (simm & 0x100000) simm |= ~0x1FFFFF;
      ir.imm = simm;
      break;
    }
    default:
      break;
  }

  return ir;
}

} // namespace mut
