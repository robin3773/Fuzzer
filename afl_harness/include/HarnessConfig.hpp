#pragma once

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <string>
#include <unistd.h>

struct HarnessConfig {
  std::string crash_dir; // Absolute path to crash log directory
  std::string objdump; // Path to objdump binary
  int xlen = 32;       // 32 or 64
  unsigned max_cycles = 10000;
  bool stop_on_spike_done = true;
  bool append_exit_stub = true;
  bool use_tohost = false;
  uint32_t tohost_addr = 0;
  unsigned pc_stagnation_limit = 512;
  unsigned max_program_words = 256;

  static bool parse_u32_env(const char* text, uint32_t& out) {
    if (!text || !*text) return false;
    char* end = nullptr;
    unsigned long v = std::strtoul(text, &end, 0);
    if (end == text) return false;
    out = static_cast<uint32_t>(v);
    return true;
  }

  static unsigned parse_unsigned_env(const char* text, unsigned defv) {
    if (!text || !*text) return defv;
    char* end = nullptr;
    unsigned long v = std::strtoul(text, &end, 0);
    if (end == text) return defv;
    if (v == 0) return defv;
    return static_cast<unsigned>(v);
  }

  static bool parse_bool_env(const char* text, bool defv) {
    if (!text || !*text) return defv;
    if (std::strcmp(text, "0") == 0) return false;
    if (std::strcmp(text, "1") == 0) return true;
    char c = static_cast<char>(std::tolower(static_cast<unsigned char>(text[0])));
    if (c == 't' || c == 'y') return true;
    if (c == 'f' || c == 'n') return false;
    return defv;
  }

  static std::string getenv_or(const char *key, const char *defv) {
    const char *val = std::getenv(key);
    return (val && *val) ? val : defv;
  }

  void load_from_env() {
    std::string cd = getenv_or("CRASH_LOG_DIR", "workdir/logs/crash");

    if (!cd.empty() && cd.front() != '/') {
      char exe_path[PATH_MAX];
      ssize_t n = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
      if (n > 0) {
        exe_path[n] = '\0';
        std::filesystem::path root = std::filesystem::path(exe_path).parent_path().parent_path();
        cd = (std::filesystem::absolute(root) / cd).string();
      }
    }

    crash_dir = cd;
    std::cout << "[INFO] Using crash directory: " << crash_dir << "\n";

    objdump = getenv_or("OBJDUMP", "riscv32-unknown-elf-objdump");
    std::cout << "[INFO] Using objdump: " << objdump << "\n";

    xlen = (getenv_or("XLEN", "32") == "64") ? 64 : 32;

    std::string mc = getenv_or("MAX_CYCLES", "");
    if (!mc.empty()) {
      if (unsigned v = std::strtoul(mc.c_str(), nullptr, 0); v)
        max_cycles = v;
    }

    stop_on_spike_done = parse_bool_env(std::getenv("STOP_ON_SPIKE_DONE"), true);
    append_exit_stub = parse_bool_env(std::getenv("APPEND_EXIT_STUB"), true);
    pc_stagnation_limit = parse_unsigned_env(std::getenv("PC_STAGNATION_LIMIT"), pc_stagnation_limit);
    max_program_words = parse_unsigned_env(std::getenv("MAX_PROGRAM_WORDS"), max_program_words);

    if (uint32_t addr = 0; parse_u32_env(std::getenv("TOHOST_ADDR"), addr)) {
      tohost_addr = addr;
      use_tohost = true;
    }

    std::cout << "[INFO] Max cycles: " << max_cycles << "\n";
    std::cout << "[INFO] Max program words: " << max_program_words << "\n";
    std::cout << "[INFO] PC stagnation limit: " << pc_stagnation_limit << "\n";
    std::cout << "[INFO] Stop on Spike completion: " << (stop_on_spike_done ? "yes" : "no") << "\n";
    std::cout << "[INFO] Append exit stub: " << (append_exit_stub ? "yes" : "no") << "\n";
    if (use_tohost) {
      std::cout << "[INFO] tohost address: 0x" << std::hex << tohost_addr << std::dec << "\n";
    }
  }
};
