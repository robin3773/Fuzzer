#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace fuzz::isa {

// Describes how a logical field spills into the physical instruction word.
struct FieldSegment {
  uint32_t word_lsb = 0;   // Least-significant bit of the segment in the word.
  uint32_t width = 0;      // Number of bits occupied by the segment.
  uint32_t value_lsb = 0;  // Least-significant bit in the logical field value.
};

// High-level classification for a decoded field to guide mutations.
enum class FieldKind : uint8_t {
  Unknown = 0,   // Kind could not be inferred from schema metadata.
  Register,      // General-purpose register field.
  Immediate,     // Immediate literal value.
  Opcode,        // Opcode or primary encoding field.
  Enum,          // Enumerated control or funct bits.
  Predicate,     // Conditional or predicate bits.
  Memory,        // Memory addressing information (base/offset etc.).
  Floating,      // Floating-point register field.
};

// Fully describes a logical field and its layout segments.
struct FieldEncoding {
  std::string name;                    // Schema name for the field.
  uint32_t width = 0;                  // Total width in bits.
  bool is_signed = false;              // True if field should be treated as signed.
  std::vector<FieldSegment> segments;  // Physical bit slices composing the field.
  FieldKind kind = FieldKind::Unknown; // Mutation hint category derived from metadata.
  std::string raw_type;                // Original schema type string, if provided.
};

// Format definition mapping fields into a fixed instruction width.
struct FormatSpec {
  std::string name;             // Schema-provided format identifier.
  uint32_t width = 0;           // Total instruction width in bits for the format.
  std::vector<std::string> fields; // Ordered list of field names used by the format.
};

// Instruction entry tying a format to specific fixed field values.
struct InstructionSpec {
  std::string name;                                       // Instruction mnemonic or id.
  std::string format;                                     // Format name consumed by encoder.
  std::unordered_map<std::string, uint32_t> fixed_fields; // Field→value pairs pinned by schema.
};

// Tunables that bias random selection for better fuzz coverage.
struct MutationHints {
  bool reg_prefers_zero_one_hot = false; // Favor zero/one-hot register choices.
  bool signed_immediates_bias = false;   // Encourage near-zero signed immediates.
  uint32_t align_load_store = 0;         // Alignment to respect for memory offsets.
};

// Static ISA metadata shared across instructions.
struct ISAStatics {
  std::string endianness = "little"; // Expected endianness for instruction words.
  uint64_t default_pc = 0;            // Default program counter start for harnesses.
  MutationHints hints;                // Global mutation heuristics.
};

// Fully resolved ISA definition used by the mutator.
struct ISAConfig {
  std::string isa_name;                                    // Canonical ISA identifier.
  uint32_t base_width = 0;                                 // Native instruction width in bits.
  uint32_t register_count = 0;                             // Count of general registers.
  ISAStatics defaults;                                     // ISA-wide defaults and hints.
  std::unordered_map<std::string, FieldEncoding> fields;   // Field definitions by name.
  std::unordered_map<std::string, FormatSpec> formats;     // Format definitions by name.
  std::vector<InstructionSpec> instructions;               // Instruction catalogue.
};

// Parameters describing where and how to locate schema sources on disk.
struct SchemaLocator {
  std::string root_dir;      // Base directory containing ISA schema files.
  std::string isa_name;      // ISA identifier to load (e.g. rv32imc).
  std::string map_path;      // Optional map file path overriding default includes.
  std::string override_path; // Optional direct schema override to load instead.
};

// Loads an ISA configuration described by the locator and returns a populated model.
ISAConfig load_isa_config(const SchemaLocator &locator);

// Convenience overload allowing call sites to specify root/ISA/override directly.
inline ISAConfig load_isa_config(const std::string &root_dir,
                                 const std::string &isa_name,
                                 const std::string &override_path = "") {
  SchemaLocator locator{root_dir, isa_name, std::string{}, override_path};
  return load_isa_config(locator);
}

} // namespace fuzz::isa
