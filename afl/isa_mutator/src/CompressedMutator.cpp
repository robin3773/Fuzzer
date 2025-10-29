#include <fuzz/mutator/CompressedMutator.hpp>

#include <fuzz/mutator/Random.hpp>

namespace fuzz::mutator {

void CompressedMutator::mutateAt(unsigned char *buf,
                                 size_t byte_index,
                                 size_t buf_size,
                                 const Config &cfg) {
  if (!cfg.enable_c)
    return;
  if (byte_index + 1 >= buf_size)
    return;

  uint16_t c = isa::get_u16_le(buf, byte_index);
  uint8_t op_lo = static_cast<uint8_t>(c & 0x3u);
  uint8_t funct3 = static_cast<uint8_t>((c >> 13) & 0x7u);

  if ((op_lo == 0x0 && (funct3 == 0b010 || funct3 == 0b110)) ||
      (op_lo == 0x1 &&
       (funct3 == 0b000 || funct3 == 0b001 || funct3 == 0b101)) ||
      (op_lo == 0x2 && (funct3 == 0b010 || funct3 == 0b110))) {
    uint16_t mask = static_cast<uint16_t>(1u << (2 + (Random::rnd32() % 3)));
    c ^= mask;
    isa::put_u16_le(buf, byte_index, c);
    return;
  }

  c ^= static_cast<uint16_t>(1u << (Random::rnd32() & 15));
  isa::put_u16_le(buf, byte_index, c);
}

} // namespace fuzz::mutator
