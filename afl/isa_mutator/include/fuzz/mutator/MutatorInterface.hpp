#pragma once

#include <cstddef>
#include <string>

namespace fuzz::mutator {

/**
 * @interface MutatorInterface
 * @brief Abstract interface for custom AFL++ mutators
 * 
 * Defines the contract that all mutators must implement for integration
 * with the AFL++ fuzzing framework.
 */
class MutatorInterface {
public:
  virtual ~MutatorInterface() = default;

  /**
   * @brief Initialize mutator from environment variables
   * Called once at startup to load configuration and ISA schemas.
   */
  virtual void initFromEnv() = 0;

  /**
   * @brief Mutate an input buffer and return the mutated result
   * 
   * @param in Input buffer to mutate (unmodified)
   * @param in_len Length of input buffer in bytes
   * @param out_buf Optional pre-allocated output buffer (may be nullptr)
   *                Implementations may ignore this and allocate their own buffer
   * @param max_size Maximum size constraint for output (soft limit)
   * @return Pointer to mutated buffer (may be newly allocated or out_buf)
   * 
   * @note If implementation allocates new buffer, caller must free it
   * @note out_buf parameter is optional - implementations may ignore it
   */
  virtual unsigned char *mutateStream(unsigned char *in, size_t in_len,
                                      unsigned char *out_buf,
                                      size_t max_size) = 0;

  /**
   * @brief Get the size of the last mutation output
   * @return Size in bytes of buffer returned by last mutateStream() call
   */
  virtual size_t last_out_len() const = 0;

  /**
   * @brief Set configuration file path before initialization
   * @param path Absolute path to configuration file
   */
  virtual void setConfigPath(const std::string &path) { (void)path; }
};

} // namespace fuzz::mutator
