/**
 * @file ISAMutator.hpp
 * @brief Schema-driven instruction mutation engine for AFL++ fuzzing
 * 
 * This file defines the main ISAMutator class which performs intelligent
 * mutations on binary instruction streams using ISA schema knowledge.
 * Supports multiple mutation strategies (RAW, IR, HYBRID, AUTO) and
 * automatically appends exit stubs to ensure clean program termination.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <fuzz/isa/IsaLoader.hpp>
#include <fuzz/mutator/MutatorConfig.hpp>
#include <fuzz/mutator/MutatorInterface.hpp>

namespace fuzz::mutator {

/**
 * @class ISAMutator
 * @brief Main mutation engine that applies ISA-aware transformations
 * 
 * ISAMutator is the core component that mutates instruction streams
 * using either schema-guided mutations (when ISA definitions are available)
 * or fallback heuristic mutations. It integrates with AFL++ via the
 * MutatorInterface and automatically handles exit stub injection.
 * 
 * @details
 * Key features:
 * - Schema-driven mutation using YAML ISA definitions
 * - Multiple mutation strategies (REPLACE, INSERT, DELETE, DUPLICATE)
 * - Automatic exit stub appending for clean termination
 * - Fallback to heuristic mutations when schema unavailable
 * - Support for compressed (RVC) instructions
 * 
 * @see MutatorInterface
 * @see Config
 * @see isa::ISAConfig
 */
class ISAMutator : public MutatorInterface {
  public:
    /**
     * @brief Construct a new ISAMutator object
     */
    ISAMutator();
    
    /**
     * @brief Initialize mutator from environment variables and config file
     * 
     * Loads configuration from MUTATOR_CONFIG environment variable,
     * initializes the ISA schema, and sets up the debug system.
     */
    void initFromEnv() override;
    
    /**
     * @brief Set configuration file path before initialization
     * @param path Absolute path to YAML configuration file
     */
    void setConfigPath(const std::string &path) override { cli_config_path_ = path; }

    /**
     * @brief Main mutation entry point
     * 
     * Applies mutations to the input buffer according to the configured
     * strategy, validates the result, and appends an exit stub.
     * 
     * @param in Input buffer containing instruction bytes
     * @param in_len Length of input buffer in bytes
     * @param out_buf Output buffer for mutated instructions
     * @param max_size Maximum size of output buffer
     * @return Pointer to output buffer containing mutated instructions
     */
    unsigned char *mutateStream(unsigned char *in, size_t in_len,
                                unsigned char *out_buf,
                                size_t max_size) override;

    /**
     * @brief Get the length of the last mutation output
     * @return Size in bytes of the last mutated buffer
     */
    size_t last_out_len() const override { return last_len_; }

    /**
     * @brief Check if schema-guided mutations are active
     * @return true if ISA schema is loaded and being used
     */
    bool using_schema() const { return use_schema_; }
    
    /**
     * @brief Get the current ISA name (e.g., "rv32im")
     * @return ISA name string
     */
    const std::string &isa_name() const { return isa_.isa_name; }
    
    /**
     * @brief Get the current mutation strategy
     * @return Strategy enum (RAW, IR, HYBRID, AUTO)
     */
    Strategy strategy() const { return cfg_.strategy; }

  private:
    Config cfg_;                     ///< Mutator configuration (strategy, probabilities, weights)
    isa::ISAConfig isa_;             ///< ISA schema loaded from YAML files
    std::string cli_config_path_;    ///< Config path set via command line
    bool use_schema_ = false;        ///< Whether schema-guided mutations are active
    size_t last_len_ = 0;            ///< Length of last mutation output
    uint32_t word_bytes_ = 4;        ///< Bytes per instruction word (4 for RV32)

    /**
     * @brief Apply schema-guided mutations
     * @param in Input buffer
     * @param in_len Input length in bytes
     * @param out_buf Output buffer
     * @param max_size Maximum output size
     * @return Pointer to mutated output buffer
     */
    unsigned char *mutateWithSchema(unsigned char *in, size_t in_len,
                                    unsigned char *out_buf,
                                    size_t max_size);
    
    /**
     * @brief Apply heuristic mutations when schema unavailable
     * @param in Input buffer
     * @param in_len Input length in bytes
     * @param out_buf Output buffer
     * @param max_size Maximum output size
     * @return Pointer to mutated output buffer
     */
    unsigned char *mutateFallback(unsigned char *in, size_t in_len,
                                  unsigned char *out_buf,
                                  size_t max_size);

    /**
     * @brief Select a random instruction from the ISA schema
     * @return Reference to randomly selected instruction spec
     */
    const isa::InstructionSpec &pickInstruction() const;
    
    /**
     * @brief Encode an instruction according to its schema definition
     * @param spec Instruction specification
     * @return Encoded 32-bit instruction word
     */
    uint32_t encodeInstruction(const isa::InstructionSpec &spec) const;
    
    /**
     * @brief Generate random value for an instruction field
     * @param field_name Name of the field (for special handling)
     * @param enc Field encoding specification
     * @return Random value within field constraints
     */
    uint32_t randomFieldValue(const std::string &field_name,
                              const isa::FieldEncoding &enc) const;
    
    /**
     * @brief Apply a field value to an instruction word
     * @param word Instruction word to modify (in/out)
     * @param enc Field encoding (position, mask, width)
     * @param value Value to apply
     */
    void applyField(uint32_t &word, const isa::FieldEncoding &enc,
                    uint32_t value) const;
    
    /**
     * @brief Read a 32-bit word from buffer (little-endian)
     * @param buf Buffer to read from
     * @param offset Byte offset
     * @return 32-bit word value
     */
    uint32_t readWord(const unsigned char *buf, size_t offset) const;
    
    /**
     * @brief Write a 32-bit word to buffer (little-endian)
     * @param buf Buffer to write to
     * @param offset Byte offset
     * @param word Value to write
     */
    void writeWord(unsigned char *buf, size_t offset, uint32_t word) const;

    /**
     * @struct Rule
     * @brief Fallback mutation rule definition
     */
    struct Rule {
      std::string type;              ///< Rule type (replace, insert, delete, etc.)
      uint32_t weight = 10;          ///< Selection weight for this rule
      uint32_t min = 1;              ///< Minimum applications per mutation
      uint32_t max = 1;              ///< Maximum applications per mutation
      std::vector<uint8_t> pattern;  ///< Byte pattern for this rule
    };

    std::vector<Rule> rules_;        ///< Loaded fallback mutation rules
    
    /**
     * @brief Load fallback mutation rules from config
     * @param path Path to fallback config file
     * @return true if rules loaded successfully
     */
    bool loadFallbackConfig(const std::string &path);
    
    /**
     * @brief Trim whitespace from string
     * @param s Input string
     * @return Trimmed string
     */
    static std::string trim(const std::string &s);
    
    /**
     * @brief Apply a single mutation rule to buffer
     * @param r Rule to apply
     * @param buf Buffer to mutate
     * @param len Current buffer length (in/out)
     * @param cap Maximum buffer capacity
     */
    void applyRule(const Rule &r, unsigned char *buf, size_t &len, size_t cap);
};

} // namespace fuzz::mutator
