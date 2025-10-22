
#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

namespace mut {

enum class Strategy : uint8_t {
  RAW = 0,       //only raw-bitfield mutations (fast but brittle )
  IR  = 1,      // decode → mutate IR → re-encode (slower but produces valid instructions).
  HYBRID = 2,   // probabilistically pick between RAW and IR per-instruction.
  AUTO = 3      // Placeholder: adaptive mode stub
};

struct Config {
  // Strategy control 
  Strategy strategy = Strategy::IR;
  uint32_t decode_prob = 20; // percent chance to use IR path in HYBRID

  // ISA knobs
  bool rv32e_mode = false;   // limit regs to x0..x15
  bool enable_c   = true;    // compressed aware edits

  // Weights / behavior
  uint32_t imm_random_prob = 30;  // % use random immediate vs delta
  int32_t  imm_delta_max   = 16;  // +/- range
  uint32_t r_weight_base_alu = 70; // weight for base integer ALU ops vs M-extension when replacing R-type.
  uint32_t r_weight_m        = 30; // weight for M-extension ops  
  uint32_t i_shift_weight    = 30; // % chance to prefer shift-imm opcodes in I-type

  // Logging
  bool verbose = true; // enable verbose logging

  static Strategy parseStrategy(const char* s) {
    if (!s) return Strategy::HYBRID;
    if (!strcasecmp(s, "raw"))    return Strategy::RAW;
    if (!strcasecmp(s, "ir"))     return Strategy::IR;
    if (!strcasecmp(s, "hybrid")) return Strategy::HYBRID;
    if (!strcasecmp(s, "auto"))   return Strategy::AUTO;
    return Strategy::HYBRID;
  }

  void loadFromEnv() {
    const char* s;

    s = std::getenv("RV32_MUT_STRATEGY");
    strategy = parseStrategy(s);

    s = std::getenv("RV32_DECODE_PROB");
    if (s) decode_prob = (uint32_t)std::strtoul(s, nullptr, 10);

    s = std::getenv("RV32E_MODE");
    if (s) rv32e_mode = (std::strcmp(s, "0") != 0);

    s = std::getenv("RV32_C_ENABLE");
    if (s) enable_c = (std::strcmp(s, "0") != 0);

    s = std::getenv("RV32_IMM_RANDOM_PROB");
    if (s) imm_random_prob = (uint32_t)std::strtoul(s, nullptr, 10);

    s = std::getenv("RV32_IMM_DELTA_MAX");
    if (s) imm_delta_max = (int32_t)std::strtol(s, nullptr, 10);

    s = std::getenv("RV32_R_WEIGHT_BASE");
    if (s) r_weight_base_alu = (uint32_t)std::strtoul(s, nullptr, 10);
    s = std::getenv("RV32_R_WEIGHT_M");
    if (s) r_weight_m = (uint32_t)std::strtoul(s, nullptr, 10);

    s = std::getenv("RV32_I_SHIFT_WEIGHT");
    if (s) i_shift_weight = (uint32_t)std::strtoul(s, nullptr, 10);

    s = std::getenv("RV32_VERBOSE");
    if (s) verbose = (std::strcmp(s, "0") != 0);
  }
};

inline uint32_t clamp_pct(int x) {
  if (x < 0) return 0;
  if (x > 100) return 100;
  return (uint32_t)x;
  }

} // namespace mut
