/**
 * @file HarnessConfig.cpp
 * @brief Implementation of configuration management for the AFL++ fuzzing harness
 */

#include "HarnessConfig.hpp"
#include <hwfuzz/Debug.hpp>
#include <algorithm>
#include <fstream>
#include <cctype>

std::unordered_map<std::string, std::string> HarnessConfig::parse_conf_file(const std::string& conf_path) {
  std::unordered_map<std::string, std::string> config;
  std::ifstream file(conf_path);
  
  if (!file.is_open()) {
    return config;
  }

  std::string line;
  while (std::getline(file, line)) {
    // Trim whitespace
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) continue;  // Empty line
    
    size_t end = line.find_last_not_of(" \t\r\n");
    line = line.substr(start, end - start + 1);
    
    // Skip comments and section headers
    if (line.empty() || line[0] == '#' || line[0] == '[') continue;
    
    // Parse KEY=value
    size_t eq_pos = line.find('=');
    if (eq_pos != std::string::npos) {
      std::string key = line.substr(0, eq_pos);
      std::string value = line.substr(eq_pos + 1);
      
      // Trim key and value
      size_t key_end = key.find_last_not_of(" \t");
      if (key_end != std::string::npos) {
        key = key.substr(0, key_end + 1);
      }
      
      size_t val_start = value.find_first_not_of(" \t");
      if (val_start != std::string::npos) {
        value = value.substr(val_start);
      }
      
      config[key] = value;
    }
  }

  return config;
}

void HarnessConfig::loadconfig() {
    // Read PROJECT_ROOT from environment (required)
    const char* root_env = std::getenv("PROJECT_ROOT");
    if (!root_env || !*root_env) {
        hwfuzz::debug::logError("[CONFIG] PROJECT_ROOT environment variable not set!\n");
        hwfuzz::debug::logError("[CONFIG] Please run via run.sh or export PROJECT_ROOT=/path/to/Fuzz\n");
        std::exit(1);
    }
    std::string root = std::string(root_env);
    std::filesystem::path project_root = std::filesystem::absolute(root);
    
    // Crash directory is always {PROJECT_ROOT}/workdir/logs/crash
    crash_dir = (project_root / "workdir" / "logs" / "crash").string();
    // Trace directory is always {PROJECT_ROOT}/workdir/traces
    trace_dir = (project_root / "workdir" / "traces").string();
    hwfuzz::debug::logInfo("Project root: %s\n", project_root.string().c_str());
    hwfuzz::debug::logInfo("Using crash directory: %s\n", crash_dir.c_str());
    hwfuzz::debug::logInfo("Using trace directory: %s\n", trace_dir.c_str());

    // Load config file from {PROJECT_ROOT}/afl_harness/harness.conf
    std::filesystem::path conf_path = project_root / "afl_harness" / "harness.conf";
    std::unordered_map<std::string, std::string> config = parse_conf_file(conf_path.string());

    auto get_string = [&](const std::string& key, const std::string& default_value = std::string()) -> std::string {
        auto it = config.find(key);
        if (it == config.end() || it->second.empty()) return default_value;
        return it->second;
    };

    auto parse_bool = [](const std::string& value, bool default_value) -> bool {
        if (value.empty()) return default_value;
        std::string lower = value;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lower == "1" || lower == "true" || lower == "t" || lower == "y" || lower == "yes" || lower == "on") {
            return true;
        }
        if (lower == "0" || lower == "false" || lower == "f" || lower == "n" || lower == "no" || lower == "off") {
            return false;
        }
        return default_value;
    };

    auto get_bool = [&](const std::string& key, bool default_value) -> bool {
        auto it = config.find(key);
        if (it == config.end()) return default_value;
        return parse_bool(it->second, default_value);
    };

    auto resolve_relative = [&](const std::string& path_value) -> std::string {
        if (path_value.empty()) return path_value;
        std::filesystem::path p(path_value);
        if (p.is_absolute()) return p.string();
        return (project_root / p).string();
    };

    // Read TOHOST_ADDR from environment only (required by ISA mutator)
    const char* tohost_env = std::getenv("TOHOST_ADDR");
    if (!tohost_env || !*tohost_env) {
        hwfuzz::debug::logError("[CONFIG] TOHOST_ADDR environment variable not set!\n");
        std::exit(1);
    }
    tohost_addr = std::stoul(tohost_env, nullptr, 0);
    
    // Read from config file
    objdump = get_string("OBJDUMP");
    xlen = std::stoi(config["XLEN"]);
    max_cycles = std::stoul(config["MAX_CYCLES"]);
    stop_on_spike_done = get_bool("STOP_ON_SPIKE_DONE", true);
    pc_stagnation_limit = std::stoul(config["PC_STAGNATION_LIMIT"]);
    max_program_words = std::stoul(config["MAX_PROGRAM_WORDS"]);

    // Linker binary and script
    ld_bin = config["LD_BIN"];
    linker_script = config["LINKER_SCRIPT"];

    golden_mode = get_string("GOLDEN_MODE", "live");
    spike_bin = get_string("SPIKE_BIN");
    spike_isa = get_string("SPIKE_ISA", "rv32imc");
    pk_bin = get_string("PK_BIN");
    const std::string default_spike_log = (project_root / "workdir" / "logs" / "spike.log").string();
    spike_log_file = resolve_relative(get_string("SPIKE_LOG_FILE", default_spike_log));
    trace_enabled = get_bool("TRACE_MODE", true);
    

    hwfuzz::debug::logInfo("LD bin: %s\n", ld_bin.c_str());
    hwfuzz::debug::logInfo("Linker script: %s\n", linker_script.c_str());
    hwfuzz::debug::logInfo("tohost address: 0x%08x\n", tohost_addr);
    hwfuzz::debug::logInfo("Using objdump: %s\n", objdump.c_str());
    hwfuzz::debug::logInfo("Max cycles: %u\n", max_cycles);
    hwfuzz::debug::logInfo("Max program words: %u\n", max_program_words);
    hwfuzz::debug::logInfo("PC stagnation limit: %u\n", pc_stagnation_limit);
    hwfuzz::debug::logInfo("Stop on Spike completion: %s\n", stop_on_spike_done ? "yes" : "no");
    hwfuzz::debug::logInfo("Golden mode: %s\n", golden_mode.c_str());
    hwfuzz::debug::logInfo("Trace mode: %s\n", trace_enabled ? "on" : "off");
    hwfuzz::debug::logInfo("Spike binary: %s\n", spike_bin.empty() ? "<unset>" : spike_bin.c_str());
    hwfuzz::debug::logInfo("Spike ISA: %s\n", spike_isa.c_str());
    hwfuzz::debug::logInfo("PK binary: %s\n", pk_bin.empty() ? "<unset>" : pk_bin.c_str());
    hwfuzz::debug::logInfo("Spike log file: %s\n", spike_log_file.empty() ? "<disabled>" : spike_log_file.c_str());
}
