#include <fuzz/isa/IsaLoader.hpp>

#include <yaml-cpp/yaml.h>

#include <fuzz/isa/YamlUtils.hpp>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace fuzz::isa {
namespace {

  // Internal structure for locating schema files
  struct SchemaLocator {
    std::string root_dir;
    std::string isa_name;
    std::string map_path;
  };

  std::vector<fs::path> resolve_schema_sources(const SchemaLocator &locator) {
    if (locator.isa_name.empty())
      throw std::runtime_error("[ERROR] Schema locator missing ISA name");

    fs::path root = locator.root_dir.empty() ? fs::path("./schemas") : fs::path(locator.root_dir);
    fs::path map_path = locator.map_path.empty() ? root / "isa_map.yaml" : fs::path(locator.map_path);
    if (!map_path.is_absolute())
      map_path = root / map_path;

    printf("[INFO] Using ISA map: %s\n", map_path.string().c_str());

    std::vector<fs::path> schema_files;
    std::error_code ec;
    if (fs::exists(map_path, ec)) {
      auto includes = yaml_utils::includes_from_map(map_path, locator.isa_name);
      fs::path base = map_path.parent_path();
      for (const auto &inc : includes) {
        fs::path candidate = fs::path(inc);
        if (!candidate.is_absolute())
          candidate = root / candidate;
        candidate = candidate.lexically_normal();
        if (!fs::exists(candidate, ec)) {
          throw std::runtime_error("[ERROR] Schema include '" + candidate.string() + "' referenced by ISA map not found");
        }
        schema_files.push_back(candidate);
      }
    }

    if (schema_files.empty())
      throw std::runtime_error("[ERROR] Unable to resolve schema sources for ISA '" + locator.isa_name + "'");

    std::vector<fs::path> ordered;
    std::unordered_set<std::string> visited;
    for (auto &schema_file : schema_files)
      yaml_utils::collect_dependencies(schema_file, ordered, visited);

    return ordered;
  }

  uint32_t compute_field_width(const std::vector<FieldSegment> &segments) {
    return std::accumulate(segments.begin(), segments.end(), 0u,
      [](uint32_t max_extent, const FieldSegment &seg) {
        return std::max(max_extent, seg.value_lsb + seg.width);
      });
  }

  FieldSegment parse_segment(const YAML::Node &node, uint32_t default_value_lsb) {
    FieldSegment segment;
    segment.value_lsb = default_value_lsb;

    if (node.IsSequence()) {
      if (node.size() != 2)
        throw std::runtime_error("Segment sequence must contain [lsb, msb]");
      segment.word_lsb = node[0].as<uint32_t>();
      uint32_t msb = node[1].as<uint32_t>();
      if (msb < segment.word_lsb)
        throw std::runtime_error("Segment msb < lsb");
      segment.width = msb - segment.word_lsb + 1;
      return segment;
    }

    if (!node.IsMap())
      throw std::runtime_error("Unexpected segment node type");

    if (node["value_lsb"])
      segment.value_lsb = node["value_lsb"].as<uint32_t>();
    if (node["lsb"]) {
      segment.word_lsb = node["lsb"].as<uint32_t>();
    }
    if (node["width"]) {
      segment.width = node["width"].as<uint32_t>();
    }

    if (node["bits"]) {
      const YAML::Node &bits = node["bits"];
      if (!bits.IsSequence() || bits.size() != 2)
        throw std::runtime_error("Segment bits must contain [lsb, msb]");
      segment.word_lsb = bits[0].as<uint32_t>();
      uint32_t msb = bits[1].as<uint32_t>();
      if (msb < segment.word_lsb)
        throw std::runtime_error("Segment msb < lsb");
      segment.width = msb - segment.word_lsb + 1;
    }

    if (!segment.width)
      throw std::runtime_error("Segment missing width definition");

    return segment;
  }

