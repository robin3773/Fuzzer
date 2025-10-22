#pragma once
#include "Instruction.hpp"

namespace mut {

class RV32Encoder {
public:
  static uint32_t encode(const IR32 &ir);
};

} // namespace mut
