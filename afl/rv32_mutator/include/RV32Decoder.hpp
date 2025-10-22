#pragma once
#include "Instruction.hpp"

namespace mut {

class RV32Decoder {
public:
  static Fmt getFormat(uint32_t insn32);
  static IR32 decode(uint32_t insn32);
};

} // namespace mut
