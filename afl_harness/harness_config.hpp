#pragma once

#include <string>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <limits.h>

struct HarnessConfig {
  std::string crash_dir;
  std::string objdump;
  int xlen = 32;
  unsigned max_cycles = 10000;

  static std::string getenv_or(const char* k, const char* defv) {
    const char* v = std::getenv(k);
    return (v && *v) ? std::string(v) : std::string(defv);
  }

  void load_from_env() {
    // Read desired crash dir, anchor relative paths to project root (derived from exe path)
    std::string cd = getenv_or("CRASH_LOG_DIR", "workdir/logs/crash");
    if (!cd.empty() && cd[0] != '/') {
      // Resolve /proc/self/exe -> .../afl/afl_picorv32, project root is dirname(dirname(exe))
      char exe_path[PATH_MAX];
      ssize_t n = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
      std::string root;
      if (n > 0) {
        exe_path[n] = '\0';
        std::string ep(exe_path);
        // dirname twice
        auto slash1 = ep.rfind('/');
        if (slash1 != std::string::npos) {
          auto dir = ep.substr(0, slash1);
          auto slash2 = dir.rfind('/');
          if (slash2 != std::string::npos) root = dir.substr(0, slash2);
        }
      }
      if (!root.empty()) cd = root + "/" + cd; // anchor to project root
      // else: fallback to current working directory semantics
    }
    crash_dir = cd;
    printf("[HARNESS/CONFIG] Using crash directory: %s\n", crash_dir.c_str());
    objdump   = getenv_or("OBJDUMP", "riscv32-unknown-elf-objdump");
    printf("[HARNESS/CONFIG] Using objdump: %s\n", objdump.c_str());
    std::string x = getenv_or("XLEN", "32");
    xlen = (x == "64") ? 64 : 32;
    std::string mc = getenv_or("MAX_CYCLES", "");
    if (!mc.empty()) {
      unsigned v = (unsigned)strtoul(mc.c_str(), nullptr, 0);
      if (v) max_cycles = v;
    }
  }
};
