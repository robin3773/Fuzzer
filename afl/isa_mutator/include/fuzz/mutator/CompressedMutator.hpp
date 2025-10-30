#pragma once

#include <cstddef>

#include <fuzz/mutator/MutatorConfig.hpp>

namespace fuzz::mutator {

class CompressedMutator {
public:
  static void mutateAt(unsigned char *buf,
                       size_t byte_index,
                       size_t buf_size,
                       const Config &cfg);
};

} // namespace fuzz::mutator
