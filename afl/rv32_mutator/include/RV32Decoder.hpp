
#pragma once
#include "Instruction.hpp"

namespace mut {

class RV32Decoder {
public:
  // Inspect the 32-bit instruction word (or the low 16 bits for compressed) 
  // and return the Fmt classification (R, I, S, B, U, J, compressed forms, or UNKNOWN)
  static Fmt getFormat(uint32_t insn32); //LINK - definition in RV32Decoder.cpp
  // decode(uint32_t insn32): Returns an IR32 that contains decoded rd, rs1, rs2, funct3, 
  // funct7, opcode, and a sign-extended imm if the format has an immediate. For compressed 
  // instructions it returns coarse Fmt values (C_*), but the IR for compressed is minimal 
  // because of their bit scatter — the compressed mutator handles them separately.
  static IR32 decode(uint32_t insn32); //LINK - definition in RV32Decoder.cpp
};

} // namespace mut
