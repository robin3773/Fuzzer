#include "RV32Decoder.hpp"

namespace mut {

static inline uint32_t opcode32(uint32_t insn) { return insn & 0x7f; }
static inline uint32_t rd32(uint32_t insn) { return (insn >> 7) & 0x1f; }
static inline uint32_t funct332(uint32_t insn) { return (insn >> 12) & 0x7; }
static inline uint32_t rs132(uint32_t insn) { return (insn >> 15) & 0x1f; }
static inline uint32_t rs232(uint32_t insn) { return (insn >> 20) & 0x1f; }
static inline uint32_t funct732(uint32_t insn) { return (insn >> 25) & 0x7f; }

Fmt RV32Decoder::getFormat(uint32_t insn32) {
  uint16_t low16 = (uint16_t)(insn32 & 0xFFFFu);
  if ((low16 & 0x3u) != 0x3u) { // compressed 16-bit
    // coarse subformat inference
    uint8_t op_lo = low16 & 0x3u;
    uint8_t funct3 = (low16 >> 13) & 0x7u;
    switch (op_lo) {
    case 0x0: // quadrant 0
      switch (funct3) {
      case 0b010:
        return Fmt::C_CL; // c.lw
      case 0b110:
        return Fmt::C_CS; // c.sw
      default:
        return Fmt::C16;
      }
    case 0x1: // quadrant 1
      switch (funct3) {
      case 0b000:
        return Fmt::C_CI; // c.addi et al.
      case 0b001:
        return Fmt::C_CJ; // c.jal (RV32)
      case 0b101:
        return Fmt::C_CJ; // c.j
      default:
        return Fmt::C16;
      }
    case 0x2: // quadrant 2
      switch (funct3) {
      case 0b010:
        return Fmt::C_CL; // c.lwsp
      case 0b110:
        return Fmt::C_CSS; // c.swsp
      default:
        return Fmt::C16;
      }
    }
    return Fmt::C16;
  }

  uint32_t op = opcode32(insn32);
  switch (op) {
  case 0x33:
    return Fmt::R; // OP
  case 0x13:
    return Fmt::I; // OP-IMM
  case 0x03:
    return Fmt::I; // LOAD
  case 0x23:
    return Fmt::S; // STORE
  case 0x63:
    return Fmt::B; // BRANCH
  case 0x37:
    return Fmt::U; // LUI
  case 0x17:
    return Fmt::U; // AUIPC
  case 0x6F:
    return Fmt::J; // JAL
  case 0x67:
    return Fmt::I; // JALR
  case 0x43:
    return Fmt::R4; // FMADD (placeholder classification)
  case 0x2F:
    return Fmt::A; // AMO
  default:
    return Fmt::UNKNOWN;
  }
}

IR32 RV32Decoder::decode(uint32_t insn32) {
  IR32 ir{};
  ir.raw = insn32;
  ir.opcode = (uint8_t)opcode32(insn32);

  ir.fmt = getFormat(insn32);
  if (ir.fmt == Fmt::C16 || ir.fmt == Fmt::C_CR || ir.fmt == Fmt::C_CI ||
      ir.fmt == Fmt::C_CSS || ir.fmt == Fmt::C_CIW || ir.fmt == Fmt::C_CL ||
      ir.fmt == Fmt::C_CS || ir.fmt == Fmt::C_CB || ir.fmt == Fmt::C_CJ) {
    // minimal IR for compressed; mutate separately
    return ir;
  }

  ir.rd = (uint8_t)rd32(insn32);
  ir.funct3 = (uint8_t)funct332(insn32);
  ir.rs1 = (uint8_t)rs132(insn32);
  ir.rs2 = (uint8_t)rs232(insn32);
  ir.funct7 = (uint8_t)funct732(insn32);

  switch (ir.fmt) {
  case Fmt::I: {
    int32_t imm = (int32_t)(insn32 >> 20) & 0xFFF;
    if (imm & 0x800)
      imm |= ~0xFFF;
    ir.imm = imm;
    break;
  }
  case Fmt::S: {
    int32_t imm = (int32_t)(((insn32 >> 25) << 5) | ((insn32 >> 7) & 0x1F));
    if (imm & 0x800)
      imm |= ~0xFFF;
    ir.imm = imm;
    break;
  }
  case Fmt::B: {
    uint32_t imm = (((insn32 >> 31) & 1) << 12) | (((insn32 >> 7) & 1) << 11) |
                   (((insn32 >> 25) & 0x3F) << 5) |
                   (((insn32 >> 8) & 0xF) << 1);
    int32_t simm = (int32_t)imm;
    if (simm & 0x1000)
      simm |= ~0x1FFF;
    ir.imm = simm;
    break;
  }
  case Fmt::U: {
    int32_t imm = (int32_t)(insn32 & 0xFFFFF000);
    ir.imm = imm;
    break;
  }
  case Fmt::J: {
    uint32_t imm =
        (((insn32 >> 31) & 1) << 20) | (((insn32 >> 21) & 0x3FF) << 1) |
        (((insn32 >> 20) & 1) << 11) | (((insn32 >> 12) & 0xFF) << 12);
    int32_t simm = (int32_t)imm;
    if (simm & 0x100000)
      simm |= ~0x1FFFFF;
    ir.imm = simm;
    break;
  }
  default:
    break;
  }
  return ir;
}

} // namespace mut
