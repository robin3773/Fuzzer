#include "LegalCheck.hpp"

bool is_legal_instruction(uint32_t insn32) {
  fmt_t f = get_format(insn32);
  uint32_t op = opcode32(insn32);
  uint32_t f3 = funct332(insn32);
  uint32_t f7 = funct732(insn32);

  if (f == FMT_UNKNOWN) return false;

  switch (f) {
    case FMT_R:
      if (op == 0x33u) {
        // RV32I OP: allow base ALU (funct7=0 or 0x20) and M-ext (funct7=0x01)
        if (f7 == 0x00u || f7 == 0x20u || f7 == 0x01u) return true;
        return false;
      }
      // FP/Vector/etc: accept conservatively
      return true;

    case FMT_I:
      if (op == 0x13u) {
        // SLLI: bit30 must be 0 on RV32; SRLI/SRAI: bit30 selects arithmetic
        if (f3 == 0x1u) {
          if (((insn32 >> 30) & 1u) != 0u) return false;
          return true;
        }
        return true; // other I-imm ok, loads/sys accepted below too
      }
      return true; // accept loads/JALR/FENCE/SYSTEM

    case FMT_S:
      if (op == 0x23u) {
        // SB/SH/SW
        if (f3 <= 0x2u) return true;
        return false;
      }
      return true;

    case FMT_B:
      if (op == 0x63u) {
        // BEQ,BNE,BLT,BGE,BLTU,BGEU
        if (f3 == 0x0u || f3 == 0x1u || f3 == 0x4u || f3 == 0x5u || f3 == 0x6u || f3 == 0x7u) return true;
        return false;
      }
      return true;

    case FMT_U:
      return (op == 0x37u || op == 0x17u); // LUI/AUIPC only

    case FMT_J:
      return (op == 0x6Fu); // JAL

    // Compressed parsed subformats: consider legal if recognized
    case FMT_C16:
    case FMT_C_CI:
    case FMT_C_CR:
    case FMT_C_CSS:
    case FMT_C_CIW:
    case FMT_C_CL:
    case FMT_C_CS:
    case FMT_C_CB:
    case FMT_C_CJ:
      return true;

    case FMT_R4: // FMADD/FMSUB/FNMSUB/FNMADD
    case FMT_A:  // atomics
      return true;

    default:
      return false;
  }
}
