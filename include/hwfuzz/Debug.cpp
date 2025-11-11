#include <hwfuzz/Debug.hpp>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <unistd.h>

namespace hwfuzz {
namespace debug {

namespace {

// Logging is now always enabled since this is the primary logging system
// (replaces the old HARNESS_STDIO_LOG/harness.log system)
bool g_log_initialized = false;
FILE* g_debug_log = nullptr;
std::mutex g_log_mutex;

// For backward compatibility: DEBUG=1 still enables function tracing
bool g_trace_enabled = false;

void initDebug() {
  if (g_log_initialized)
    return;
  
  g_log_initialized = true;
  
  // Check if function tracing should be enabled (DEBUG=1)
  const char* debug_env = std::getenv("DEBUG");
  g_trace_enabled = (debug_env && std::strcmp(debug_env, "1") == 0);
  
  // Create logs directory if it doesn't exist
  namespace fs = std::filesystem;
  
  const char* project_root = std::getenv("PROJECT_ROOT");
  fs::path log_dir;
  if (project_root) {
    log_dir = fs::path(project_root) / "workdir" / "logs";
  } else {
    // Fallback to current directory if PROJECT_ROOT not set
    log_dir = "workdir/logs";
  }
  
  std::error_code ec;
  fs::create_directories(log_dir, ec);
  
  // Open runtime log file (shared by mutator and harness)
  // This is now always enabled (not gated by DEBUG flag)
  fs::path log_file = log_dir / "runtime.log";
  g_debug_log = std::fopen(log_file.string().c_str(), "a");
  if (g_debug_log) {
    std::fprintf(g_debug_log, "\n=== Runtime session started (pid=%d) ===\n", getpid());
    std::fflush(g_debug_log);
  }
}

void writeLog(FILE* file, const char* prefix, const char* fmt, va_list args) {
  std::lock_guard<std::mutex> lock(g_log_mutex);
  
  if (file) {
    std::fprintf(file, "%s", prefix);
    std::vfprintf(file, fmt, args);
    std::fflush(file);
  }
}

} // anonymous namespace

bool isDebugEnabled() {
  initDebug();
  return g_trace_enabled;
}

FILE* getDebugLog() {
  initDebug();
  return g_debug_log;
}

void logInfo(const char* fmt, ...) {
  initDebug();
  if (!g_debug_log)
    return;
  
  va_list args;
  va_start(args, fmt);
  writeLog(g_debug_log, "[INFO] ", fmt, args);
  va_end(args);
}

void logWarn(const char* fmt, ...) {
  initDebug();
  if (!g_debug_log)
    return;
  
  va_list args;
  va_start(args, fmt);
  writeLog(g_debug_log, "[WARN] ", fmt, args);
  va_end(args);
}

void logError(const char* fmt, ...) {
  initDebug();
  if (!g_debug_log)
    return;
  
  va_list args;
  va_start(args, fmt);
  writeLog(g_debug_log, "[ERROR] ", fmt, args);
  va_end(args);
}

void logDebug(const char* fmt, ...) {
  initDebug();
  if (!g_debug_log)
    return;
  
  va_list args;
  va_start(args, fmt);
  writeLog(g_debug_log, "[DEBUG] ", fmt, args);
  va_end(args);
}

void logIllegal(const char* src, uint32_t before, uint32_t after) {
  initDebug();
  if (!g_debug_log)
    return;
  
  std::lock_guard<std::mutex> lock(g_log_mutex);
  std::fprintf(g_debug_log, "[ILLEGAL] %s()\n  before = 0x%08x\n  after  = 0x%08x\n",
               src, before, after);
  std::fflush(g_debug_log);
}

const char* basename(const char* path) {
  if (!path)
    return "";
  const char* slash = std::strrchr(path, '/');
  const char* backslash = std::strrchr(path, '\\');
  const char* last = slash;
  if (!last || (backslash && backslash > last))
    last = backslash;
  return last ? last + 1 : path;
}

FunctionTracer::FunctionTracer(const char* file, const char* func)
    : base_(basename(file)), func_(func) {
  initDebug();
  // Function tracing only happens when DEBUG=1 (to reduce noise)
  if (!g_trace_enabled || !g_debug_log)
    return;
  
  std::lock_guard<std::mutex> lock(g_log_mutex);
  std::fprintf(g_debug_log, "[Fn Start  ] %s::%s\n", base_, func_);
  std::fflush(g_debug_log);
}

FunctionTracer::~FunctionTracer() {
  initDebug();
  // Function tracing only happens when DEBUG=1 (to reduce noise)
  if (!g_trace_enabled || !g_debug_log)
    return;
  
  std::lock_guard<std::mutex> lock(g_log_mutex);
  std::fprintf(g_debug_log, "[Fn End    ] %s::%s\n", base_, func_);
  std::fflush(g_debug_log);
}

} // namespace debug
} // namespace hwfuzz
