#pragma once

#include <cstddef>
#include <string>

namespace fuzz::mutator {

class MutatorInterface {
public:
  virtual ~MutatorInterface() = default;

  virtual void initFromEnv() = 0;

  virtual unsigned char *mutateStream(unsigned char *in, size_t in_len,
                                      unsigned char *out_buf,
                                      size_t max_size) = 0;

  virtual size_t last_out_len() const = 0;

  virtual void setConfigPath(const std::string &path) { (void)path; }
};

} // namespace fuzz::mutator
