#pragma once
// Conservative RV32 legality check
#include "Instruction.hpp"
#include "RV32Decoder.hpp"
#include <cstdint>

namespace mut {

bool is_legal_instruction(uint32_t insn32);

} // namespace mut
