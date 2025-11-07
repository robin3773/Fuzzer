#pragma once

#include <cstdint>
#include <string>

namespace fuzz::mutator {

/// Mutation strategy selection
enum class Strategy : uint8_t { 
  RAW = 0,    ///< Raw bitflip mutations only
  IR = 1,     ///< Schema-driven instruction-level mutations
  HYBRID = 2, ///< Mix of IR and RAW based on probabilities
  AUTO = 3    ///< Adaptive strategy selection
};

/**
 * @brief Clamp integer value to percentage range [0, 100]
 * @param x Input value to clamp
 * @return Clamped value as uint32_t in range [0, 100]
 */
inline uint32_t clamp_pct(int x) {
  if (x < 0)
    return 0;
  if (x > 100)
    return 100;
  return static_cast<uint32_t>(x);
}

/**
 * @brief Mutator configuration loaded from YAML file
 * 
 * This structure holds all runtime configuration for the ISA mutator,
 * including mutation strategy, probabilities, and ISA schema settings.
 */
struct Config {
  Strategy strategy = Strategy::IR;          ///< Mutation strategy (RAW/IR/HYBRID/AUTO)
  bool verbose = false;                      ///< Enable verbose logging
  bool enable_c = true;                      ///< Enable compressed (RVC) instruction mutations
  uint32_t decode_prob = 60;                 ///< Probability (0-100) of schema-guided mutation in HYBRID mode
  uint32_t imm_random_prob = 25;             ///< Probability (0-100) of random vs delta immediate mutation
  uint32_t r_weight_base_alu = 70;           ///< Weight (0-100) for base ALU instructions
  uint32_t r_weight_m = 30;                  ///< Weight (0-100) for M-extension instructions

  std::string isa_name;                      ///< ISA identifier (e.g., "rv32im")

  /**
   * @brief Load configuration from YAML file
   * @param path Absolute or relative path to YAML config file
   * @return true if loaded successfully, false if file not found or invalid
   * @throws std::runtime_error if YAML parsing fails
   */
  bool loadFromFile(const std::string &path);
};

/**
 * @brief Load configuration from MUTATOR_CONFIG environment variable
 * @return Loaded configuration structure
 * @throws std::exit(1) if MUTATOR_CONFIG not set or file cannot be loaded
 * 
 * This function reads the MUTATOR_CONFIG environment variable, loads the
 * specified YAML file, and applies any environment variable overrides.
 */
Config loadConfigFromEnv();

/**
 * @brief Print configuration to harness log
 * @param cfg Configuration to display
 * 
 * Outputs configuration in format:
 * [INFO] strategy=IR verbose=true enable_c=false decode_prob=60 ...
 */
void showConfig(const Config &cfg);


} // namespace fuzz::mutator
