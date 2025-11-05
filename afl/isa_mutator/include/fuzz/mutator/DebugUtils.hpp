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
  
  const char *base_;
  const char *func_;
};

} // namespace fuzz::mutator
