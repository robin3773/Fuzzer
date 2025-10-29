#include <fuzz/isa/Loader.hpp>

#include <yaml-cpp/yaml.h>

#include <cctype>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace fuzz::isa {
namespace {

uint64_t parse_integer(const YAML::Node &node) {
  if (!node)
    throw std::runtime_error("Missing scalar value in ISA schema");

  if (node.IsScalar()) {
    std::string text = node.as<std::string>();
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])))
      ++begin;
    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
      --end;
    std::string trimmed = text.substr(begin, end - begin);
    if (trimmed.empty())
      return 0;

    int base = 10;
    size_t offset = 0;
    if (trimmed.size() > 2 && trimmed[0] == '0' && (trimmed[1] == 'x' || trimmed[1] == 'X')) {
      base = 16;
      offset = 2;
    } else if (trimmed.size() > 2 && trimmed[0] == '0' && (trimmed[1] == 'b' || trimmed[1] == 'B')) {
      base = 2;
      offset = 2;
    }
    std::string digits = trimmed.substr(offset);
    if (base == 10)
      return std::stoll(trimmed, nullptr, 10);

    uint64_t value = 0;
    for (char ch : digits) {
      value <<= (base == 16) ? 4 : 1;
      if (std::isdigit(static_cast<unsigned char>(ch)))
        value |= static_cast<uint64_t>(ch - '0');
      else if (ch >= 'a' && ch <= 'f')
        value |= static_cast<uint64_t>(10 + (ch - 'a'));
      else if (ch >= 'A' && ch <= 'F')
        value |= static_cast<uint64_t>(10 + (ch - 'A'));
      else
        throw std::runtime_error("Invalid digit in numeric literal: " + trimmed);
      if (base == 2 && value > (1ull << 63))
        value &= 0xFFFFFFFFFFFFFFFFull;
    }
    return value;
  }

  if (node.IsScalar() || node.IsNull())
    return 0;

  return node.as<uint64_t>();
}

void merge_nodes(YAML::Node &base, const YAML::Node &overlay) {
  if (!overlay)
    return;
  if (!base || base.IsNull()) {
    base = overlay;
    return;
  }
  if (overlay.IsMap()) {
    if (!base.IsMap()) {
      base = YAML::Clone(overlay);
      return;
    }
    for (auto it = overlay.begin(); it != overlay.end(); ++it) {
      const YAML::Node &key = it->first;
      const YAML::Node &val = it->second;
      YAML::Node target = base[key.Scalar()];
      merge_nodes(target, val);
      base[key.Scalar()] = target;
    }
    return;
  }
  base = overlay;
}

YAML::Node load_with_includes(const fs::path &path, std::set<fs::path> &stack) {
  fs::path norm = fs::weakly_canonical(path);
  if (!stack.insert(norm).second)
    throw std::runtime_error("Recursive include detected while loading " + norm.string());

  YAML::Node node = YAML::LoadFile(norm.string());
  YAML::Node result = YAML::Clone(node);

  auto includeNode = node["include"];
  if (includeNode) {
    if (includeNode.IsScalar()) {
      fs::path include_path = norm.parent_path() / includeNode.as<std::string>();
      merge_nodes(result, load_with_includes(include_path, stack));
    } else if (includeNode.IsSequence()) {
      for (auto inc : includeNode) {
        fs::path include_path = norm.parent_path() / inc.as<std::string>();
        merge_nodes(result, load_with_includes(include_path, stack));
      }
    }
  }

  stack.erase(norm);
  return result;
}

