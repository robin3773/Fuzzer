#include <fuzz/isa/Loader.hpp>

#include <yaml-cpp/yaml.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cctype>
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

  // NOTE - This function is designed to extract and convert a scalar value from a YAML node into a uint64_t integer
  // handling decimal, hexadecimal (0x), and binary (0b) formats.
  uint64_t parse_integer(const YAML::Node &node) {
    if (!node || node.IsNull())
      return 0;

    if (!node.IsScalar())
      return node.as<uint64_t>();

    std::string text = node.as<std::string>();
    boost::algorithm::trim(text);
    if (text.empty())
      return 0;

    size_t sign_pos = (text[0] == '+' || text[0] == '-') ? 1u : 0u;

    int base = 10;
    size_t offset = sign_pos;
    if (text.size() > offset + 1 && text[offset] == '0') {
      char marker = text[offset + 1];
      if (marker == 'x' || marker == 'X') {
        base = 16;
        offset += 2;
      } else if (marker == 'b' || marker == 'B') {
        base = 2;
        offset += 2;
      }
    }

    std::string digits = text.substr(offset);
    if (digits.empty())
      return 0;

    try {
      if (base == 10)
        return static_cast<uint64_t>(std::stoll(text, nullptr, base));

      uint64_t magnitude = std::stoull(digits, nullptr, base);
      if (sign_pos && text[0] == '-')
        return static_cast<uint64_t>(-static_cast<int64_t>(magnitude));
      return magnitude;
    } catch (const std::exception &) {
      throw std::runtime_error("Invalid numeric literal: " + text);
    }
  }

  //NOTE - This function merges two YAML nodes, with the overlay node taking precedence over the base node.
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
        if (!key.IsScalar())
          continue;
        std::string key_text = key.Scalar();
        if (key_text.rfind("__", 0) == 0)
          continue;
        YAML::Node target = base[key_text];
        merge_nodes(target, val);
        base[key_text] = target;
      }
      return;
    }
    base = overlay;
  }

  // NOTE - This function removes surrounding quotes from a string.
  std::string strip_quotes(std::string s) {
    boost::algorithm::trim_if(s, boost::algorithm::is_any_of("\"'"));
    return s;
  }

  //NOTE - This function splits a multi-line string into individual lines.
  std::vector<std::string> split_lines(const std::string &text) {
    std::vector<std::string> lines;
    boost::algorithm::split(lines, text, boost::algorithm::is_any_of("\n"), boost::algorithm::token_compress_off);
    return lines;
  }

