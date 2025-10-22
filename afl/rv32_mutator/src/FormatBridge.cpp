// FormatBridge.cpp — exposes get_format() to LegalCheck
#include "Instruction.hpp"

extern "C" Fmt get_format(uint32_t insn) {
    return ::get_format(insn);  // Call original C++ version
}
