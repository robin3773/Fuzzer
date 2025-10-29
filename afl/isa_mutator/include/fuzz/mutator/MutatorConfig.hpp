#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
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

  std::string isa_name = "riscv32_base";
  std::string schema_dir;
  std::string schema_override;

  void initFromEnv() {
    const char *s = std::getenv("RV32_STRATEGY");
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

    s = std::getenv("MUTATOR_ISA");
    if (s && *s)
      isa_name = s;

    s = std::getenv("MUTATOR_DIR");
    if (s && *s)
      schema_dir = s;

    s = std::getenv("MUTATOR_SCHEMA");
    if (s && *s)
      schema_override = s;

    s = std::getenv("MUTATOR_SCHEMAS");
    if (s && *s)
      schema_dir = s;
  }
};

} // namespace fuzz::mutator
