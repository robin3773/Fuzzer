#pragma once
#include "Instruction.hpp"
#include "MutatorConfig.hpp"
#include "Random.hpp"
#include <cstddef>

namespace mut {

class CompressedMutator {
public:
  static void mutateAt(unsigned char *buf, size_t byte_i, size_t buf_size,
                       const Config &cfg);
};

} // namespace mut