  FieldKind deduce_field_kind(const std::string &raw) {
    std::string lower = raw;
    boost::algorithm::to_lower(lower);

    auto contains = [&](const char *token) {
      return lower.find(token) != std::string::npos;
    };

    if (lower == "opcode" || contains("opcode"))
      return FieldKind::Opcode;
    if (lower == "enum" || contains("funct") || contains("flag"))
      return FieldKind::Enum;
    if (contains("imm"))
      return FieldKind::Immediate;
    if (contains("pred"))
      return FieldKind::Predicate;
    if (contains("mem"))
      return FieldKind::Memory;
    if (contains("csr"))
      return FieldKind::Enum;
    if (contains("freg") || contains("fp_reg"))
      return FieldKind::Floating;
    if (contains("reg") || lower == "rs" || lower == "rd" || lower == "rt")
      return FieldKind::Register;
    if (lower == "aq_rl")
      return FieldKind::Enum;
    return FieldKind::Unknown;
  }

  FieldEncoding parse_field(const std::string &name, const YAML::Node &node) {
    FieldEncoding encoding;
    encoding.name = name;
    if (node["signed"])
      encoding.is_signed = node["signed"].as<bool>();
    if (node["width"])
      encoding.width = node["width"].as<uint32_t>();
    if (node["type"]) {
      encoding.raw_type = node["type"].as<std::string>();
      encoding.kind = deduce_field_kind(encoding.raw_type);
    }

    auto append_segments = [&](const YAML::Node &source) {
      if (!source.IsSequence())
        throw std::runtime_error("Field '" + name + "' segments must be a sequence");
      uint32_t next_value_lsb = 0;
      if (!encoding.segments.empty())
        next_value_lsb = encoding.segments.back().value_lsb + encoding.segments.back().width;
      for (auto entry : source) {
        FieldSegment seg = parse_segment(entry, next_value_lsb);
        next_value_lsb = seg.value_lsb + seg.width;
        encoding.segments.push_back(seg);
      }
    };

    if (node["segments"]) {
      append_segments(node["segments"]);
    } else if (node["bits"]) {
      const YAML::Node &bits = node["bits"];
      if (bits.IsSequence() && bits.size() == 2 && bits[0].IsScalar()) {
        FieldSegment seg = parse_segment(bits, 0);
        if (node["value_lsb"]) seg.value_lsb = node["value_lsb"].as<uint32_t>();
        encoding.segments.push_back(seg);
      } else {
        append_segments(bits);
      }
    } else if (node["lsb"] && node["width"]) {
      FieldSegment seg;
      seg.word_lsb = node["lsb"].as<uint32_t>();
      seg.width = node["width"].as<uint32_t>();
      if (node["value_lsb"])
        seg.value_lsb = node["value_lsb"].as<uint32_t>();
      encoding.segments.push_back(seg);
    }

    if (!encoding.segments.empty() && !encoding.width)
      encoding.width = compute_field_width(encoding.segments);

    if (encoding.segments.empty() && encoding.width == 0)
      throw std::runtime_error("Field '" + name + "' missing width/segments definition");

    if (encoding.kind == FieldKind::Unknown)
      encoding.kind = deduce_field_kind(name);

    return encoding;
  }

