/**
 * @file HarnessConfig.cpp
 * @brief Implementation of configuration management for the AFL++ fuzzing harness
 */

#include "HarnessConfig.hpp"
#include <hwfuzz/Debug.hpp>
#include <fstream>

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
    std::string root = std::getenv("PROJECT_ROOT");
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

    // Read TOHOST_ADDR from environment only
    tohost_addr = std::stoul(std::getenv("TOHOST_ADDR"), nullptr, 0);
    
    // Read from config file
    objdump = config["OBJDUMP"];
    xlen = std::stoi(config["XLEN"]);
    max_cycles = std::stoul(config["MAX_CYCLES"]);
    stop_on_spike_done = (config["STOP_ON_SPIKE_DONE"] == "true" );
    pc_stagnation_limit = std::stoul(config["PC_STAGNATION_LIMIT"]);
    max_program_words = std::stoul(config["MAX_PROGRAM_WORDS"]);

    
    hwfuzz::debug::logInfo("tohost address: 0x%08x\n", tohost_addr);
    hwfuzz::debug::logInfo("Using objdump: %s\n", objdump.c_str());
    hwfuzz::debug::logInfo("Max cycles: %u\n", max_cycles);
    hwfuzz::debug::logInfo("Max program words: %u\n", max_program_words);
    hwfuzz::debug::logInfo("PC stagnation limit: %u\n", pc_stagnation_limit);
    hwfuzz::debug::logInfo("Stop on Spike completion: %s\n", stop_on_spike_done ? "yes" : "no");
}
