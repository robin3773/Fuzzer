#pragma once
#include <cstddef>

extern "C" {

// Classic AFL++ API we’ll use (stable ABI)
size_t afl_custom_mutator(void *afl /*unused*/,
                          unsigned char *buf, size_t buf_size,
                          unsigned char **out_buf, size_t max_size);

size_t afl_custom_havoc_mutation(void *afl /*unused*/,
                                 unsigned char *buf, size_t buf_size,
                                 unsigned char **out_buf, size_t max_size);

// Lifecycle
int  afl_custom_init(void *afl);
void afl_custom_deinit(void);

} // extern "C"
