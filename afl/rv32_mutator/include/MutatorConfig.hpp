#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace mut {

enum class Strategy : uint8_t { RAW = 0, IR = 1, HYBRID = 2, AUTO = 3 };

inline uint32_t clamp_pct(int x) {
  if (x < 0)
    return 0;
  if (x > 100)
    return 100;
  return (uint32_t)x;
}

struct Config {
  Strategy strategy = Strategy::IR;
  bool verbose = false;
  bool enable_c = true;            // enable compressed tweaks
  uint32_t decode_prob = 60;       // in HYBRID, % chance to use IR path
  uint32_t imm_random_prob = 25;   // % of random vs delta-based imm change
  uint32_t r_weight_base_alu = 70; // weight split base-ALU vs M-ext
  uint32_t r_weight_m = 30;

  void initFromEnv() {
    const char *s;
    s = std::getenv("RV32_STRATEGY");
    if (s) {
      if (!std::strcmp(s, "RAW"))
        strategy = Strategy::RAW;
      else if (!std::strcmp(s, "IR"))
        strategy = Strategy::IR;
      else if (!std::strcmp(s, "HYBRID"))
        strategy = Strategy::HYBRID;
      else if (!std::strcmp(s, "AUTO"))
        strategy = Strategy::AUTO;
    }
    s = std::getenv("RV32_VERBOSE");
    if (s)
      verbose = (std::strcmp(s, "0") != 0);
    s = std::getenv("RV32_ENABLE_C");
    if (s)
      enable_c = (std::strcmp(s, "0") != 0);
    s = std::getenv("RV32_DECODE_PROB");
    if (s)
      decode_prob = clamp_pct(std::atoi(s));
    s = std::getenv("RV32_IMM_RANDOM");
    if (s)
      imm_random_prob = clamp_pct(std::atoi(s));
    s = std::getenv("RV32_R_BASE");
    if (s)
      r_weight_base_alu = clamp_pct(std::atoi(s));
    s = std::getenv("RV32_R_M");
    if (s)
      r_weight_m = clamp_pct(std::atoi(s));
  }
};

} // namespace mut
