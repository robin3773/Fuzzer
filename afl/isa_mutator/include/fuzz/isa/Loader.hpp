#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace fuzz::isa {

struct FieldSegment {
  uint32_t word_lsb = 0;   // bit position in instruction word
  uint32_t width = 0;      // number of bits in this segment
  uint32_t value_lsb = 0;  // bit position in the logical field value
};

struct FieldEncoding {
  std::string name;
  uint32_t width = 0;
  bool is_signed = false;
  std::vector<FieldSegment> segments;
};

struct FormatSpec {
  std::string name;
  uint32_t width = 0;
  std::vector<std::string> fields;
};

struct InstructionSpec {
  std::string name;
  std::string format;
  std::unordered_map<std::string, uint32_t> fixed_fields;
};

struct ISAConfig {
  std::string isa_name;
  uint32_t base_width = 32;
  uint32_t register_count = 32;
  std::unordered_map<std::string, FieldEncoding> fields;
  std::unordered_map<std::string, FormatSpec> formats;
  std::vector<InstructionSpec> instructions;
};

ISAConfig load_isa_config(const std::string &root_dir,
                          const std::string &isa_name,
                          const std::string &override_path = "");

} // namespace fuzz::isa
