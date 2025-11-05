#include <fuzz/mutator/DebugUtils.hpp>
#include <cstdlib>

#include <hwfuzz/Log.hpp>

namespace fuzz::mutator {

FunctionTracer::FunctionTracer(const char *file, const char *func)
    : base_(basename(file)), func_(func), enabled_(is_debug_enabled()) {
  if (enabled_) {
  std::fprintf(hwfuzz::harness_log(), "[Fn Start  ]  %s::%s\n", base_, func_);
  }
}

FunctionTracer::~FunctionTracer() {
  if (enabled_) {
  std::fprintf(hwfuzz::harness_log(), "[Fn End    ]  %s::%s\n", base_, func_);
  }
}

bool FunctionTracer::is_debug_enabled() {
  static bool checked = false;
  static bool enabled = false;
  
  if (!checked) {
    const char* debug_env = std::getenv("DEBUG_MUTATOR");
    enabled = (debug_env != nullptr && debug_env[0] != '0' && debug_env[0] != '\0');
    checked = true;
  }
  
  return enabled;
}

const char *FunctionTracer::basename(const char *path) {
  if (!path)
    return "";
  const char *slash = std::strrchr(path, '/');
  const char *backslash = std::strrchr(path, '\\');
  const char *last = slash;
  if (!last || (backslash && backslash > last))
    last = backslash;
  return last ? last + 1 : path;
}

} // namespace fuzz::mutator
