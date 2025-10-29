#include <fuzz/mutator/Random.hpp>

namespace fuzz::mutator {

uint32_t Random::state_ = 123456789u;

} // namespace fuzz::mutator
