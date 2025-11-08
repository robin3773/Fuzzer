/**
 * @file DebugUtils.hpp
 * @brief Unified debug system for function tracing and event logging
 * 
 * Provides a comprehensive debug infrastructure for the ISA mutator,
 * including RAII-based function tracing, event logging, and file output.
 * All debug functionality is controlled by the DEBUG environment variable.
 * 
 * @details
 * When DEBUG=1:
 * - Function entry/exit is logged via FunctionTracer (RAII)
 * - Events and errors are logged via log_message() and log_illegal()
 * - All output goes to both stderr and debug log file
 * 
 * When DEBUG=0 or unset:
 * - All debug calls become no-ops (zero runtime overhead)
 * 
 * @note Debug log file: afl/isa_mutator/logs/mutator_debug.log
 * @see FunctionTracer
 * @see init_from_env()
 * @see MUTDBG_ILLEGAL
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <hwfuzz/Log.hpp>

/**
 * @namespace fuzz::mutator::debug
 * @brief Debug system namespace containing tracing and logging utilities
 */
namespace fuzz::mutator::debug {

/**
 * @struct State
 * @brief Global debug state singleton
 */
struct State {
  bool enabled = false;        ///< Whether debug logging is enabled
  bool log_to_file = false;    ///< Whether file logging is enabled
  std::string path;            ///< Path to debug log file
  std::FILE *fp = nullptr;     ///< File handle for debug log
};

/**
 * @brief Get the global debug state singleton
 * @return Reference to global State object
 */
inline State &state() {
  static State s;
  return s;
}

/**
 * @brief Initialize debug system from DEBUG environment variable
 * 
 * Reads the DEBUG environment variable and enables logging if set to non-zero.
 * When enabled, creates/appends to debug log file.
 * 
 * @note Call this once at mutator initialization
 */
inline void init_from_env() {
  State &S = state();
  const char *dbg = std::getenv("DEBUG");
  S.enabled = (dbg && std::strcmp(dbg, "0") != 0 && dbg[0] != '\0');
  
  // If debug is enabled, also enable file logging
  if (S.enabled) {
    S.log_to_file = true;
    S.path = "afl/isa_mutator/logs/mutator_debug.log";
    S.fp = std::fopen(S.path.c_str(), "ab");
    if (S.fp) {
      std::fprintf(S.fp, "\n========== New Session ==========\n");
      std::fflush(S.fp);
    }
  }
}

/**
 * @brief Cleanup debug system and close log file
 * 
 * @note Call this at mutator shutdown
 */
inline void deinit() {
  State &S = state();
  if (S.fp) {
    std::fclose(S.fp);
    S.fp = nullptr;
  }
}

/**
 * @brief Log a generic debug message with tag
 * @param tag Category/tag for the message (e.g., "INFO", "WARNING")
 * @param msg Message content
 */
inline void log_message(const char *tag, const char *msg) {
  State &S = state();
  if (!S.enabled)
    return;

  std::fprintf(hwfuzz::harness_log(), "[%s] %s\n", tag, msg);

  if (S.fp) {
    std::fprintf(S.fp, "[%s] %s\n", tag, msg);
    std::fflush(S.fp);
  }
}

// Log illegal mutation attempts
inline void log_illegal(const char *src, uint32_t before, uint32_t after) {
  State &S = state();
  if (!S.enabled)
    return;

  std::fprintf(hwfuzz::harness_log(), 
               "[ILLEGAL] %s()\n  before = 0x%08x\n  after  = 0x%08x\n",
               src, before, after);

  if (S.fp) {
    std::fprintf(S.fp, 
                 "[ILLEGAL] %s()\n  before = 0x%08x\n  after  = 0x%08x\n",
                 src, before, after);
    std::fflush(S.fp);
  }
}

// Helper to extract basename from path
inline const char *basename(const char *path) {
  if (!path)
    return "";
  const char *slash = std::strrchr(path, '/');
  const char *backslash = std::strrchr(path, '\\');
  const char *last = slash;
  if (!last || (backslash && backslash > last))
    last = backslash;
  return last ? last + 1 : path;
}

// RAII function tracer - logs entry/exit automatically
class FunctionTracer {
public:
  FunctionTracer(const char *file, const char *func)
      : base_(basename(file)), func_(func) {
    State &S = state();
    if (S.enabled) {
      std::fprintf(hwfuzz::harness_log(), "[Fn Start  ] %s::%s\n", base_, func_);
      if (S.fp) {
        std::fprintf(S.fp, "[Fn Start  ] %s::%s\n", base_, func_);
        std::fflush(S.fp);
      }
    }
  }

  ~FunctionTracer() {
    State &S = state();
    if (S.enabled) {
      std::fprintf(hwfuzz::harness_log(), "[Fn End    ] %s::%s\n", base_, func_);
      if (S.fp) {
        std::fprintf(S.fp, "[Fn End    ] %s::%s\n", base_, func_);
        std::fflush(S.fp);
      }
    }
  }

private:
  const char *base_;
  const char *func_;
};

} // namespace fuzz::mutator::debug

// Convenience aliases
namespace MutatorDebug = fuzz::mutator::debug;

// Macros for easy usage
#define MUTDBG_ILLEGAL(before_u32, after_u32, src_name)                        \
  do {                                                                         \
    fuzz::mutator::debug::log_illegal(src_name, (before_u32), (after_u32));   \
  } while (0)
