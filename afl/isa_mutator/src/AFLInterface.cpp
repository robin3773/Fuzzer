#include <fuzz/mutator/AFLInterface.hpp>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <string>
#include <unistd.h>

#include <hwfuzz/Log.hpp>

#include <fuzz/mutator/DebugUtils.hpp>
#include <fuzz/mutator/ISAMutator.hpp>
#include <fuzz/mutator/Random.hpp>

namespace mut = fuzz::mutator;

namespace {

std::mutex &cli_mutex() {
  static std::mutex m;
  return m;
}

const char *strategy_token(mut::Strategy strategy) {
  switch (strategy) {
  case mut::Strategy::RAW:
    return "RAW";
  case mut::Strategy::IR:
    return "IR";
  case mut::Strategy::HYBRID:
    return "HYBRID";
  case mut::Strategy::AUTO:
    return "AUTO";
  }
  return "IR";
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
  std::fprintf(hwfuzz::harness_log(),
               "[INFO] custom mutator initialized. pid=%d time=%ld\n",
               static_cast<int>(getpid()),
               static_cast<long>(time(nullptr)));
  return 0;
}

const char *mutator_get_active_strategy() {
  mut::MutatorInterface &mutator = get_mutator();
  if (auto *isa = dynamic_cast<mut::ISAMutator *>(&mutator))
    return strategy_token(isa->strategy());
  return "IR";
}

void afl_custom_deinit(void) {
  std::fprintf(hwfuzz::harness_log(), "[] deinit\n");
  fuzz::mutator::debug::deinit();
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