// NOTE - This function extracts file paths associated with a specific key from a YAML-formatted string.
  std::vector<std::string> extract_paths_for_key(const std::string &text,
                                                const std::string &key) {
    std::vector<std::string> result;
    std::vector<std::string> lines = split_lines(text);
    const std::string needle = key + ":";

    for (size_t i = 0; i < lines.size(); ++i) {
      const std::string &line = lines[i];
      std::string trimmed = line;
      boost::algorithm::trim_left(trimmed);
      if (trimmed.compare(0, needle.size(), needle) != 0)
        continue;

      size_t indent = line.size() - trimmed.size();
      std::string rhs = trimmed.substr(needle.size());
      boost::algorithm::trim(rhs);
      size_t comment_pos = rhs.find('#');
      if (comment_pos != std::string::npos) {
        rhs = rhs.substr(0, comment_pos);
        boost::algorithm::trim(rhs);
      }

      if (!rhs.empty()) {
        if (rhs.front() == '[' && rhs.back() == ']') {
          std::string inner = rhs.substr(1, rhs.size() - 2);
          std::stringstream ss(inner);
          std::string item;
          while (std::getline(ss, item, ',')) {
            boost::algorithm::trim(item);
            item = strip_quotes(item);
            if (!item.empty())
              result.push_back(item);
          }
        } else {
          std::string value = strip_quotes(rhs);
          if (!value.empty())
            result.push_back(value);
        }
        continue;
      }

      size_t j = i + 1;
      while (j < lines.size()) {
        const std::string &next = lines[j];
        std::string next_trimmed = next;
        boost::algorithm::trim_left(next_trimmed);
        if (next_trimmed.empty() || next_trimmed[0] == '#') {
          ++j;
          continue;
        }
        size_t next_indent = next.size() - next_trimmed.size();
        if (next_indent <= indent)
          break;
        if (next_trimmed[0] == '-') {
          std::string value = next_trimmed.substr(1);
          boost::algorithm::trim(value);
          size_t comment = value.find('#');
          if (comment != std::string::npos) {
            value = value.substr(0, comment);
            boost::algorithm::trim(value);
          }
          value = strip_quotes(value);
          if (!value.empty())
            result.push_back(value);
          ++j;
          continue;
        }
        break;
      }
    }

    return result;
  }

  // NOTE - This function reads the entire contents of a file into a string.
  std::string read_file_to_string(const fs::path &path) {
    std::ifstream ifs(path);
    if (!ifs.is_open())
      throw std::runtime_error("Failed to open schema file: " + path.string());
    return {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
  }

  std::map<std::string, std::string> extract_anchor_blocks(const std::string &text) {
    std::map<std::string, std::string> blocks;
    std::vector<std::string> lines = split_lines(text);
    const std::string whitespace = " \t";

    for (size_t i = 0; i < lines.size(); ++i) {
      const std::string &line = lines[i];
      size_t anchor_pos = line.find('&');
      if (anchor_pos == std::string::npos)
        continue;

      size_t comment = line.find('#');
      if (comment != std::string::npos && anchor_pos > comment)
        continue;

      size_t start = anchor_pos + 1;
      size_t end = start;
      while (end < line.size() && (std::isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_' || line[end] == '-'))
        ++end;
      if (end == start)
        continue;

      std::string anchor_name = line.substr(start, end - start);
      size_t indent = 0;
      while (indent < line.size() && (line[indent] == ' ' || line[indent] == '\t'))
        ++indent;

      std::ostringstream block;
      block << line << '\n';
      size_t j = i + 1;
      while (j < lines.size()) {
        const std::string &next = lines[j];
        std::string trimmed = next;
        boost::algorithm::trim_left(trimmed);
        if (trimmed.empty()) {
          block << next << '\n';
          ++j;
          continue;
        }
        size_t next_indent = next.size() - trimmed.size();
        if (next_indent <= indent && trimmed[0] != '-')
          break;
        block << next << '\n';
        ++j;
      }

      if (!anchor_name.empty())
        blocks[anchor_name] = block.str();
    }

    return blocks;
  }

  std::string build_anchor_context(const std::map<std::string, std::string> &anchors) {
    if (anchors.empty())
      return {};

    std::ostringstream ctx;
    ctx << "__anchors:\n";
    for (const auto &[key, content] : anchors) {
      std::istringstream in(content);
      std::string line;
      while (std::getline(in, line)) {
        ctx << "  " << line << '\n';
      }
    }
    ctx << '\n';
    return ctx.str();
  }

  void collect_dependencies(const fs::path &path,
                            std::vector<fs::path> &ordered,
                            std::unordered_set<std::string> &visited) {
    fs::path norm = fs::weakly_canonical(path);
    if (!visited.insert(norm.string()).second)
      return;

    std::string content = read_file_to_string(norm);
    
    auto process_key = [&](const std::string &key) {
      for (const auto &rel : extract_paths_for_key(content, key)) {
        collect_dependencies(norm.parent_path() / fs::path(rel).lexically_normal(), ordered, visited);
      }
    };
    
    process_key("extends");
    process_key("include");
    ordered.push_back(norm);
  }

  std::vector<std::string> includes_from_map(const fs::path &map_path,
                                            const std::string &isa_name) {
    std::vector<std::string> includes;
    YAML::Node map;
    try {
      map = YAML::LoadFile(map_path.string());
    } catch (const YAML::Exception &ex) {
      throw std::runtime_error("Failed to parse ISA map '" + map_path.string() + "': " + ex.what());
    }
    if (!map)
      return includes;

    if (map["isa_families"]) {
      YAML::Node families = map["isa_families"];
      for (auto family_it = families.begin(); family_it != families.end(); ++family_it) {
        YAML::Node variants = family_it->second;
        YAML::Node entry = variants[isa_name];
        if (!entry)
          continue;
        YAML::Node include_list = entry["includes"];
        if (include_list && include_list.IsSequence()) {
          for (auto inc : include_list)
            includes.push_back(inc.as<std::string>());
        } else if (entry.IsSequence()) {
          for (auto inc : entry)
            includes.push_back(inc.as<std::string>());
        } else if (entry.IsScalar()) {
          includes.push_back(entry.as<std::string>());
        }
        if (!includes.empty())
          return includes;
      }
    }

    if (map[isa_name]) {
      YAML::Node entry = map[isa_name];
      if (entry.IsScalar()) {
        includes.push_back(entry.as<std::string>());
      } else if (entry["includes"] && entry["includes"].IsSequence()) {
        for (auto inc : entry["includes"])
          includes.push_back(inc.as<std::string>());
      } else if (entry.IsSequence()) {
        for (auto inc : entry)
          includes.push_back(inc.as<std::string>());
      }
    }

    return includes;
  }

  std::vector<fs::path> resolve_schema_sources(const SchemaLocator &locator) {
    if (locator.isa_name.empty())
      throw std::runtime_error("Schema locator missing ISA name");

    fs::path root = locator.root_dir.empty() ? fs::path("./schemas") : fs::path(locator.root_dir);
    fs::path override_path = locator.override_path.empty() ? fs::path() : fs::path(locator.override_path);
    if (!override_path.empty() && !override_path.is_absolute())
      override_path = root / override_path;

    if (!override_path.empty()) {
      std::vector<fs::path> ordered;
      std::unordered_set<std::string> visited;
      collect_dependencies(override_path, ordered, visited);
      if (ordered.empty())
        throw std::runtime_error("Override schema file did not produce any sources");
      return ordered;
    }

    fs::path map_path = locator.map_path.empty() ? root / "isa_map.yaml" : fs::path(locator.map_path);
    if (!map_path.is_absolute())
      map_path = root / map_path;

    std::vector<fs::path> seeds;
    std::error_code ec;
    if (fs::exists(map_path, ec)) {
      auto includes = includes_from_map(map_path, locator.isa_name);
      fs::path base = map_path.parent_path();
      for (const auto &inc : includes) {
        fs::path candidate = fs::path(inc);
        if (!candidate.is_absolute())
          candidate = root / candidate;
        candidate = candidate.lexically_normal();
        if (!fs::exists(candidate, ec)) {
          throw std::runtime_error("Schema include '" + candidate.string() + "' referenced by ISA map not found");
        }
        seeds.push_back(candidate);
      }
    }

    if (seeds.empty()) {
      std::vector<fs::path> candidates = {
          root / locator.isa_name,
          root / (locator.isa_name + ".yaml"),
          root / "riscv" / (locator.isa_name + ".yaml"),
          root / "riscv" / locator.isa_name};
      for (const auto &candidate : candidates) {
        if (fs::exists(candidate, ec)) {
          seeds.push_back(candidate.lexically_normal());
          break;
        }
      }
    }

    if (seeds.empty())
      throw std::runtime_error("Unable to resolve schema sources for ISA '" + locator.isa_name + "'");

    std::vector<fs::path> ordered;
    std::unordered_set<std::string> visited;
    for (auto &seed : seeds)
      collect_dependencies(seed, ordered, visited);

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
      throw std::runtime_error("Conflicting definition for field '" + candidate.name + "'");
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
        spec.fixed_fields[it->first.as<std::string>()] = static_cast<uint32_t>(parse_integer(it->second));
      }
    }
    
    const std::unordered_set<std::string> skip_keys = {
      "format", "fixed", "description", "comment", "notes", "tags", "weight", "probability"
    };
    
    for (auto it = node.begin(); it != node.end(); ++it) {
      std::string key = it->first.as<std::string>();
      if (skip_keys.count(key) || !it->second.IsScalar())
        continue;
      spec.fixed_fields[key] = static_cast<uint32_t>(parse_integer(it->second));
    }
    return spec;
  }

} // namespace

