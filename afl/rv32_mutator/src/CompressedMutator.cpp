// 


#include "CompressedMutator.hpp"

namespace mut {

void CompressedMutator::mutateAt(unsigned char* buf, size_t byte_i, size_t buf_size, const Config& cfg) {
  if (!cfg.enable_c) return;
  if (byte_i + 1 >= buf_size) return;
  uint16_t c = get_u16_le(buf, byte_i);
  uint8_t op_lo = c & 0x3u;
  uint8_t funct3 = (c >> 13) & 0x7u;

  // Examples adapted from a conservative strategy
  if ((op_lo == 0x0 && funct3 == 0b010) || // c.lw
      (op_lo == 0x0 && funct3 == 0b110) || // c.sw
      (op_lo == 0x2 && funct3 == 0b010)) { // c.lwsp
    // flip one immediate bit among upper range
    uint16_t mask = (uint16_t)(1u << (4 + (Random::rnd32() & 3)));
    c ^= mask;
    put_u16_le(buf, byte_i, c);
    return;
  }

  if ((op_lo == 0x1 && (funct3 == 0b001 || funct3 == 0b101)) || // c.jal/c.j
      (op_lo == 0x1 && (funct3 == 0b110 || funct3 == 0b111))) { // c.beqz/c.bnez
    uint16_t mask = (uint16_t)(1u << (1 + (Random::rnd32() % 10)));
    c ^= mask;
    put_u16_le(buf, byte_i, c);
    return;
  }

  // CR/CI groups: tweak compressed reg bits (conservative)
  if ((op_lo == 0x2 && funct3 == 0b100) ||
      (op_lo == 0x1 && funct3 == 0b000) ||
      (op_lo == 0x2 && funct3 == 0b000)) {
    uint16_t mask = (uint16_t)(1u << (2 + (Random::rnd32() % 3)));
    c ^= mask;
    put_u16_le(buf, byte_i, c);
    return;
  }

  // fallback small flip
  c ^= (uint16_t)(1u << (Random::rnd32() & 15));
  put_u16_le(buf, byte_i, c);
}

} // namespace mut
