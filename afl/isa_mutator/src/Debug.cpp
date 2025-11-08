#include <fuzz/Debug.hpp>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>

namespace fuzz {
namespace debug {

namespace {

// Cache debug flag status to avoid repeated getenv calls
bool g_debug_checked = false;
bool g_debug_enabled = false;
FILE* g_debug_log = nullptr;
std::mutex g_log_mutex;

void initDebug() {
  if (g_debug_checked)
    return;
  
  g_debug_checked = true;
  
  const char* env = std::getenv("DEBUG");
  g_debug_enabled = (env && std::strcmp(env, "1") == 0);
  
  if (g_debug_enabled) {
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
    
    // Open debug log file
    fs::path log_file = log_dir / "mutator_debug.log";
    g_debug_log = std::fopen(log_file.string().c_str(), "a");
    if (g_debug_log) {
      std::fprintf(g_debug_log, "\n=== Debug session started ===\n");
      std::fflush(g_debug_log);
    }
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
  return g_debug_enabled;
}

FILE* getDebugLog() {
  initDebug();
  return g_debug_log;
}

void logInfo(const char* fmt, ...) {
  initDebug();
  if (!g_debug_enabled || !g_debug_log)
    return;
  
  va_list args;
  va_start(args, fmt);
  writeLog(g_debug_log, "[INFO] ", fmt, args);
  va_end(args);
}

void logWarn(const char* fmt, ...) {
  initDebug();
  if (!g_debug_enabled || !g_debug_log)
    return;
  
  va_list args;
  va_start(args, fmt);
  writeLog(g_debug_log, "[WARN] ", fmt, args);
  va_end(args);
}

void logError(const char* fmt, ...) {
  initDebug();
  if (!g_debug_enabled || !g_debug_log)
    return;
  
  va_list args;
  va_start(args, fmt);
  writeLog(g_debug_log, "[ERROR] ", fmt, args);
  va_end(args);
}

void logDebug(const char* fmt, ...) {
  initDebug();
  if (!g_debug_enabled || !g_debug_log)
    return;
  
  va_list args;
  va_start(args, fmt);
  writeLog(g_debug_log, "[DEBUG] ", fmt, args);
  va_end(args);
}

} // namespace debug
} // namespace fuzz