  bool segments_equal(const std::vector<FieldSegment> &lhs,
                      const std::vector<FieldSegment> &rhs) {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin(),
      [](const FieldSegment &a, const FieldSegment &b) {
        return a.word_lsb == b.word_lsb && a.width == b.width && a.value_lsb == b.value_lsb;
      });
  }

  void ensure_field(std::unordered_map<std::string, FieldEncoding> &fields,
                    const FieldEncoding &candidate) {
    auto [it, inserted] = fields.emplace(candidate.name, candidate);
    if (inserted)
      return;

    FieldEncoding &existing = it->second;
    if (existing.segments.empty() && !candidate.segments.empty()) {
      existing.segments = candidate.segments;
    }
    if (existing.width == 0)
      existing.width = candidate.width;
    if (candidate.is_signed)
      existing.is_signed = true;

    if (!candidate.segments.empty() && !segments_equal(existing.segments, candidate.segments))
      return;
  }

  FormatSpec parse_format(const std::string &name,
                          const YAML::Node &node,
                          std::unordered_map<std::string, FieldEncoding> &fields) {
    FormatSpec fmt;
    fmt.name = name;
    if (node["width"])
      fmt.width = node["width"].as<uint32_t>();
    if (!node["fields"])
      throw std::runtime_error("Format '" + name + "' missing fields");

    const YAML::Node &field_list = node["fields"];
    if (!field_list.IsSequence())
      throw std::runtime_error("Format '" + name + "' fields must be a sequence");

    for (auto entry : field_list) {
      if (entry.IsScalar()) {
        fmt.fields.emplace_back(entry.as<std::string>());
        continue;
      }
      if (!entry.IsMap())
        throw std::runtime_error("Format '" + name + "' has invalid field entry");
      if (!entry["name"])
        throw std::runtime_error("Inline field definition missing name in format '" + name + "'");
      std::string field_name = entry["name"].as<std::string>();
      fmt.fields.emplace_back(field_name);
      FieldEncoding derived = parse_field(field_name, entry);
      ensure_field(fields, derived);
    }

    return fmt;
  }

  InstructionSpec parse_instruction(const std::string &name, const YAML::Node &node) {
    InstructionSpec spec;
    spec.name = name;
    if (!node["format"])
      throw std::runtime_error("Instruction '" + name + "' missing format");
    spec.format = node["format"].as<std::string>();
    
    if (auto fixed = node["fixed"]; fixed && fixed.IsMap()) {
      for (auto it = fixed.begin(); it != fixed.end(); ++it) {
        spec.fixed_fields[it->first.as<std::string>()] = static_cast<uint32_t>(yaml_utils::parse_integer(it->second));
      }
    }
    
    const std::unordered_set<std::string> skip_keys = {
      "format", "fixed", "description", "comment", "notes", "tags", "weight", "probability"
    };
    
    for (auto it = node.begin(); it != node.end(); ++it) {
      std::string key = it->first.as<std::string>();
      if (skip_keys.count(key) || !it->second.IsScalar())
        continue;
      spec.fixed_fields[key] = static_cast<uint32_t>(yaml_utils::parse_integer(it->second));
    }
    return spec;
  }

} // namespace

