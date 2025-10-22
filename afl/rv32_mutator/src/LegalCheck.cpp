#include "LegalCheck.hpp"

namespace mut {

static inline uint32_t opcode32(uint32_t insn) { return insn & 0x7f; }
static inline uint32_t funct332(uint32_t insn) { return (insn >> 12) & 0x7; }
static inline uint32_t funct732(uint32_t insn) { return (insn >> 25) & 0x7f; }

bool is_legal_instruction(uint32_t insn32) {
  Fmt f = RV32Decoder::getFormat(insn32);
  uint32_t op = opcode32(insn32);
  uint32_t f3 = funct332(insn32);
  uint32_t f7 = funct732(insn32);

  if (f == Fmt::UNKNOWN)
    return false;

  switch (f) {
  case Fmt::R:
    if (op == 0x33u) {
      // RV32I OP: allow base ALU (funct7=0 or 0x20) and M-ext (funct7=0x01)
      if (f7 == 0x00u || f7 == 0x20u || f7 == 0x01u)
        return true;
      return false;
    }
    return true;

  case Fmt::I:
    // OP-IMM, LOAD, JALR: basic funct3 range check
    if (op == 0x13u || op == 0x03u || op == 0x67u)
      return (f3 <= 7u);
    return true;

  case Fmt::S:
    // STORE: funct3 000/001/010 for SB/SH/SW (permissive)
    return (op == 0x23u) ? (f3 <= 2u) : true;

  case Fmt::B:
    // BRANCH: funct3 in {000,001,100,101,110,111}
    return (op == 0x63u) ? (f3 <= 7u) : true;

  case Fmt::U:
  case Fmt::J:
    return true;

  // Compressed formats: allow as-is (they were detected as compressed already)
  case Fmt::C16:
  case Fmt::C_CR:
  case Fmt::C_CI:
  case Fmt::C_CSS:
  case Fmt::C_CIW:
  case Fmt::C_CL:
  case Fmt::C_CS:
  case Fmt::C_CB:
  case Fmt::C_CJ:
  case Fmt::R4: // FMADD/FMSUB/FNMSUB/FNMADD (placeholder)
  case Fmt::A:  // atomics (placeholder)
    return true;

  default:
    return false;
  }
}

} // namespace mut
