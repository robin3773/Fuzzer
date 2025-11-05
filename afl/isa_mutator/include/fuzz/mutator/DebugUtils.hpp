#pragma once

#include <cstdio>
#include <cstring>

namespace fuzz::mutator {

class FunctionTracer {
public:
  FunctionTracer(const char *file, const char *func);
  ~FunctionTracer();

private:
  static const char *basename(const char *path);
  static bool is_debug_enabled();
  
  const char *base_;
  const char *func_;
  bool enabled_;
};

} // namespace fuzz::mutator
