#pragma once

#include <cstdint>
#include <string>

namespace fuzz::mutator {

enum class Strategy : uint8_t { RAW = 0, IR = 1, HYBRID = 2, AUTO = 3 };

inline uint32_t clamp_pct(int x) {
  if (x < 0)
    return 0;
  if (x > 100)
    return 100;
  return static_cast<uint32_t>(x);
}

struct Config {
  Strategy strategy = Strategy::IR;
  bool verbose = false;
  bool enable_c = true;
  uint32_t decode_prob = 60;
  uint32_t imm_random_prob = 25;
  uint32_t r_weight_base_alu = 70;
  uint32_t r_weight_m = 30;

  std::string isa_name;

  bool loadFromFile(const std::string &path);
  void applyEnvironment();
  void dumpToFile(const std::string &path) const;
};

// Load config from MUTATOR_CONFIG environment variable or exit
Config loadConfigFromEnvOrDie();

} // namespace fuzz::mutator
