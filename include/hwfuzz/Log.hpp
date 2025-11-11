#pragma once

// ============================================================================
// DEPRECATED: This logging system has been replaced by Debug.hpp
// ============================================================================
// The hwfuzz::harness_log() system that wrote to HARNESS_STDIO_LOG has been
// deprecated in favor of the unified hwfuzz::debug:: logging system which
// writes to a single runtime.log file.
//
// Migration guide:
//   std::fprintf(hwfuzz::harness_log(), "[INFO] ...") → hwfuzz::debug::logInfo(...)
//   std::fprintf(hwfuzz::harness_log(), "[WARN] ...") → hwfuzz::debug::logWarn(...)
//   std::fprintf(hwfuzz::harness_log(), "[ERROR] ...") → hwfuzz::debug::logError(...)
//
// This file is kept for backward compatibility but should not be used in new code.
// All logs now go to: ${PROJECT_ROOT}/workdir/logs/runtime.log
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include "QuietLog.hpp"
#include "Debug.hpp"

namespace hwfuzz {

// DEPRECATED: Use hwfuzz::debug::logInfo/logError/logWarn instead
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
