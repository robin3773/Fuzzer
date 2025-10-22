#pragma once
#include <cstddef>

extern "C" {

// Modern AFL++ API
unsigned char *afl_custom_fuzz(unsigned char *buf, size_t buf_size,
                               unsigned char *out_buf, size_t max_size,
                               unsigned int seed);

// Older API variant
size_t afl_custom_fuzz_b(unsigned char *data, size_t size,
                         unsigned char **out_buf, unsigned int seed);

// Havoc alias
size_t afl_custom_havoc_mutation(unsigned char *data, size_t size,
                                 unsigned char **out_buf, unsigned int seed);

// Lifecycle
int afl_custom_init(void *afl);
void afl_custom_deinit(void);

} // extern "C"
