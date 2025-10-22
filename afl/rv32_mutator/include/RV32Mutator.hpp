#pragma once
#include "Instruction.hpp"
#include "MutatorConfig.hpp"
#include <cstddef>

namespace mut {

class RV32Mutator {
public:
  RV32Mutator() = default;
  void initFromEnv();
  unsigned char *mutateStream(unsigned char *in, size_t in_len,
                              unsigned char *out_buf, size_t max_size);

private:
  Config cfg_;

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

  // Strategy selection
  bool shouldDecodeIR() const;
};

} // namespace mut
