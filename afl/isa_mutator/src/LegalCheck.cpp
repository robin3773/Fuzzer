#include <fuzz/mutator/LegalCheck.hpp>

namespace fuzz::mutator {

namespace {

uint32_t extract_field(uint32_t word, const isa::FieldEncoding &enc) {
  if (enc.segments.empty())
    return 0;

  uint64_t value = 0;
  for (const auto &seg : enc.segments) {
    uint64_t mask = (seg.width >= 32) ? 0xFFFFFFFFull : ((1ull << seg.width) - 1ull);
    uint64_t segment = (static_cast<uint64_t>(word) >> seg.word_lsb) & mask;
    value |= segment << seg.value_lsb;
  }
  return static_cast<uint32_t>(value & 0xFFFFFFFFull);
}

} // namespace

bool is_legal_instruction(uint32_t insn32, const isa::ISAConfig &isa_cfg) {
  if (isa_cfg.instructions.empty())
    return false;

  for (const auto &spec : isa_cfg.instructions) {
    auto fmt_it = isa_cfg.formats.find(spec.format);
    if (fmt_it == isa_cfg.formats.end())
      continue;

    bool match = true;
    for (const auto &kv : spec.fixed_fields) {
      auto field_it = isa_cfg.fields.find(kv.first);
      if (field_it == isa_cfg.fields.end()) {
        match = false;
        break;
      }
      uint32_t actual = extract_field(insn32, field_it->second);
      uint64_t mask = (field_it->second.width >= 32)
                          ? 0xFFFFFFFFull
                          : ((1ull << field_it->second.width) - 1ull);
      uint32_t expected = static_cast<uint32_t>(kv.second & mask);
      if (actual != expected) {
        match = false;
        break;
      }
    }

    if (match)
      return true;
  }

  return false;
}

} // namespace fuzz::mutator
