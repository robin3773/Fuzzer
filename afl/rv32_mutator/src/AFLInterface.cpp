#include "AFLInterface.hpp"
#include "MutatorDebug.hpp"
#include "RV32Mutator.hpp"
#include "Random.hpp"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <unistd.h>   // getpid()

using mut::RV32Mutator;

// Single global mutator, init once
static RV32Mutator &get_mut() {
  static RV32Mutator g_mut;
  static std::once_flag once;
  std::call_once(once, []() { g_mut.initFromEnv(); });
  return g_mut;
}

extern "C" {

// AFL++ calls this once per forkserver instance
  int afl_custom_init(void * /*afl*/) {
    (void)get_mut();
    mut::Random::seed((uint32_t)time(nullptr));
    std::fprintf(stderr,
                "[mutator] RV32 hybrid mutator initialized. pid=%d time=%ld\n",
                (int)getpid(), (long)time(nullptr));
    return 0;
  }

  void afl_custom_deinit(void) {
    std::fprintf(stderr, "[mutator] deinit\n");
    MutatorDebug::deinit();
  }

/*
 * Classic AFL++ API: afl_custom_mutator()
 * - We ignore AFL's out_buf and allocate our own in mutateStream()
 * - We return the exact produced length via last_out_len()
 */
  size_t afl_custom_mutator(/* afl_state_t *afl */ void * /*unused*/,
                          unsigned char *buf, size_t buf_size,
                          unsigned char **out_buf, size_t max_size) {
  RV32Mutator &m = get_mut();

  // Optional: reseed per-call based on time; AFL also passes seeds via other APIs.
  // mut::Random::seed((uint32_t)time(nullptr));

  unsigned char *res = m.mutateStream(buf, buf_size, nullptr, max_size);
  size_t out_len = m.last_out_len();

  // // std::fprintf(stderr,
  //              "[mutator/mutator] in_len=%zu max_size=%zu -> out=%p out_len=%zu\n",
  //              buf_size, max_size, (void *)res, out_len);

  if (!res) return 0;
  if (out_len == 0) { res[0] = 0; out_len = 1; }

  *out_buf = res;
  return out_len;
}

/*
 * Havoc path: reuse afl_custom_mutator semantics
 */
size_t afl_custom_havoc_mutation(/* afl_state_t *afl */ void * /*unused*/,
                                 unsigned char *buf, size_t buf_size,
                                 unsigned char **out_buf, size_t max_size) {
  size_t n = afl_custom_mutator(nullptr, buf, buf_size, out_buf, max_size);
  return n;
}

} // extern "C"
