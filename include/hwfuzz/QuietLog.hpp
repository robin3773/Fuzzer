#pragma once

#include <cstdio>

namespace hwfuzz {

// Quiet mode: redirect all logging to /dev/null
inline FILE* get_quiet_log() {
  static FILE* null_fp = nullptr;
  if (!null_fp) {
    null_fp = std::fopen("/dev/null", "w");
  }
  return null_fp ? null_fp : stderr;
}

// Check if quiet mode is enabled
inline bool is_quiet_mode() {
  static int quiet = -1;
  if (quiet == -1) {
    const char* env = std::getenv("FUZZER_QUIET");
    quiet = (env && (env[0] == '1' || env[0] == 'y' || env[0] == 'Y')) ? 1 : 0;
  }
  return quiet == 1;
}

} // namespace hwfuzz
