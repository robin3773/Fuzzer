#pragma once
#include "Instruction.hpp"
#include "MutatorConfig.hpp"
#include <cstddef>
#include <cstdint>

namespace mut {

class RV32Mutator {
public:
  RV32Mutator() = default;

  void initFromEnv();

  // Main mutation API
  unsigned char *mutateStream(unsigned char *in, size_t in_len,
                              unsigned char *out_buf, size_t max_size);

  // ✅ AFL needs this to know how many bytes we actually produced
  inline size_t last_out_len() const { return last_len_; }

private:
  Config cfg_;

  // ✅ Tracks last output length for AFL
  size_t last_len_ = 0;

  // Mutations
  void mutateRaw32(uint32_t &v) const;
  void mutateIR32(uint32_t &v) const;
  void mutateRegs32(uint32_t &v) const;
  void mutateImm32(uint32_t &v) const;
  void replaceWithSameFmt32(uint32_t &v) const;

  // Helpers
  uint32_t pickReg() const;
  void uToggleOp(uint32_t &v) const;
  void uMutateImmSmall(uint32_t &v, int32_t pages_delta) const;

  // Strategy logic
  bool shouldDecodeIR() const;
};

} // namespace mut
