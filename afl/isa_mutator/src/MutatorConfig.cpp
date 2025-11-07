#include <fuzz/mutator/MutatorConfig.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <fuzz/Debug.hpp>
#include <stdexcept>
#include <string>

#include <yaml-cpp/yaml.h>

namespace {

using fuzz::mutator::Config;
using fuzz::mutator::Strategy;

Strategy parse_strategy_token(const std::string &token, Strategy current) {
  if (token.empty())
    return current;

  std::string upper = token;
  std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char ch) {
    return static_cast<char>(std::toupper(ch));
  });

  if (upper == "RAW")
    return Strategy::RAW;
  if (upper == "IR")
    return Strategy::IR;
  if (upper == "HYBRID")
    return Strategy::HYBRID;
  if (upper == "AUTO")
    return Strategy::AUTO;

  bool numeric = !upper.empty() &&
                 std::all_of(upper.begin(), upper.end(), [](unsigned char ch) {
                   return std::isdigit(ch) != 0;
                 });
  if (numeric) {
    int value = std::stoi(upper);
    switch (value) {
    case 0:
      return Strategy::RAW;
    case 1:
      return Strategy::IR;
    case 2:
      return Strategy::HYBRID;
    case 3:
      return Strategy::AUTO;
    default:
      break;
    }
  }

  return current;
}

Strategy parse_strategy_node(const YAML::Node &node, Strategy current) {
  if (!node)
    return current;

  try {
    int numeric = node.as<int>();
    switch (numeric) {
    case 0:
      return Strategy::RAW;
    case 1:
      return Strategy::IR;
    case 2:
      return Strategy::HYBRID;
    case 3:
      return Strategy::AUTO;
    default:
      break;
    }
  } catch (const YAML::BadConversion &) {
    // Fall through and try parsing as string below.
  }

  try {
    std::string text = node.as<std::string>();
    return parse_strategy_token(text, current);
  } catch (const YAML::BadConversion &) {
    return current;
  }
}

std::string node_to_string(const YAML::Node &node) {
  if (!node)
    return {};
  if (node.IsNull())
    return {};
  return node.as<std::string>();
}

const char *strategy_to_string(Strategy s) {
  switch (s) {
  case Strategy::RAW:
    return "RAW";
  case Strategy::IR:
    return "IR";
  case Strategy::HYBRID:
    return "HYBRID";
  case Strategy::AUTO:
    return "AUTO";
  }
  return "IR";
}

void apply_probability_block(const YAML::Node &node, Config &cfg) {
  if (!node || !node.IsMap())
    return;
  if (auto v = node["decode"]; v)
    cfg.decode_prob = fuzz::mutator::clamp_pct(v.as<int>());
  if (auto v = node["imm_random"]; v)
    cfg.imm_random_prob = fuzz::mutator::clamp_pct(v.as<int>());
}

void apply_weight_block(const YAML::Node &node, Config &cfg) {
  if (!node || !node.IsMap())
    return;
  if (auto v = node["r_base_alu"]; v)
    cfg.r_weight_base_alu = fuzz::mutator::clamp_pct(v.as<int>());
  if (auto v = node["r_m"]; v)
    cfg.r_weight_m = fuzz::mutator::clamp_pct(v.as<int>());
}

void apply_schema_block(const YAML::Node &node, Config &cfg) {
  if (!node || !node.IsMap())
    return;

  auto assign_if_present = [](const YAML::Node &n, const char *key, std::string &out) {
    if (auto v = n[key]; v)
      out = node_to_string(v);
  };

  assign_if_present(node, "isa", cfg.isa_name);
  assign_if_present(node, "isa_name", cfg.isa_name);
}

} // namespace

namespace fuzz::mutator {

bool Config::loadFromFile(const std::string &path) {
  if (path.empty())
    return false;

  namespace fs = std::filesystem;
  std::error_code ec;
  fs::path resolved(path);
  if (!fs::exists(resolved, ec))
    return false;

  YAML::Node root;
  try {
    root = YAML::LoadFile(resolved.string());
  } catch (const YAML::BadFile &ex) {
    throw std::runtime_error("Failed to open mutator config '" + resolved.string() + "': " +
                             ex.what());
  } catch (const YAML::ParserException &ex) {
    throw std::runtime_error("Failed to parse mutator config '" + resolved.string() +
                             "': " + ex.what());
  }

  strategy = parse_strategy_node(root["strategy"], strategy);

  if (auto node = root["verbose"]; node)
    verbose = node.as<bool>();

  if (auto node = root["enable_c"]; node)
    enable_c = node.as<bool>();

  apply_probability_block(root["probabilities"], *this);
  apply_weight_block(root["weights"], *this);
  apply_schema_block(root["schemas"], *this);

  if (auto node = root["decode_prob"]; node)
    decode_prob = clamp_pct(node.as<int>());

  if (auto node = root["imm_random_prob"]; node)
    imm_random_prob = clamp_pct(node.as<int>());

  if (auto node = root["r_weight_base_alu"]; node)
    r_weight_base_alu = clamp_pct(node.as<int>());

  if (auto node = root["r_weight_m"]; node)
    r_weight_m = clamp_pct(node.as<int>());

  if (auto node = root["isa_name"]; node)
    isa_name = node_to_string(node);

  return true;
}

Config loadConfigFromEnv() {
  const char *env_path = std::getenv("MUTATOR_CONFIG");
  Config cfg;
  cfg.loadFromFile(env_path);
  fuzz::debug::logInfo("Loaded config: %s\n", env_path);
  return cfg;
}

void showConfig(const Config &cfg) {
  fuzz::debug::logInfo(
      "strategy=%s verbose=%s enable_c=%s decode_prob=%u imm_random_prob=%u "
      "r_weight_base_alu=%u r_weight_m=%u isa_name=%s\n",
      strategy_to_string(cfg.strategy),
      cfg.verbose ? "true" : "false",
      cfg.enable_c ? "true" : "false",
      cfg.decode_prob,
      cfg.imm_random_prob,
      cfg.r_weight_base_alu,
      cfg.r_weight_m,
      cfg.isa_name.c_str());
}

} // namespace fuzz::mutator
