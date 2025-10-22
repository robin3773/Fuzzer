
#pragma once
#include <cstddef>
#include "Instruction.hpp"
#include "MutatorConfig.hpp"
#include "Random.hpp"

namespace mut {

class CompressedMutator {
public:
  // Mutate a compressed instruction at the given byte index within the buffer
  static void mutateAt(unsigned char* buf, size_t byte_i, size_t buf_size, const Config& cfg); 
};

} // namespace mut
