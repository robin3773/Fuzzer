#pragma once

#include <cstdint>

#include <fuzz/isa/Loader.hpp>

namespace fuzz::mutator {

bool is_legal_instruction(uint32_t insn32, const isa::ISAConfig &isa);

} // namespace fuzz::mutator
