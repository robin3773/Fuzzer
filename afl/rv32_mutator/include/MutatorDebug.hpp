#pragma once
// Minimal debug/logging for illegal encodings
// Enable via env:
//   DEBUG_MUTATOR=1                  -> turn on console messages
//   DEBUG_LOG=1                      -> log to
//   afl/rv32_mutator/logs/mutator_debug.log DEBUG_LOG=/path/to/file.log      ->
//   custom log path
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace MutatorDebug {

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
  const char *lg = std::getenv("DEBUG_LOG");
  if (lg && std::strcmp(lg, "0") != 0) {
    S.log_to_file = true;
    S.path = (std::strcmp(lg, "1") == 0)
                 ? "afl/rv32_mutator/logs/mutator_debug.log"
                 : lg;
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

  std::fprintf(stderr, "[ILLEGAL] %s()\n  before = 0x%08x\n  after  = 0x%08x\n",
               src, before, after);

  if (S.fp) {
    std::fprintf(S.fp, "[ILLEGAL] %s()\n  before = 0x%08x\n  after  = 0x%08x\n",
                 src, before, after);
    std::fflush(S.fp);
  }
}

} // namespace MutatorDebug

#define MUTDBG_ILLEGAL(before_u32, after_u32, src_name)                        \
  do {                                                                         \
    MutatorDebug::log_illegal(src_name, (before_u32), (after_u32));            \
  } while (0)
