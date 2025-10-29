#pragma once

#include <algorithm>
#include <cctype>
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

} // namespace fuzz::mutator::internal
