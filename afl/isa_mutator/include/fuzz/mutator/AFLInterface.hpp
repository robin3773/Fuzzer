/**
 * @file AFLInterface.hpp
 * @brief AFL++ custom mutator interface - C API bindings
 * 
 * This file declares the C linkage functions required by AFL++'s
 * custom mutator API. These functions are loaded dynamically by AFL++
 * via dlopen when AFL_CUSTOM_MUTATOR_LIBRARY is set.
 * 
 * @details
 * AFL++ loads this library and calls:
 * 1. afl_custom_init() - Initialize mutator, seed RNG
 * 2. afl_custom_mutator() / afl_custom_havoc_mutation() - Mutate test cases
 * 3. afl_custom_deinit() - Cleanup on exit
 * 
 * @see https://github.com/AFLplusplus/AFLplusplus/blob/stable/docs/custom_mutators.md
 * @note All functions must have C linkage (extern "C")
 */

#pragma once

#include <cstddef>

extern "C" {

/**
 * @brief Set configuration file path before initialization
 * @param path Absolute path to YAML config file
 * 
 * @note Must be called before afl_custom_init() if custom config needed
 */
void mutator_set_config_path(const char *path);

/**
 * @brief Get current active mutation strategy as string
 * @return Strategy name ("RAW", "IR", "HYBRID", "AUTO")
 */
const char *mutator_get_active_strategy();

/**
 * @brief Main AFL++ custom mutator entry point
 * @param afl AFL++ state pointer (unused by this mutator)
 * @param buf Input buffer to mutate
 * @param buf_size Size of input buffer
 * @param out_buf Output buffer pointer (set to mutated data)
 * @param max_size Maximum allowed output size
 * @return Size of mutated output, or 0 on error
 * 
 * Called by AFL++ for each mutation. Applies ISA-aware mutations
 * and returns pointer to mutated buffer.
 */
size_t afl_custom_mutator(void *afl,
                          unsigned char *buf,
                          size_t buf_size,
                          unsigned char **out_buf,
                          size_t max_size);

/**
 * @brief Havoc-stage custom mutation
 * @param afl AFL++ state pointer
 * @param buf Input buffer to mutate
 * @param buf_size Size of input buffer
 * @param out_buf Output buffer pointer
 * @param max_size Maximum allowed output size
 * @return Size of mutated output
 * 
 * Called during AFL++'s havoc stage for more aggressive mutations.
 */
size_t afl_custom_havoc_mutation(void *afl,
                                 unsigned char *buf,
                                 size_t buf_size,
                                 unsigned char **out_buf,
                                 size_t max_size);

/**
 * @brief Initialize custom mutator
 * @param afl AFL++ state pointer
 * @return 0 on success, non-zero on error
 * 
 * Called once by AFL++ at startup. Initializes mutator configuration,
 * loads ISA schema, and seeds RNG.
 */
int afl_custom_init(void *afl);

/**
 * @brief Cleanup custom mutator
 * 
 * Called by AFL++ on shutdown. Closes debug log files and frees resources.
 */
void afl_custom_deinit(void);

} // extern "C"