ISAConfig load_isa_config_impl(const SchemaLocator &locator) {
  auto sources = resolve_schema_sources(locator);
  if (sources.empty())
    throw std::runtime_error("No schema files resolved for ISA '" + locator.isa_name + "'");

  std::vector<std::pair<std::string, std::string>> anchor_library;
  YAML::Node merged;

  for (const auto &source : sources) {
    std::string content = yaml_utils::read_file_to_string(source);
    std::string context = yaml_utils::build_anchor_context(anchor_library);
    std::string combined = context + content;

    YAML::Node node;
    try {
      node = YAML::Load(combined);
    } catch (const YAML::ParserException &ex) {
      throw std::runtime_error("Failed to parse schema file '" + source.string() + "': " + ex.what());
    }

    if (node["__anchors"])
      node.remove("__anchors");

    auto anchors = yaml_utils::extract_anchor_blocks(content);
    for (auto &entry : anchors) {
      auto it = std::find_if(anchor_library.begin(), anchor_library.end(),
        [&](const auto &kv) { return kv.first == entry.first; });
      if (it != anchor_library.end()) {
        *it = entry;
      } else {
        anchor_library.push_back(entry);
      }
    }

    yaml_utils::merge_nodes(merged, node);
  }

  if (!merged || !merged.IsMap())
    throw std::runtime_error("Merged schema for ISA '" + locator.isa_name + "' is empty");

  ISAConfig isa;
  isa.isa_name = locator.isa_name;

  if (merged["isa"])
    isa.isa_name = merged["isa"].as<std::string>();
  if (merged["meta"]) {
    const YAML::Node &meta = merged["meta"];
    if (meta["isa_name"]) {
      std::string meta_name = meta["isa_name"].as<std::string>();
      if (!meta_name.empty())
        isa.isa_name = meta_name;
    }
    if (meta["endianness"])
      isa.defaults.endianness = meta["endianness"].as<std::string>();
    if (meta["default_pc"])
      isa.defaults.default_pc = yaml_utils::parse_integer(meta["default_pc"]);
  }

  if (merged["defaults"]) {
    const YAML::Node &defaults = merged["defaults"];
    if (defaults["register_count"])
      isa.register_count = defaults["register_count"].as<uint32_t>();
    if (defaults["default_pc"])
      isa.defaults.default_pc = yaml_utils::parse_integer(defaults["default_pc"]);
    if (defaults["endianness"])
      isa.defaults.endianness = defaults["endianness"].as<std::string>();
    if (defaults["mutation_hints"]) {
      const YAML::Node &hints = defaults["mutation_hints"];
      if (hints["reg_prefers_zero_one_hot"])
        isa.defaults.hints.reg_prefers_zero_one_hot = hints["reg_prefers_zero_one_hot"].as<bool>();
      if (hints["signed_immediates_bias"])
        isa.defaults.hints.signed_immediates_bias = hints["signed_immediates_bias"].as<bool>();
      if (hints["align_load_store"])
        isa.defaults.hints.align_load_store = hints["align_load_store"].as<uint32_t>();
    }
  }

  if (isa.register_count == 0) {
    if (merged["registers"])
      isa.register_count = merged["registers"].as<uint32_t>();
    else if (merged["register_count"])
      isa.register_count = merged["register_count"].as<uint32_t>();
  }

  if (merged["base_width"])
    isa.base_width = merged["base_width"].as<uint32_t>();

  if (merged["fields"]) {
    const YAML::Node &fields = merged["fields"];
    for (auto it = fields.begin(); it != fields.end(); ++it) {
      if (!it->first.IsScalar())
        continue;
      std::string name = it->first.as<std::string>();
      if (name == "<<")
        continue;
      isa.fields[name] = parse_field(name, it->second);
    }
  }

  if (merged["formats"]) {
    const YAML::Node &formats = merged["formats"];
    for (auto it = formats.begin(); it != formats.end(); ++it) {
      if (!it->first.IsScalar())
        continue;
      std::string name = it->first.as<std::string>();
      if (name == "<<")
        continue;
      isa.formats[name] = parse_format(name, it->second, isa.fields);
    }
  }

  if (merged["instructions"]) {
    const YAML::Node &inst = merged["instructions"];
    for (auto it = inst.begin(); it != inst.end(); ++it) {
      if (!it->first.IsScalar())
        continue;
      std::string name = it->first.as<std::string>();
      if (name == "<<")
        continue;
      isa.instructions.push_back(parse_instruction(name, it->second));
    }
  }

  uint32_t max_format_width = std::accumulate(isa.formats.begin(), isa.formats.end(), 0u,
    [&isa](uint32_t max_width, auto &kv) {
      if (kv.second.width == 0)
        kv.second.width = isa.base_width;
      return std::max(max_width, kv.second.width);
    });

  if (isa.base_width == 0)
    isa.base_width = max_format_width ? max_format_width : 32;
  if (isa.base_width == 0)
    isa.base_width = 32;
  if (isa.register_count == 0)
    isa.register_count = 32;

  return isa;
}

ISAConfig load_isa_config(const std::string &isa_name) {
  // Read schema directory from environment variable
  const char *schema_dir_env = std::getenv("SCHEMA_DIR");
  
  SchemaLocator locator {
    .root_dir = schema_dir_env ? schema_dir_env : "./schemas",
    .isa_name = isa_name,
    .map_path = "isa_map.yaml"
  };
  
  return load_isa_config_impl(locator);
}

} // namespace fuzz::isa