FieldEncoding parse_field(const std::string &name, const YAML::Node &node) {
  FieldEncoding enc;
  enc.name = name;
  if (node["signed"])
    enc.is_signed = node["signed"].as<bool>();

  uint32_t width_from_segments = 0;
  uint32_t running_offset = 0;
  if (node["segments"]) {
    const YAML::Node &segments = node["segments"];
    if (!segments.IsSequence())
      throw std::runtime_error("Field '" + name + "' segments must be a sequence");
    for (auto segNode : segments) {
      FieldSegment seg;
      seg.word_lsb = segNode["lsb"].as<uint32_t>();
      seg.width = segNode["width"].as<uint32_t>();
      if (segNode["value_lsb"]) {
        seg.value_lsb = segNode["value_lsb"].as<uint32_t>();
      } else {
        seg.value_lsb = running_offset;
      }
      running_offset = seg.value_lsb + seg.width;
      width_from_segments += seg.width;
      enc.segments.push_back(seg);
    }
  }
  if (node["width"])
    enc.width = node["width"].as<uint32_t>();
  else
    enc.width = width_from_segments;

  return enc;
}

FormatSpec parse_format(const std::string &name, const YAML::Node &node) {
  FormatSpec fmt;
  fmt.name = name;
  if (node["width"])
    fmt.width = node["width"].as<uint32_t>();
  if (node["fields"]) {
    const YAML::Node &fields = node["fields"];
    if (!fields.IsSequence())
      throw std::runtime_error("Format '" + name + "' fields must be a sequence");
    for (auto f : fields)
      fmt.fields.emplace_back(f.as<std::string>());
  }
  return fmt;
}

InstructionSpec parse_instruction(const std::string &name, const YAML::Node &node) {
  InstructionSpec spec;
  spec.name = name;
  if (!node["format"])
    throw std::runtime_error("Instruction '" + name + "' missing format");
  spec.format = node["format"].as<std::string>();
  for (auto it = node.begin(); it != node.end(); ++it) {
    std::string key = it->first.as<std::string>();
    if (key == "format")
      continue;
    spec.fixed_fields[key] = static_cast<uint32_t>(parse_integer(it->second));
  }
  return spec;
}

fs::path resolve_schema_path(const std::string &root_dir,
                             const std::string &isa_name,
                             const std::string &override_path) {
  if (!override_path.empty())
    return fs::path(override_path);

  fs::path root = root_dir.empty() ? fs::path("./schemas") : fs::path(root_dir);
  fs::path map_path = root / "isa_map.yaml";
  if (fs::exists(map_path)) {
    YAML::Node map = YAML::LoadFile(map_path.string());
    if (map[isa_name])
      return root / map[isa_name].as<std::string>();
  }

  fs::path candidate = root / (isa_name + ".yaml");
  if (fs::exists(candidate))
    return candidate;

  candidate = root / isa_name;
  if (fs::exists(candidate))
    return candidate;

  throw std::runtime_error("Unable to locate schema for ISA '" + isa_name + "'");
}

} // namespace

ISAConfig load_isa_config(const std::string &root_dir,
                          const std::string &isa_name,
                          const std::string &override_path) {
  fs::path schema_path = resolve_schema_path(root_dir, isa_name, override_path);
  std::set<fs::path> stack;
  YAML::Node root = load_with_includes(schema_path, stack);

  ISAConfig isa;
  isa.isa_name = root["isa"] ? root["isa"].as<std::string>() : isa_name;
  if (root["base_width"])
    isa.base_width = root["base_width"].as<uint32_t>();
  if (root["registers"])
    isa.register_count = root["registers"].as<uint32_t>();

  if (root["fields"]) {
    for (auto it = root["fields"].begin(); it != root["fields"].end(); ++it) {
      std::string name = it->first.as<std::string>();
      isa.fields[name] = parse_field(name, it->second);
    }
  }

  if (root["formats"]) {
    for (auto it = root["formats"].begin(); it != root["formats"].end(); ++it) {
      std::string name = it->first.as<std::string>();
      isa.formats[name] = parse_format(name, it->second);
      if (isa.formats[name].width == 0)
        isa.formats[name].width = isa.base_width;
    }
  }

  if (root["instructions"]) {
    for (auto it = root["instructions"].begin(); it != root["instructions"].end(); ++it) {
      std::string name = it->first.as<std::string>();
      isa.instructions.push_back(parse_instruction(name, it->second));
    }
  }

  if (isa.base_width == 0)
    isa.base_width = 32;
  if (isa.register_count == 0)
    isa.register_count = 32;

  return isa;
}

} // namespace fuzz::isa
