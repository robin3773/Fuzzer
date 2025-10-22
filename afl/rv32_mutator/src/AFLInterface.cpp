#include "AFLInterface.hpp"
#include "MutatorDebug.hpp"
#include "RV32Mutator.hpp"
#include "Random.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

static mut::RV32Mutator g_mut;

extern "C" {

int afl_custom_init(void * /*afl*/) {
  g_mut.initFromEnv();
  mut::Random::seed((uint32_t)time(nullptr));
  std::fprintf(stderr, "[mutator] RV32 hybrid mutator initialized.\n");
  return 0;
}

void afl_custom_deinit(void) { MutatorDebug::deinit(); }

unsigned char *afl_custom_fuzz(unsigned char *buf, size_t buf_size,
                               unsigned char *out_buf, size_t max_size,
                               unsigned int seed) {
  mut::Random::seed(seed);
  return g_mut.mutateStream(buf, buf_size, out_buf, max_size);
}

size_t afl_custom_fuzz_b(unsigned char *data, size_t size,
                         unsigned char **out_buf, unsigned int seed) {
  mut::Random::seed(seed);
  unsigned char *res = g_mut.mutateStream(data, size, nullptr, 0);
  if (!res)
    return 0;
  *out_buf = res;
  return size;
}

size_t afl_custom_havoc_mutation(unsigned char *data, size_t size,
                                 unsigned char **out_buf, unsigned int seed) {
  return afl_custom_fuzz_b(data, size, out_buf, seed);
}

} // extern "C"
