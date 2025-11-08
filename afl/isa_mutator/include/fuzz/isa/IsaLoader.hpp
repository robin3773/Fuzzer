/**
 * @file IsaLoader.hpp
 * @brief ISA schema loader for YAML-based instruction set definitions
 * 
 * This file defines the data structures and loader functions for parsing
 * ISA descriptions from YAML files. The schema system enables intelligent,
 * architecture-aware mutations by providing complete instruction encoding
 * information including formats, fields, and constraints.
 * 
 * @details
 * Schema System Overview:
 * - YAML files define instruction formats, fields, and encodings
 * - Supports multi-segment fields (e.g., RISC-V immediates)
 * - Provides mutation hints for better fuzzing coverage
 * - Enables validation of generated instructions
 * 
 * Schema Directory Resolution:
 * - Always uses PROJECT_ROOT/schemas (computed relative to library location)
 * - ISA definitions resolved via isa_map.yaml
 * - Supports multi-file schemas with includes and anchors
 * 
 * @see ISAConfig
 * @see load_isa_config()
 * @see schemas/riscv/ for example YAML files
 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace fuzz::isa {

  /**
   * @struct FieldSegment
   * @brief Describes how a logical field maps to physical instruction bits
   * 
   * A single logical field (e.g., a 12-bit immediate) may be split across
   * non-contiguous bit positions in the instruction word. Each segment
   * describes one contiguous chunk.
   * 
   * Example - RISC-V S-type instruction immediate field:
   * @code
   * // The 12-bit immediate is split:
   * // Segment 0: imm[4:0]  -> instruction bits [11:7]
   * FieldSegment{.word_lsb = 7, .width = 5, .value_lsb = 0};
   * 
   * // Segment 1: imm[11:5] -> instruction bits [31:25]
   * FieldSegment{.word_lsb = 25, .width = 7, .value_lsb = 5};
   * 
   * // For immediate value 0xABC (binary: 101010111100):
   * // bits [4:0]  (11100) go to instruction[11:7]
   * // bits [11:5] (1010101) go to instruction[31:25]
   * @endcode
   */
  struct FieldSegment {
    uint32_t word_lsb = 0;    ///< LSB position in instruction word (0-31 for RV32)
    uint32_t width = 0;       ///< Number of bits in this segment
    uint32_t value_lsb = 0;   ///< LSB position in logical field value
  };

  /**
   * @enum FieldKind
   * @brief High-level classification for instruction fields
   * 
   * Used to guide mutation strategies based on field semantics.
   */
  enum class FieldKind : uint8_t {
    Unknown = 0,   ///< Unclassified field
    Opcode,        ///< Operation code (usually fixed)
    Enum,          ///< Enumerated value (e.g., funct3, funct7)
    Immediate,     ///< Immediate value (signed or unsigned)
    Predicate,     ///< Conditional predicate
    Memory,        ///< Memory addressing field
    Register,      ///< Register specifier
    Floating,      ///< Floating-point specific field
  };

  /**
   * @struct FieldEncoding
   * @brief Complete specification of an instruction field
   * 
   * Describes both the logical properties (width, signedness) and
   * physical layout (segments) of a field within an instruction format.
   * 
   * Example - RISC-V rd (destination register) field:
   * @code
   * FieldEncoding rd_field = {
   *   .name = "rd",
   *   .width = 5,
   *   .is_signed = false,
   *   .segments = {
   *     FieldSegment{.word_lsb = 7, .width = 5, .value_lsb = 0}
   *   },
   *   .kind = FieldKind::Register,
   *   .raw_type = "uint"
   * };
   * @endcode
   * 
   * Example - RISC-V I-type 12-bit signed immediate:
   * @code
   * FieldEncoding imm12_field = {
   *   .name = "imm12",
   *   .width = 12,
   *   .is_signed = true,
   *   .segments = {
   *     FieldSegment{.word_lsb = 20, .width = 12, .value_lsb = 0}
   *   },
   *   .kind = FieldKind::Immediate,
   *   .raw_type = "imm"
   * };
   * @endcode
   */
  struct FieldEncoding {
    std::string name;                      ///< Field name (e.g., "rd", "imm12")
    uint32_t width = 0;                    ///< Total width in bits
    bool is_signed = false;                ///< Whether field is sign-extended
    std::vector<FieldSegment> segments;    ///< Physical bit positions
    FieldKind kind = FieldKind::Unknown;   ///< Semantic classification
    std::string raw_type;                  ///< Type from YAML (for reference)
  };

  /**
   * @struct FormatSpec
   * @brief Instruction format definition
   * 
   * Defines the structure of an instruction format (e.g., R-type, I-type)
   * by specifying its width and ordered list of fields. The fields list
   * represents the logical order of field definitions, which may differ
   * from their bit layout order (handled by FieldSegment positions).
   * 
   * @code
   * // RISC-V R-type format (register-register operations like ADD, SUB, AND)
   * FormatSpec r_format = {
   *   .name = "R",
   *   .width = 32,
   *   .fields = {"opcode", "rd", "funct3", "rs1", "rs2", "funct7"}
   * };
   * 
   * // RISC-V I-type format (immediate operations like ADDI, LW)
   * FormatSpec i_format = {
   *   .name = "I",
   *   .width = 32,
   *   .fields = {"opcode", "rd", "funct3", "rs1", "imm12"}
   * };
   * @endcode
   */
  struct FormatSpec {
    std::string name;                ///< Format name (e.g., "R", "I", "S")
    uint32_t width = 0;              ///< Instruction width in bits (usually 32)
    std::vector<std::string> fields; ///< Ordered list of field names
  };

  /**
   * @struct InstructionSpec
   * @brief Individual instruction specification
   * 
   * Ties an instruction mnemonic to its format and specifies fixed
   * field values (e.g., opcode, funct3/funct7). These fixed fields
   * distinguish one instruction from another within the same format.
   * Variable fields (like rs1, rs2, rd) are filled during mutation.
   * 
   * @code
   * // RISC-V ADD instruction: rd = rs1 + rs2
   * InstructionSpec add_insn = {
   *   .name = "add",
   *   .format = "R",
   *   .fixed_fields = {
   *     {"opcode", 0b0110011},  // OP opcode
   *     {"funct3", 0b000},      // ADD funct3
   *     {"funct7", 0b0000000}   // ADD (not SUB) funct7
   *   }
   * };
   * 
   * // RISC-V LW instruction: rd = mem[rs1 + imm12]
   * InstructionSpec lw_insn = {
   *   .name = "lw",
   *   .format = "I",
   *   .fixed_fields = {
   *     {"opcode", 0b0000011},  // LOAD opcode
   *     {"funct3", 0b010}       // LW width (word)
   *   }
   *   // imm12, rs1, rd are variable and filled during mutation
   * };
   * @endcode
   */
  struct InstructionSpec {
    std::string name;                                    ///< Instruction mnemonic (e.g., "add", "lw")
    std::string format;                                  ///< Format name reference
    std::unordered_map<std::string, uint32_t> fixed_fields; ///< Fixed field values (opcode, funct, etc.)
  };

  /**
   * @struct MutationHints
   * @brief Tunable parameters to guide mutation strategies
   * 
   * Provides hints to the mutator for generating more interesting/valid
   * test cases based on ISA-specific constraints and patterns.
   */
  struct MutationHints {
    bool reg_prefers_zero_one_hot = false;  ///< Prefer x0/x1 register values
    bool signed_immediates_bias = false;    ///< Bias toward edge values (-1, 0, 1)
    uint32_t align_load_store = 0;          ///< Required alignment for memory ops (0=none, 4=word)
  };

  /**
   * @struct ISAConfigDefaults
   * @brief Default values and conventions for the ISA
   */
  struct ISAConfigDefaults {
    std::string endianness = "little";  ///< Byte order ("little" or "big")
    int64_t default_pc = 0;             ///< Default program counter value
    MutationHints hints;                ///< Mutation strategy hints
  };

  /**
   * @struct ISAConfig
   * @brief Complete ISA definition loaded from YAML schemas
   * 
   * This is the main structure returned by load_isa_config() and consumed
   * by the mutator. Contains all information needed to generate valid
   * instructions for the target ISA. The loader resolves references between
   * formats, fields, and instructions to create a cohesive ISA model.
   * 
   * @code
   * // Load RISC-V RV32I base ISA configuration
   * auto config = fuzz::isa::load_isa_config("rv32i");
   * 
   * // Access ISA properties
   * std::cout << "ISA: " << config.isa_name << std::endl;              // "rv32i"
   * std::cout << "Registers: " << config.register_count << std::endl;  // 32
   * std::cout << "Width: " << config.base_width << " bits" << std::endl; // 32
   * 
   * // Look up instruction by name
   * for (const auto& insn : config.instructions) {
   *   if (insn.name == "add") {
   *     std::cout << "ADD uses format: " << insn.format << std::endl;  // "R"
   *     std::cout << "ADD opcode: 0x" << std::hex 
   *               << insn.fixed_fields.at("opcode") << std::endl;      // 0x33
   *   }
   * }
   * 
   * // Access field encoding for mutation
   * const auto& rd_field = config.fields.at("rd");
   * std::cout << "rd field width: " << rd_field.width << " bits" << std::endl; // 5
   * @endcode
   */
  struct ISAConfig {
    std::string isa_name;                                   ///< ISA identifier (e.g., "rv32im")
    uint32_t base_width = 0;                                ///< Default instruction width in bits
    uint32_t register_count = 0;                            ///< Number of general-purpose registers
    ISAConfigDefaults defaults;                             ///< ISA-wide defaults and hints
    std::unordered_map<std::string, FieldEncoding> fields;  ///< All field definitions
    std::unordered_map<std::string, FormatSpec> formats;    ///< All format definitions
    std::vector<InstructionSpec> instructions;              ///< All instruction definitions
  };

  /**
   * @brief Load ISA configuration by name from YAML schemas
   * 
   * Reads the ISA definition from YAML files in the schema directory.
   * The loader resolves field definitions, formats, and instructions
   * into a complete ISAConfig structure. Uses isa_map.yaml to map
   * ISA names to their corresponding YAML schema files.
   * 
   * The schema directory is automatically computed as PROJECT_ROOT/schemas
   * relative to the location of the ISA mutator library.
   * 
   * @param isa_name ISA identifier (e.g., "rv32im", "rv64gc")
   * @return Fully populated ISAConfig structure
   * @throws std::runtime_error if schema files cannot be loaded or parsed
   * 
   * @note Schema directory is always PROJECT_ROOT/schemas (no environment variable needed)
   * @note Uses isa_map.yaml for ISA-to-file mapping
   * 
   * @code
   * // Load RV32I (base integer instruction set)
   * try {
   *   auto rv32i_config = fuzz::isa::load_isa_config("rv32i");
   *   std::cout << "Loaded " << rv32i_config.instructions.size() 
   *             << " instructions" << std::endl;
   * } catch (const std::runtime_error& e) {
   *   std::cerr << "Failed to load ISA: " << e.what() << std::endl;
   * }
   * 
   * // Load RV32IM (base + multiply/divide extension)
   * // Schema directory is automatically resolved to PROJECT_ROOT/schemas
   * auto rv32im_config = fuzz::isa::load_isa_config("rv32im");
   * 
   * // The returned config contains all instructions from both rv32i.yaml and rv32m.yaml
   * for (const auto& insn : rv32im_config.instructions) {
   *   std::cout << insn.name << " (" << insn.format << ")" << std::endl;
   * }
   * @endcode
   */
  ISAConfig load_isa_config(const std::string &isa_name);

} // namespace fuzz::isa
