#pragma once
// LegalCheck.hpp — conservative RV32 legality check
// Returns true if 'insn32' looks like a valid RV32/C encoding (common opcodes).

#include <cstdint>
#include "Instruction.hpp"

// These must be provided by your existing code:
uint32_t opcode32(uint32_t insn);   // insn & 0x7F
uint32_t funct332(uint32_t insn);   // (insn >> 12) & 0x7
uint32_t funct732(uint32_t insn);   // (insn >> 25) & 0x7F
mut::Fmt    get_format(uint32_t insn); // your existing format detector

bool is_legal_instruction(uint32_t insn32);
