#include "CompressedMutator.hpp"

namespace mut {

void CompressedMutator::mutateAt(unsigned char *buf, size_t byte_i,
                                 size_t buf_size, const Config &cfg) {
  if (!cfg.enable_c)
    return;
  if (byte_i + 1 >= buf_size)
    return;
  uint16_t c = get_u16_le(buf, byte_i);
  uint8_t op_lo = c & 0x3u;
  uint8_t funct3 = (c >> 13) & 0x7u;

  // Very conservative tweaks: flip a small immediate or rd'/rs'
  if ((op_lo == 0x0 && (funct3 == 0b010 || funct3 == 0b110)) || // c.lw/c.sw
      (op_lo == 0x1 &&
       (funct3 == 0b000 || funct3 == 0b001 || funct3 == 0b101)) || // addi/jal/j
      (op_lo == 0x2 && (funct3 == 0b010 || funct3 == 0b110))) {    // lwsp/swsp
    uint16_t mask = (uint16_t)(1u << (2 + (mut::Random::rnd32() % 3)));
    c ^= mask;
    put_u16_le(buf, byte_i, c);
    return;
  }

  // fallback small flip
  c ^= (uint16_t)(1u << (mut::Random::rnd32() & 15));
  put_u16_le(buf, byte_i, c);
}

} // namespace mut
