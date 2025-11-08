/**
 * @file Random.hpp
 * @brief Fast xorshift-based random number generator for mutations
 * 
 * Provides a lightweight, deterministic RNG suitable for fuzzing workloads.
 * Uses xorshift algorithm for speed and repeatability.
 * 
 * @note This RNG is NOT cryptographically secure - designed for fuzzing only
 */

#pragma once

#include <cstdint>
#include <ctime>

namespace fuzz::mutator {

/**
 * @class Random
 * @brief Static xorshift PRNG for mutation operations
 * 
 * @details
 * Thread-safe static interface for generating random numbers.
 * Seed once at initialization, then call rnd32() or convenience methods.
 * 
 * Algorithm: xorshift32 (period: 2^32-1)
 * 
 * @code
 * Random::seed(12345);
 * uint32_t val = Random::rnd32();
 * uint32_t index = Random::range(array_size);
 * if (Random::chancePct(25)) { ... }  // 25% probability
 * @endcode
 */
class Random {
  static uint32_t state_;  ///< Internal PRNG state

public:
  /**
   * @brief Seed the random number generator
   * @param s Seed value (0 = use current time)
   */
  static inline void seed(uint32_t s) {
    state_ = s ? s : static_cast<uint32_t>(time(nullptr));
  }

  /**
   * @brief Generate a random 32-bit unsigned integer
   * @return Random value in range [0, 2^32-1]
   * 
   * Uses xorshift32 algorithm for speed and simplicity.
   */
  static inline uint32_t rnd32() {
    uint32_t x = state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state_ = x;
    return x;
  }

  /**
   * @brief Generate random number in range [0, n)
   * @param n Upper bound (exclusive), must be > 0
   * @return Random value in [0, n-1], or 0 if n=0
   */
  static inline uint32_t range(uint32_t n) { return n ? (rnd32() % n) : 0; }

  /**
   * @brief Random boolean with specified probability
   * @param pct Probability percentage [0-100]
   * @return true with pct% probability
   * 
   * @code
   * if (Random::chancePct(75)) {  // 75% chance
   *   applyMutation();
   * }
   * @endcode
   */
  static inline bool chancePct(uint32_t pct) { return (rnd32() % 100u) < pct; }
};

} // namespace fuzz::mutator
