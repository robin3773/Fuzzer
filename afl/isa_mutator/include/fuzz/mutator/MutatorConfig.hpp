/**
 * @file MutatorConfig.hpp
 * @brief Configuration structures and strategy definitions for ISA mutator
 * 
 * Defines the configuration schema loaded from YAML files and the
 * mutation strategy enumeration. Configuration controls mutation behavior,
 * probabilities, weights, and ISA schema selection.
 * 
 * @see loadConfig()
 * @see Strategy
 */

#pragma once

#include <cstdint>
#include <string>

namespace fuzz::mutator {

/**
 * @enum Strategy
 * @brief Mutation strategy selection
 * 
 * Determines how the mutator generates new test cases:
 * - RAW: Simple bit-flipping and byte mutations
 * - IR: Schema-driven instruction-level mutations only
 * - HYBRID: Mix of IR and RAW based on decode_prob
 * - AUTO: Adaptive strategy based on feedback
 */
enum class Strategy : uint8_t { 
  RAW = 0,    ///< Raw bitflip mutations only
  IR = 1,     ///< Schema-driven instruction-level mutations
  HYBRID = 2, ///< Mix of IR and RAW based on probabilities
  AUTO = 3    ///< Adaptive strategy selection
};

/**
 * @struct Config
 * @brief Mutator configuration loaded from YAML file
 * 
 * This structure holds all runtime configuration for the ISA mutator,
 * including mutation strategy, probabilities, and ISA schema settings.
 * Loaded from MUTATOR_CONFIG environment variable path.
 * 
 * @see loadConfig()
 */
struct Config {
  Strategy strategy = Strategy::IR;          ///< Mutation strategy (RAW/IR/HYBRID/AUTO)
  bool verbose = false;                      ///< Enable verbose logging at startup
  uint32_t decode_prob = 60;                 ///< Probability (0-100) of schema-guided mutation in HYBRID mode
  uint32_t imm_random_prob = 25;             ///< Probability (0-100) of random vs delta immediate mutation
  uint32_t r_weight_base_alu = 70;           ///< Weight (0-100) for base ALU instructions
  uint32_t r_weight_m = 30;                  ///< Weight (0-100) for M-extension instructions

  std::string isa_name;                      ///< ISA identifier (e.g., "rv32im")
};

/**
 * @brief Load configuration from MUTATOR_CONFIG environment variable
 * @param show_config If true, logs the loaded configuration (only when DEBUG=1)
 * @return Loaded configuration structure
 * 
 * Reads the MUTATOR_CONFIG environment variable and loads the specified YAML file.
 * User is responsible for setting MUTATOR_CONFIG to a valid file path.
 */
Config loadConfig(bool show_config = true);


} // namespace fuzz::mutator
