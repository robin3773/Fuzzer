#include <fuzz/mutator/MutatorConfig.hpp>

#include <cstdio>
#include <cstdlib>
#include <fuzz/Debug.hpp>
#include <string>

#include <yaml-cpp/yaml.h>

namespace {

using fuzz::mutator::Config;
using fuzz::mutator::Strategy;

Strategy string_to_strategy(const std::string &str) {
  if (str == "RAW") return Strategy::RAW;
  if (str == "IR") return Strategy::IR;
  if (str == "HYBRID") return Strategy::HYBRID;
  if (str == "AUTO") return Strategy::AUTO;
  return Strategy::IR; // default
}

std::string node_to_string(const YAML::Node &node) {
  if (!node)
    return {};
  if (node.IsNull())
    return {};
  return node.as<std::string>();
}

void apply_probability_block(const YAML::Node &node, Config &cfg) {
  if (!node || !node.IsMap())
    return;
  if (auto v = node["decode"]; v)
    cfg.decode_prob = v.as<int>();
  if (auto v = node["imm_random"]; v)
    cfg.imm_random_prob = v.as<int>();
}

void apply_weight_block(const YAML::Node &node, Config &cfg) {
  if (!node || !node.IsMap())
    return;
  if (auto v = node["r_base_alu"]; v)
    cfg.r_weight_base_alu = v.as<int>();
  if (auto v = node["r_m"]; v)
    cfg.r_weight_m = v.as<int>();
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
  
  std::string strategy_str = "IR";
  if (auto node = root["strategy"]; node) {
    strategy_str = node.as<std::string>();
    cfg.strategy = string_to_strategy(strategy_str);
  }

  if (auto node = root["verbose"]; node)
    cfg.verbose = node.as<bool>();

  apply_probability_block(root["probabilities"], cfg);
  apply_weight_block(root["weights"], cfg);
  apply_schema_block(root["schemas"], cfg);

  fuzz::debug::logInfo("Loaded config: %s\n", env_path);
  
  if (show_config) {
    fuzz::debug::logInfo(
        "Strategy=%s Verbose=%s Decode_prob=%u Imm_random_prob=%u "
        "R_weight_base_alu=%u R_weight_m=%u Isa_name=%s\n",
        strategy_str.c_str(),
        cfg.verbose ? "true" : "false",
        cfg.decode_prob,
        cfg.imm_random_prob,
        cfg.r_weight_base_alu,
        cfg.r_weight_m,
        cfg.isa_name.c_str());
  }
  
  return cfg;
}

} // namespace fuzz::mutator
