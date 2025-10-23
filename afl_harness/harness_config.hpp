#pragma once

#include <string>
#include <cstdlib>

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
    crash_dir = getenv_or("CRASH_LOG_DIR", "workdir/logs/crash");
    printf("[HARNESS/CONFIG] Using crash directory: %s\n", crash_dir.c_str());
    objdump   = getenv_or("OBJDUMP", "riscv64-unknown-elf-objdump");
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
