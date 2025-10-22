
#include "AFLInterface.hpp"
#include "RV32Mutator.hpp"
#include "Random.hpp"
#include "MutatorDebug.hpp" // optional debug/logging
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>
#include <iostream>
#include <unistd.h>

static mut::RV32Mutator g_mut;

extern "C" {

unsigned char *afl_custom_fuzz(unsigned char *buf, size_t buf_size,
                               unsigned char *out_buf, size_t max_size,
                               unsigned int seed) {
  mut::Random::seed(seed);
  return g_mut.mutateStream(buf, buf_size, out_buf, max_size);
}

size_t afl_custom_fuzz_b(unsigned char *data, size_t size, unsigned char **out_buf, unsigned int seed) {
  mut::Random::seed(seed);
  unsigned char* m = g_mut.mutateStream(data, size, nullptr, 0);
  if (!m) { *out_buf = nullptr; return 0; }
  *out_buf = m;
  return ((size + 3) / 4) * 4; // buffer rounded internally
}

size_t afl_custom_havoc_mutation(unsigned char *data, size_t size, unsigned char **out_buf, unsigned int seed) {
  return afl_custom_fuzz_b(data, size, out_buf, seed);
}

int afl_custom_init(void *afl) {
  (void)afl;

  MutatorDebug::init_from_env();
  const char *arch = getenv("RV32_MODE");
  if (arch && std::strchr(arch, 'E')) {
    // NOTE: use ::setenv from <cstdlib>, not std::setenv (which doesn't exist)
    ::setenv("RV32E_MODE", "1", 1);
    std::fprintf(stderr, "[mutator] RV32E mode (16 regs)\n");
  } else {
    ::setenv("RV32E_MODE", "0", 1);
    std::fprintf(stderr, "[mutator] RV32I mode (32 regs)\n");
  }
  // RV32E mode compatibility with RV32_MODE env
  if (arch && std::strchr(arch, 'E')) {
      setenv("RV32E_MODE", "1", 1);
  }
  g_mut.initFromEnv();
  mut::Random::seed((uint32_t)time(nullptr));
  std::fprintf(stderr, "[mutator] RV32 hybrid mutator initialized.\n");
  return 0;
}

void afl_custom_deinit(void) {
  // NEW: close debug file if any
  MutatorDebug::deinit();
  // nothing else
}

} // extern "C"
