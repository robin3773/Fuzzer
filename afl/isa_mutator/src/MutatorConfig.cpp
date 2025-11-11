#include <fuzz/mutator/MutatorConfig.hpp>

#include <cstdio>
#include <cstdlib>
#include <hwfuzz/Debug.hpp>
#include <string>

#include <yaml-cpp/yaml.h>

namespace {

using fuzz::mutator::Config;
using fuzz::mutator::Strategy;

Strategy string_to_strategy(const std::string &str) {
  if (str == "RAW" || str == "BYTE_LEVEL") return Strategy::BYTE_LEVEL;
  if (str == "IR" || str == "INSTRUCTION_LEVEL") return Strategy::INSTRUCTION_LEVEL;
  if (str == "HYBRID" || str == "MIXED_MODE") return Strategy::MIXED_MODE;
  if (str == "AUTO" || str == "ADAPTIVE") return Strategy::ADAPTIVE;
  return Strategy::INSTRUCTION_LEVEL; // default
}

std::string node_to_string(const YAML::Node &node) {
  if (!node)
    return {};
  if (node.IsNull())
    return {};
  return node.as<std::string>();
}

void apply_schema_block(const YAML::Node &node, Config &cfg) {
  if (!node || !node.IsMap())
    return;
  if (auto v = node["isa"]; v)
    cfg.isa_name = node_to_string(v);
}

} // namespace

namespace fuzz::mutator {

Config loadConfig(bool show_config) {
  const char *env_path = std::getenv("MUTATOR_CONFIG");
  
  YAML::Node root = YAML::LoadFile(env_path);

  Config cfg;
  
  std::string strategy_str = "INSTRUCTION_LEVEL";
  if (auto node = root["strategy"]; node) {
    strategy_str = node.as<std::string>();
    cfg.strategy = string_to_strategy(strategy_str);
  }

  apply_schema_block(root["schemas"], cfg);

  hwfuzz::debug::logInfo("[MUTATOR] Loaded config: %s\n", env_path);
  
  if (show_config) {
    hwfuzz::debug::logInfo(
        "[MUTATOR] Strategy=%s Isa_name=%s\n",
        strategy_str.c_str(),
        cfg.isa_name.c_str());
  }
  
  return cfg;
}

} // namespace fuzz::mutator
