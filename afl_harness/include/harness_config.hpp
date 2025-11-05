#pragma once

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits.h>
#include <string>
#include <unistd.h>

struct HarnessConfig {
  std::string crash_dir; // Absolute path to crash log directory
  std::string objdump; // Path to objdump binary
  int xlen = 32;       // 32 or 64
  unsigned max_cycles = 10000;

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
  }
};
