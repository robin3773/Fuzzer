#pragma once

#include <cstddef>

extern "C" {

size_t afl_custom_mutator(void *afl,
                          unsigned char *buf,
                          size_t buf_size,
                          unsigned char **out_buf,
                          size_t max_size);

size_t afl_custom_havoc_mutation(void *afl,
                                 unsigned char *buf,
                                 size_t buf_size,
                                 unsigned char **out_buf,
                                 size_t max_size);

int afl_custom_init(void *afl);
void afl_custom_deinit(void);

} // extern "C"
