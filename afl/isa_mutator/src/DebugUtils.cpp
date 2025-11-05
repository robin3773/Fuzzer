#include <fuzz/mutator/DebugUtils.hpp>

namespace fuzz::mutator {

FunctionTracer::FunctionTracer(const char *file, const char *func)
    : base_(basename(file)), func_(func) {
  std::fprintf(stderr, "[Fn Start  ]  %s::%s\n", base_, func_);
}

FunctionTracer::~FunctionTracer() {
  std::fprintf(stderr, "[Fn End    ]  %s::%s\n", base_, func_);
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
