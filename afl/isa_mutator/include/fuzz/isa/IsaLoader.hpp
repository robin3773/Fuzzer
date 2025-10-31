#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace fuzz::isa {

// Describes how a logical field spills into the physical instruction word.
struct FieldSegment {
  uint32_t word_lsb = 0;
  uint32_t width = 0;
  uint32_t value_lsb = 0;
};

// High-level classification for a decoded field to guide mutations.
enum class FieldKind : uint8_t {
  Unknown = 0,
  Opcode,
  Enum,
  Immediate,
  Predicate,
  Memory,
  Register,
  Floating,
};

// Fully describes a logical field and its layout segments.
struct FieldEncoding {
  std::string name;
  uint32_t width = 0;
  bool is_signed = false;
  std::vector<FieldSegment> segments;
  FieldKind kind = FieldKind::Unknown;
  std::string raw_type;
};

// Format definition mapping fields into a fixed instruction width.
struct FormatSpec {
  std::string name;
  uint32_t width = 0;
  std::vector<std::string> fields;
};

// Instruction entry tying a format to specific fixed field values.
struct InstructionSpec {
  std::string name;
  std::string format;
  std::unordered_map<std::string, uint32_t> fixed_fields;
};

// Tunables that bias random selection for better fuzz coverage.
struct MutationHints {
  bool reg_prefers_zero_one_hot = false;
  bool signed_immediates_bias = false;
  uint32_t align_load_store = 0;
};

struct ISAConfigDefaults {
  std::string endianness = "little";
  int64_t default_pc = 0;
  MutationHints hints;
};

// Fully resolved ISA definition used by the mutator.
struct ISAConfig {
  std::string isa_name;
  uint32_t base_width = 0;
  uint32_t register_count = 0;
  ISAConfigDefaults defaults;
  std::unordered_map<std::string, FieldEncoding> fields;
  std::unordered_map<std::string, FormatSpec> formats;
  std::vector<InstructionSpec> instructions;
};

// Parameters describing where and how to locate schema sources on disk.
struct SchemaLocator {
  std::string root_dir;
  std::string isa_name;
  std::string map_path;
  std::string override_path;
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
