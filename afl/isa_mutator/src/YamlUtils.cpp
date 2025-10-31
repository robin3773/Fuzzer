#include <fuzz/isa/YamlUtils.hpp>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace fuzz::isa {
namespace yaml_utils {

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
  YAML::Node val = it->second;
      if (!key.IsScalar())
        continue;
      std::string key_text = key.Scalar();
      if (key_text == "<<") {
        if (val.IsSequence()) {
          for (auto nested : val)
            merge_nodes(base, nested);
        } else {
          merge_nodes(base, val);
        }
        continue;
      }
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

std::string strip_quotes(std::string s) {
  boost::algorithm::trim_if(s, boost::algorithm::is_any_of("\"'"));
  return s;
}

std::vector<std::string> split_lines(const std::string &text) {
  std::vector<std::string> lines;
  boost::algorithm::split(lines, text, boost::algorithm::is_any_of("\n"), boost::algorithm::token_compress_off);
  return lines;
}

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

std::string read_file_to_string(const fs::path &path) {
  std::ifstream ifs(path);
  if (!ifs.is_open())
    throw std::runtime_error("Failed to open schema file: " + path.string());
  return {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
}

std::vector<std::pair<std::string, std::string>> extract_anchor_blocks(const std::string &text) {
  std::vector<std::pair<std::string, std::string>> blocks;
  std::vector<std::string> lines = split_lines(text);

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
      blocks.emplace_back(anchor_name, block.str());
  }

  return blocks;
}

std::string build_anchor_context(const std::vector<std::pair<std::string, std::string>> &anchors) {
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

} // namespace yaml_utils
} // namespace fuzz::isa
