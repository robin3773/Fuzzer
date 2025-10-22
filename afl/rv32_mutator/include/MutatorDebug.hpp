#pragma once
// MutatorDebug.hpp — minimal debug/logging for illegal encodings
// Usage:
//   1) Call MutatorDebug::init_from_env() once at init.
//   2) Wrap mutation sites with MUTDBG_ILLEGAL(before, after, "func_name").
//
// Enable via env:
//   DEBUG_MUTATOR=1                  -> turn on console messages
//   DEBUG_LOG=1                      -> log to afl/rv32_mutator/logs/mutator_debug.log
//   DEBUG_LOG=/path/to/file.log      -> custom log path

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>

namespace MutatorDebug {

struct State {
  bool enabled = false;
  FILE* fp = nullptr;            // optional file
  std::string path;              // chosen path, if any
};

inline State& state() {
  static State s;
  return s;
}

// Ensure directory exists if using default logs dir (best-effort, ignore errors)
inline void ensure_dir_best_effort(const char* path) {
#if defined(__unix__) || defined(__APPLE__)
  // create intermediate dirs afl/rv32_mutator/logs if missing
  // naive + best-effort: "mkdir -p ..."
  std::string cmd = std::string("mkdir -p ") + path;
  (void)std::system(cmd.c_str());
#endif
}

inline void init_from_env() {
  State& S = state();

  const char* d = std::getenv("DEBUG_MUTATOR");
  S.enabled = (d && *d == '1');

  if (!S.enabled) return;

  const char* l = std::getenv("DEBUG_LOG");
  if (l && *l) {
    // If "1", use default path; else use the provided path
    if (std::strcmp(l, "1") == 0) {
      // default path
      ensure_dir_best_effort("afl/rv32_mutator/logs");
      S.path = "afl/rv32_mutator/logs/mutator_debug.log";
    } else {
      S.path = l;
    }
    S.fp = std::fopen(S.path.c_str(), "a");
    if (!S.fp) {
      // Fall back to console only
      std::fprintf(stderr,
        "[DEBUG] Failed to open DEBUG_LOG file '%s' — console-only mode.\n",
        S.path.c_str());
    }
  }
  std::fprintf(stderr, "[DEBUG] Mutator debug enabled%s%s\n",
               S.fp ? " -> file: " : "",
               S.fp ? S.path.c_str() : "");
}

inline void deinit() {
  State& S = state();
  if (S.fp) { std::fclose(S.fp); S.fp = nullptr; }
  S.enabled = false;
  S.path.clear();
}

inline void log_illegal(const char* src, uint32_t before, uint32_t after) {
  State& S = state();
  if (!S.enabled) return;

  // Console
  std::fprintf(stderr,
    "[ILLEGAL] %s()\n  before = 0x%08x\n  after  = 0x%08x\n",
    src, before, after);

  // File (optional)
  if (S.fp) {
    std::fprintf(S.fp,
      "[ILLEGAL] %s()\n  before = 0x%08x\n  after  = 0x%08x\n",
      src, before, after);
    std::fflush(S.fp);
  }
}

} // namespace MutatorDebug

// Convenience macro: only logs when illegal; you still decide legality outside.
#define MUTDBG_ILLEGAL(before_u32, after_u32, src_name) \
  do { MutatorDebug::log_illegal(src_name, (before_u32), (after_u32)); } while (0)
