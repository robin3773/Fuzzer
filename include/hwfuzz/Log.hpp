#pragma once

#include <cstdio>
#include <cstdlib>
#include "QuietLog.hpp"

namespace hwfuzz {

inline FILE*& harness_log_storage() {
  static FILE* fp = nullptr;
  return fp;
}

inline FILE* harness_log() {
  // Fast path: if quiet mode, return /dev/null immediately
  if (is_quiet_mode()) {
    return get_quiet_log();
  }
  
  FILE*& fp = harness_log_storage();
  if (fp) {
    return fp;
  }
  const char* path = std::getenv("HARNESS_STDIO_LOG");
  if (path && *path) {
    fp = std::fopen(path, "a");
    if (fp) {
      std::setvbuf(fp, nullptr, _IOLBF, 0);
      return fp;
    }
  }
  fp = stderr;
  return fp;
}

inline void set_harness_log(FILE* fp) {
  if (!fp) {
    return;
  }
  if (fp != stdout && fp != stderr) {
    std::setvbuf(fp, nullptr, _IOLBF, 0);
  }
  harness_log_storage() = fp;
}

inline void flush_harness_log() {
  std::fflush(harness_log());
}

} // namespace hwfuzz
