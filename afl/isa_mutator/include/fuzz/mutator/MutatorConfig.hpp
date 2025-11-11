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
 * - BYTE_LEVEL: Simple bit-flipping and byte mutations (legacy - not recommended)
 * - INSTRUCTION_LEVEL: Schema-driven instruction-level mutations (recommended)
 * - MIXED_MODE: Combination of instruction-level and byte-level based on decode_prob
 * - ADAPTIVE: Dynamically selects strategy based on fuzzing feedback
 * 
 * **Recommended:** Use INSTRUCTION_LEVEL for ISA-aware fuzzing with full schema support.
 */
enum class Strategy : uint8_t { 
  BYTE_LEVEL = 0,       ///< Raw bitflip mutations only (legacy mode)
  INSTRUCTION_LEVEL = 1, ///< Schema-driven instruction-level mutations (default)
  MIXED_MODE = 2,       ///< Combination of instruction and byte mutations
  ADAPTIVE = 3          ///< Adaptive strategy selection based on coverage feedback
};

/**
 * @struct Config
 * @brief Mutator configuration loaded from YAML file
 * 
 * This structure holds runtime configuration for the ISA mutator,
 * including mutation strategy and ISA schema settings.
 * Loaded from MUTATOR_CONFIG environment variable path.
 * 
 * @see loadConfig()
 */
struct Config {
  Strategy strategy = Strategy::INSTRUCTION_LEVEL; ///< Mutation strategy (BYTE_LEVEL/INSTRUCTION_LEVEL/MIXED_MODE/ADAPTIVE)
  std::string isa_name;                            ///< ISA identifier (e.g., "rv32im")
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