ISAConfig load_isa_config(const SchemaLocator &locator) {
  auto sources = resolve_schema_sources(locator);
  if (sources.empty())
    throw std::runtime_error("No schema files resolved for ISA '" + locator.isa_name + "'");

  std::map<std::string, std::string> anchor_library;
  YAML::Node merged;

  for (const auto &source : sources) {
    std::string content = read_file_to_string(source);
    std::string context = build_anchor_context(anchor_library);
    std::string combined = context + content;

    YAML::Node node;
    try {
      node = YAML::Load(combined);
    } catch (const YAML::ParserException &ex) {
      throw std::runtime_error("Failed to parse schema file '" + source.string() + "': " + ex.what());
    }

    if (node["__anchors"])
      node.remove("__anchors");

    auto anchors = extract_anchor_blocks(content);
    for (auto &entry : anchors)
      anchor_library[entry.first] = entry.second;

    merge_nodes(merged, node);
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
      isa.defaults.default_pc = parse_integer(meta["default_pc"]);
  }

  if (merged["defaults"]) {
    const YAML::Node &defaults = merged["defaults"];
    if (defaults["register_count"])
      isa.register_count = defaults["register_count"].as<uint32_t>();
    if (defaults["default_pc"])
      isa.defaults.default_pc = parse_integer(defaults["default_pc"]);
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
      isa.fields[name] = parse_field(name, it->second);
    }
  }

  if (merged["formats"]) {
    const YAML::Node &formats = merged["formats"];
    for (auto it = formats.begin(); it != formats.end(); ++it) {
      if (!it->first.IsScalar())
        continue;
      std::string name = it->first.as<std::string>();
      isa.formats[name] = parse_format(name, it->second, isa.fields);
    }
  }

  if (merged["instructions"]) {
    const YAML::Node &inst = merged["instructions"];
    for (auto it = inst.begin(); it != inst.end(); ++it) {
      if (!it->first.IsScalar())
        continue;
      std::string name = it->first.as<std::string>();
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

} // namespace fuzz::isa
