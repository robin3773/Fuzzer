#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <hwfuzz/Log.hpp>

namespace fuzz::mutator::debug {

struct State {
  bool enabled = false;
  bool log_to_file = false;
  std::string path;
  std::FILE *fp = nullptr;
};

inline State &state() {
  static State s;
  return s;
}

inline void init_from_env() {
  State &S = state();
  const char *dbg = std::getenv("DEBUG_MUTATOR");
  S.enabled = (dbg && std::strcmp(dbg, "0") != 0);
  
  // If debug is enabled, also enable file logging
  if (S.enabled) {
    S.log_to_file = true;
    S.path = "afl/isa_mutator/logs/mutator_debug.log";
    S.fp = std::fopen(S.path.c_str(), "ab");
  }
}

inline void deinit() {
  State &S = state();
  if (S.fp) {
    std::fclose(S.fp);
    S.fp = nullptr;
  }
}

inline void log_illegal(const char *src, uint32_t before, uint32_t after) {
  State &S = state();
  if (!S.enabled)
    return;

  std::fprintf(hwfuzz::harness_log(), "[ILLEGAL] %s()\n  before = 0x%08x\n  after  = 0x%08x\n",
               src, before, after);

  if (S.fp) {
    std::fprintf(S.fp, "[ILLEGAL] %s()\n  before = 0x%08x\n  after  = 0x%08x\n",
                 src, before, after);
    std::fflush(S.fp);
  }
}

} // namespace fuzz::mutator::debug

namespace MutatorDebug = fuzz::mutator::debug;

#define MUTDBG_ILLEGAL(before_u32, after_u32, src_name)                        \
  do {                                                                         \
    MutatorDebug::log_illegal(src_name, (before_u32), (after_u32));            \
  } while (0)
