#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fuzz::mutator::internal {

inline std::vector<uint8_t> parse_pattern(const std::string &text) {
  std::vector<uint8_t> out;
  std::string token;
  for (char c : text) {
    if (c == '[' || c == ']')
      continue;
    if (c == ',') {
      if (!token.empty()) {
        out.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 0)));
        token.clear();
      }
      continue;
    }
    if (!std::isspace(static_cast<unsigned char>(c)))
      token.push_back(c);
  }
  if (!token.empty())
    out.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 0)));
  return out;
}

inline uint64_t mask_bits(uint32_t width) {
  if (width == 0)
    return 0;
  if (width >= 32)
    return 0xFFFFFFFFull;
  return (1ull << width) - 1ull;
}

inline size_t clamp_cap(size_t value, size_t limit) {
  return value > limit ? limit : value;
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
