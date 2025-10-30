#include <fuzz/mutator/AFLInterface.hpp>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <string>
#include <unistd.h>

#include <fuzz/mutator/ISAMutator.hpp>
#include <fuzz/mutator/MutatorDebug.hpp>
#include <fuzz/mutator/Random.hpp>

namespace mut = fuzz::mutator;

namespace {

std::mutex &cli_mutex() {
  static std::mutex m;
  return m;
}

std::string &cli_config_path() {
  static std::string path;
  return path;
}

mut::MutatorInterface &get_mutator() {
  static mut::MutatorInterface *global = nullptr;
  static std::once_flag once;

  std::call_once(once, []() {
    auto *schema = new mut::ISAMutator();
    std::string snapshot;
    {
      std::lock_guard<std::mutex> lock(cli_mutex());
      snapshot = cli_config_path();
    }
    if (!snapshot.empty())
      schema->setConfigPath(snapshot);
    schema->initFromEnv();
    global = schema;
  });

  return *global;
}

} // namespace

extern "C" {

void mutator_set_config_path(const char *path) {
  std::lock_guard<std::mutex> lock(cli_mutex());
  cli_config_path() = path ? std::string(path) : std::string();
}

int afl_custom_init(void * /*afl*/) {
  (void)get_mutator();
  mut::Random::seed(static_cast<uint32_t>(time(nullptr)));
  std::fprintf(stderr,
               "[mutator] custom mutator initialized. pid=%d time=%ld\n",
               static_cast<int>(getpid()),
               static_cast<long>(time(nullptr)));
  return 0;
}

void afl_custom_deinit(void) {
  std::fprintf(stderr, "[mutator] deinit\n");
  MutatorDebug::deinit();
}

size_t afl_custom_mutator(void * /*afl*/,
                          unsigned char *buf,
                          size_t buf_size,
                          unsigned char **out_buf,
                          size_t max_size) {
  mut::MutatorInterface &mutator = get_mutator();

  unsigned char *result = mutator.mutateStream(buf, buf_size, nullptr, max_size);
  size_t out_len = mutator.last_out_len();

  if (!result)
    return 0;

  if (out_len == 0) {
    result[0] = 0;
    out_len = 1;
  }

  *out_buf = result;
  return out_len;
}

size_t afl_custom_havoc_mutation(void *afl,
                                 unsigned char *buf,
                                 size_t buf_size,
                                 unsigned char **out_buf,
                                 size_t max_size) {
  return afl_custom_mutator(afl, buf, buf_size, out_buf, max_size);
}

} // extern "C"
