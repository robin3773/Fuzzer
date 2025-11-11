/**
 * @file ISAMutator.hpp
 * @brief Schema-driven instruction mutation engine for AFL++ fuzzing
 * 
 * This file defines the main ISAMutator class which performs intelligent
 * mutations on binary instruction streams using ISA schema knowledge.
 * Supports multiple mutation strategies (BYTE_LEVEL, INSTRUCTION_LEVEL, 
 * MIXED_MODE, ADAPTIVE) and automatically appends exit stubs to ensure 
 * clean program termination.
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
 * using schema-guided mutations based on ISA definitions. It integrates
 * with AFL++ via the MutatorInterface and automatically handles exit stub
 * injection.
 * 
 * @details
 * Key features:
 * - Schema-driven mutation using YAML ISA definitions
 * - Multiple mutation strategies (REPLACE, INSERT, DELETE, DUPLICATE)
 * - Automatic exit stub appending for clean termination
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
     * 
     * @details
     * Initializes the mutator in an uninitialized state. The initFromEnv()
     * method must be called before performing any mutations. This two-phase
     * construction allows for flexible configuration paths (environment
     * variables or command-line arguments).
     * 
     * @post Object is in uninitialized state; call initFromEnv() before use
     */
    ISAMutator();
    
    /**
     * @brief Initialize mutator from environment variables and config file
     * 
     * @details
     * Performs complete initialization of the mutator:
     * 1. Reads MUTATOR_CONFIG environment variable for config file path
     *    (or uses cli_config_path_ if set via setConfigPath())
     * 2. Loads mutator configuration (strategy, probabilities, ISA name)
     * 3. Loads ISA schema from YAML files using PROJECT_ROOT
     * 4. Validates schema consistency (opcodes, formats, fields)
     * 5. Enables debug logging if DEBUG environment variable is set
     * 6. Falls back to heuristic mutations if schema loading fails
     * 
     * @throws std::runtime_error if critical initialization fails
     * @post Mutator is ready to perform mutations
     * @note This method is idempotent; calling multiple times is safe
     */
    void initFromEnv() override;
    
    /**
     * @brief Set configuration file path before initialization
     * 
     * @details
     * Allows setting the config file path programmatically (e.g., from
     * command-line arguments) instead of relying on the MUTATOR_CONFIG
     * environment variable. Must be called before initFromEnv().
     * 
     * @param path Absolute path to YAML configuration file
     * @pre Path should be absolute and point to valid YAML file
     * @post cli_config_path_ is set; will be used by initFromEnv()
     * @note This takes precedence over MUTATOR_CONFIG environment variable
     */
    void setConfigPath(const std::string &path) override { cli_config_path_ = path; }

    /**
     * @brief Main mutation entry point
     * 
     * @details
     * Core mutation pipeline:
     * 1. Validates input size (rejects if too small or too large)
     * 2. Applies instruction-level mutations using ISA specifications
     * 3. Applies one of 4 mutation strategies randomly (1-50 mutations per call):
     * 
     *    **REPLACE** (Strategy 0):
     *    - Selects a random instruction at index i in the stream
     *    - Generates a new valid instruction from ISA schema using pickInstruction()
     *    - Replaces instruction[i] with the newly encoded instruction
     *    - Maintains stream length (same number of instructions)
     *    - Use case: Tests different instruction opcodes and operand combinations
     * 
     *    **INSERT** (Strategy 1):
     *    - Selects a random position i in the instruction stream
     *    - Generates a new valid instruction from ISA schema
     *    - Shifts all instructions after position i forward by one slot
     *    - Inserts new instruction at position i
     *    - Increases stream length by one instruction (up to 512 instruction limit)
     *    - Use case: Tests control flow alterations and instruction sequence variations
     * 
     *    **DELETE** (Strategy 2):
     *    - Selects a random instruction at index i to remove
     *    - Shifts all instructions after position i backward by one slot
     *    - Decreases stream length by one instruction (minimum 16 instructions enforced)
     *    - Use case: Tests robustness against missing instructions and shorter sequences
     * 
     *    **DUPLICATE** (Strategy 3):
     *    - Selects a random source instruction at index src
     *    - Selects a random destination position dst
     *    - Copies instruction[src] and inserts it at position dst
     *    - Shifts instructions after dst forward by one slot
     *    - Increases stream length by one instruction (up to 512 instruction limit)
     *    - Use case: Tests repeated instruction patterns and loop-like behaviors
     * 
     * 4. Validates mutated instructions against ISA legality rules
     * 5. Automatically appends exit stub for clean termination
     * 6. Ensures output fits within max_size constraints
     * 
     * Strategy selection is uniform random (25% probability each). Multiple mutations
     * are applied per call (1-50), allowing complex transformations in a single pass.
     * 
     * The exit stub ensures that mutated code terminates cleanly rather
     * than running off the end of the buffer, preventing crashes and
     * enabling proper differential testing.
     * 
     * @param in Input buffer containing instruction bytes (unmodified)
     * @param in_len Length of input buffer in bytes (must be multiple of word_bytes_)
     * @param out_buf Output buffer for mutated instructions (caller-allocated)
     * @param max_size Maximum size of output buffer (hard limit)
     * @return Pointer to output buffer containing mutated instructions
     * @retval out_buf on success
     * @retval nullptr if mutation fails or constraints violated
     * 
     * @pre in != nullptr && out_buf != nullptr
     * @pre in_len > 0 && in_len % word_bytes_ == 0
     * @pre max_size >= in_len + exit_stub_size
     * @pre ISA schema is loaded
     * @post last_len_ contains actual output size
     * 
     * @note Thread-safe if Random instances are thread-local
     * @warning Output may be larger than input due to INSERT/DUPLICATE operations
     */
    unsigned char *mutateStream(unsigned char *in, size_t in_len,
                                unsigned char *out_buf,
                                size_t max_size) override;

    /**
     * @brief Get the length of the last mutation output
     * 
     * @details
     * Returns the size of the buffer produced by the last successful
     * call to mutateStream(). Includes the appended exit stub size.
     * Useful for AFL++ to know the actual size of the mutated test case.
     * 
     * **Example:**
     * ```cpp
     * ISAMutator mutator;
     * mutator.initFromEnv();
     * 
     * // Input: 20 instructions (80 bytes for RV32)
     * unsigned char input[80];
     * unsigned char output[4096];
     * 
     * // Perform mutation (might insert 5 instructions)
     * mutator.mutateStream(input, 80, output, 4096);
     * 
     * // Get actual output size
     * size_t output_size = mutator.last_out_len();
     * // output_size = (20 + 5 payload + 4 exit stub) * 4 = 116 bytes
     * 
     * // AFL++ uses this to know the exact mutated test case size
     * write_testcase(output, output_size);
     * ```
     * 
     * The size varies depending on mutation strategy:
     * - REPLACE: same as input + exit stub
     * - INSERT: input + (n * 4) + exit stub (n = insertions - deletions)
     * - DELETE: input - (n * 4) + exit stub (enforces minimum 64 bytes)
     * - DUPLICATE: input + (n * 4) + exit stub (n = duplications)
     * 
     * @return Size in bytes of the last mutated buffer
     * @retval 0 if no successful mutations have been performed yet
     * @note This is updated by mutateStream() on each call
     * @note Always includes 16-byte exit stub (4 instructions * 4 bytes)
     */
    size_t last_out_len() const override { return last_len_; }
    
    /**
     * @brief Get the current ISA name (e.g., "rv32im")
     * 
     * @details
     * Returns the ISA identifier loaded from the configuration file.
     * This corresponds to the YAML schema file loaded (e.g., "rv32im"
     * loads rv32i.yaml and rv32m.yaml with their respective instruction
     * sets and encoding rules).
     * 
     * @return ISA name string (e.g., "rv32i", "rv32im", "rv32imc")
     * @note Empty string if schema not loaded
     */
    const std::string &isa_name() const { return isa_.isa_name; }
    
    /**
     * @brief Get the current mutation strategy
     * 
     * @details
     * Returns the high-level mutation strategy configured for this mutator:
     * 
     * - **BYTE_LEVEL**: Direct byte-level mutations using bit flips, byte 
     *   insertions/deletions, and arithmetic operations. Does not use ISA
     *   schema knowledge. Legacy mode - not recommended.
     * 
     * - **INSTRUCTION_LEVEL**: Schema-driven mutations that understand ISA
     *   structure. Uses REPLACE, INSERT, DELETE, and DUPLICATE operations
     *   on valid instructions. Recommended for ISA-aware fuzzing.
     * 
     * - **MIXED_MODE**: Combines instruction-level and byte-level mutations.
     *   Uses decode_prob to control the ratio (e.g., 60% instruction-level,
     *   40% byte-level). Useful for exploring both valid and invalid encodings.
     * 
     * - **ADAPTIVE**: Dynamically selects between strategies based on AFL++
     *   coverage feedback. Experimental mode that adapts to fuzzing progress.
     * 
     * @return Strategy enum (BYTE_LEVEL, INSTRUCTION_LEVEL, MIXED_MODE, ADAPTIVE)
     * @see Strategy enum in MutatorConfig.hpp
     * @note Current implementation primarily uses INSTRUCTION_LEVEL
     */
    Strategy strategy() const { return cfg_.strategy; }

  private:
    Config cfg_;                     ///< Mutator configuration (strategy, probabilities, mutation weights)
    isa::ISAConfig isa_;             ///< ISA schema loaded from YAML files (instructions, formats, fields)
    std::string cli_config_path_;    ///< Config path set via setConfigPath() (overrides MUTATOR_CONFIG)
    size_t last_len_ = 0;            ///< Length of last mutation output in bytes (includes exit stub)
    uint32_t word_bytes_ = 4;        ///< Bytes per instruction word (4 for RV32, 8 for RV64)

    /**
     * @brief Apply instruction-level mutations to the input stream
     * 
     * @details
     * Core mutation engine that uses ISA schema knowledge to perform
     * intelligent instruction-level mutations. Applies 1-50 random
     * mutations per call using one of four strategies:
     * 
     * **Strategy Details:**
     * 
     * 1. **REPLACE** (Strategy 0 - In-place modification):
     *    - Operation: instruction[i] ← generateNewInstruction()
     *    - Effect: Replaces existing instruction with new random instruction
     *    - Size impact: None (maintains instruction count)
     *    - Constraints: None (always applicable if nwords > 0)
     *    - Example: ADD x1, x2, x3 → XOR x4, x5, x6
     * 
     * 2. **INSERT** (Strategy 1 - Stream expansion):
     *    - Operation: stream.insert(i, generateNewInstruction())
     *    - Effect: Adds new instruction and shifts subsequent instructions forward
     *    - Size impact: +1 instruction (+4 bytes for RV32)
     *    - Constraints: nwords < 512 (max payload limit)
     *    - Example: [ADD, SUB] → [ADD, **LW**, SUB]
     * 
     * 3. **DELETE** (Strategy 2 - Stream reduction):
     *    - Operation: stream.remove(i)
     *    - Effect: Removes instruction and shifts subsequent instructions backward
     *    - Size impact: -1 instruction (-4 bytes for RV32)
     *    - Constraints: nwords > 16 (min payload limit)
     *    - Example: [ADD, SUB, XOR] → [ADD, XOR]
     * 
     * 4. **DUPLICATE** (Strategy 3 - Instruction copying):
     *    - Operation: stream.insert(dst, instruction[src])
     *    - Effect: Copies existing instruction to new location
     *    - Size impact: +1 instruction (+4 bytes for RV32)
     *    - Constraints: nwords < 512 and nwords > 0
     *    - Example: [ADD, SUB] → [ADD, **ADD**, SUB] (duplicated ADD)
     * 
     * **Mutation Process:**
     * 1. Copy input buffer to output buffer
     * 2. For each of 1-50 iterations:
     *    a. Randomly select strategy (0-3 with uniform 25% probability)
     *    b. Check strategy-specific constraints (size limits, instruction count)
     *    c. Apply mutation if constraints satisfied
     *    d. Validate instruction legality using is_legal_instruction()
     * 3. Append exit stub to ensure clean program termination
     * 
     * **Size Limits:**
     * - Minimum payload: 16 instructions (64 bytes for RV32)
     * - Maximum payload: 512 instructions (2048 bytes for RV32)
     * - Exit stub: 4 instructions (16 bytes) appended after payload
     * 
     * @param in Input buffer containing instruction bytes
     * @param in_len Input length in bytes (must be multiple of word_bytes_)
     * @param out_buf Output buffer for mutated instructions (unused - always allocates)
     * @param max_size Maximum output size in bytes (soft limit, respects payload constraints)
     * @return Pointer to newly allocated mutated buffer
     * @retval nullptr if mutation fails or memory allocation fails
     * 
     * @pre ISA schema is loaded (isa_.instructions is non-empty)
     * @pre in_len % word_bytes_ == 0
     * @post Output contains valid instruction encoding per ISA schema
     * @post Output includes exit stub at end
     * @post last_len_ updated with total output size (payload + exit stub)
     * @note May increase or decrease output size vs input
     */
    unsigned char *applyMutations(unsigned char *in, size_t in_len,
                                  unsigned char *out_buf,
                                  size_t max_size);

    /**
     * @brief Select a random instruction from the ISA schema
     * 
     * @details
     * Randomly picks one instruction from isa_.instructions vector
     * using uniform distribution. The selected instruction's opcode,
     * format, and field specifications are used for encoding.
     * 
     * @return Reference to randomly selected instruction spec
     * @pre isa_.instructions is non-empty
     * @note Uses internal Random instance seeded from AFL++
     * @warning May return different results for same input (intentional)
     */
    const isa::InstructionSpec &pickInstruction() const;
    
    /**
     * @brief Encode an instruction according to its schema definition
     * 
     * @details
     * Constructs a complete instruction word by:
     * 1. Starting with the instruction's base opcode
     * 2. For each field in the instruction's format:
     *    a. Generate random value via randomFieldValue()
     *    b. Apply field to instruction word via applyField()
     * 3. Return complete encoded instruction
     * 
     * Handles both standard (32-bit) and compressed (16-bit) instructions
     * based on the format specification.
     * 
     * @param spec Instruction specification from ISA schema
     * @return Encoded 32-bit instruction word (or 16-bit in lower half)
     * @pre spec is valid and has known format
     * @post Returned value is valid per ISA encoding rules
     * @note For RVC instructions, only lower 16 bits are meaningful
     */
    uint32_t encodeInstruction(const isa::InstructionSpec &spec) const;
    
    /**
     * @brief Generate random value for an instruction field
     * 
     * @details
     * Creates a random value appropriate for the given field type:
     * - For registers (rd, rs1, rs2): 0-31 for RV32 (0-15 for RVC compressed)
     * - For immediates: Random value within field width, respecting signed/unsigned
     * - For opcodes: Uses opcode from instruction spec (not randomized)
     * - For funct3/funct7: Random value matching field width
     * 
     * Special handling:
     * - Register x0 may be avoided for destination registers (architecture-specific)
     * - Immediate values respect sign-extension rules
     * - Multi-segment fields (S-type, B-type) are handled correctly
     * 
     * @param field_name Name of the field (e.g., "rd", "rs1", "imm12")
     * @param enc Field encoding specification (width, position, signed flag)
     * @return Random value within field constraints
     * @pre enc.width > 0 && enc.width <= 32
     * @post Return value fits within enc.width bits
     * @note Uses internal Random instance for deterministic fuzzing
     */
    uint32_t randomFieldValue(const std::string &field_name,
                              const isa::FieldEncoding &enc) const;
    
    /**
     * @brief Apply a field value to an instruction word
     * 
     * @details
     * Inserts a field value into the correct bit positions of an
     * instruction word. Handles both single-segment and multi-segment
     * fields (e.g., S-type immediate split across two positions).
     * 
     * Process:
     * 1. For each segment in enc.segments:
     *    a. Extract appropriate bits from value
     *    b. Shift to target position
     *    c. OR into instruction word
     * 
     * Example: S-type immediate [11:5] at bits 31-25, [4:0] at bits 11-7
     * 
     * @param word Instruction word to modify (modified in-place)
     * @param enc Field encoding (segments, positions, width)
     * @param value Value to apply (must fit within enc.width bits)
     * @pre value fits within enc.width bits
     * @post word contains value at positions specified by enc
     * @note Does not clear existing bits first; caller must initialize word
     */
    void applyField(uint32_t &word, const isa::FieldEncoding &enc,
                    uint32_t value) const;
    
    /**
     * @brief Read a 32-bit word from buffer (little-endian)
     * 
     * @details
     * Reads 4 consecutive bytes from buffer and combines them into
     * a 32-bit word using little-endian byte order (LSB first).
     * Used to extract instructions from binary buffers.
     * 
     * Byte layout: [offset+0] | [offset+1]<<8 | [offset+2]<<16 | [offset+3]<<24
     * 
     * @param buf Buffer to read from (must have at least offset+4 valid bytes)
     * @param offset Byte offset into buffer (typically multiple of 4)
     * @return 32-bit word value in host byte order
     * @pre offset + 4 <= buffer size
     * @note Assumes RISC-V little-endian encoding
     */
    uint32_t readWord(const unsigned char *buf, size_t offset) const;
    
    /**
     * @brief Write a 32-bit word to buffer (little-endian)
     * 
     * @details
     * Writes a 32-bit word to buffer as 4 consecutive bytes in
     * little-endian byte order (LSB first). Used to write mutated
     * instructions back to the output buffer.
     * 
     * Byte layout: buf[offset+0] = word & 0xFF
     *              buf[offset+1] = (word >> 8) & 0xFF
     *              buf[offset+2] = (word >> 16) & 0xFF
     *              buf[offset+3] = (word >> 24) & 0xFF
     * 
     * @param buf Buffer to write to (must have at least offset+4 valid bytes)
     * @param offset Byte offset into buffer (typically multiple of 4)
     * @param word Value to write in host byte order
     * @pre offset + 4 <= buffer size
     * @post buf[offset..offset+3] contains word in little-endian format
     * @note Assumes RISC-V little-endian encoding
     */
    void writeWord(unsigned char *buf, size_t offset, uint32_t word) const;
};

} // namespace fuzz::mutator
