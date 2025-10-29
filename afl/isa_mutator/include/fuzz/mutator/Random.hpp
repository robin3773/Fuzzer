#pragma once

#include <cstdint>
#include <ctime>

namespace fuzz::mutator {

class Random {
  static uint32_t state_;

public:
  static inline void seed(uint32_t s) {
    state_ = s ? s : static_cast<uint32_t>(time(nullptr));
  }

  static inline uint32_t rnd32() {
    uint32_t x = state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state_ = x;
    return x;
  }

  static inline uint32_t range(uint32_t n) { return n ? (rnd32() % n) : 0; }

  static inline bool chancePct(uint32_t pct) { return (rnd32() % 100u) < pct; }
};

} // namespace fuzz::mutator
