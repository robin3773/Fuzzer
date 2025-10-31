#pragma once

#include <cstddef>

extern "C" {

void mutator_set_config_path(const char *path);
const char *mutator_get_active_strategy();

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
