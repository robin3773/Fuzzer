#pragma once

#include <cstddef>
#include <cstdint>

namespace fuzz::mutator::internal {

inline uint64_t mask_bits(uint32_t width) {
  if (width == 0)
    return 0;
  if (width >= 32)
    return 0xFFFFFFFFull;
  return (1ull << width) - 1ull;
}

inline uint16_t load_u16_le(const unsigned char *buf, size_t offset) {
  return static_cast<uint16_t>(buf[offset]) |
         static_cast<uint16_t>(buf[offset + 1]) << 8;
}

inline uint32_t load_u32_le(const unsigned char *buf, size_t offset) {
  return static_cast<uint32_t>(buf[offset]) |
         (static_cast<uint32_t>(buf[offset + 1]) << 8) |
         (static_cast<uint32_t>(buf[offset + 2]) << 16) |
         (static_cast<uint32_t>(buf[offset + 3]) << 24);
}

inline void store_u16_le(unsigned char *buf, size_t offset, uint16_t value) {
  buf[offset] = static_cast<unsigned char>(value & 0xFFu);
  buf[offset + 1] = static_cast<unsigned char>((value >> 8) & 0xFFu);
}

inline void store_u32_le(unsigned char *buf, size_t offset, uint32_t value) {
  buf[offset] = static_cast<unsigned char>(value & 0xFFu);
  buf[offset + 1] = static_cast<unsigned char>((value >> 8) & 0xFFu);
  buf[offset + 2] = static_cast<unsigned char>((value >> 16) & 0xFFu);
  buf[offset + 3] = static_cast<unsigned char>((value >> 24) & 0xFFu);
}

} // namespace fuzz::mutator::internal
