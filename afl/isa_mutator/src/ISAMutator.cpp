#include <fuzz/mutator/ISAMutator.hpp>

#include <hwfuzz/Log.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <fuzz/mutator/CompressedMutator.hpp>
#include <fuzz/mutator/DebugUtils.hpp>
#include <fuzz/mutator/EncodeHelpers.hpp>
#include <fuzz/mutator/LegalCheck.hpp>
#include <fuzz/mutator/MutatorDebug.hpp>
#include <fuzz/mutator/Random.hpp>

namespace fuzz::mutator {

namespace {

const char *strategyToString(Strategy strategy) {
  static const char* names[] = {"RAW", "IR", "HYBRID", "AUTO"};
  return (strategy >= Strategy::RAW && strategy <= Strategy::AUTO) 
         ? names[static_cast<int>(strategy)] 
         : "UNKNOWN";
}

void logConfigSnapshot(const Config &cfg) {
  std::fprintf(hwfuzz::harness_log(),
               "[INFO] strategy=%s verbose=%s enable_c=%s decode_prob=%u imm_random_prob=%u r_weight_base_alu=%u r_weight_m=%u isa_name=%s\n",
               strategyToString(cfg.strategy),
               cfg.verbose ? "true" : "false",
               cfg.enable_c ? "true" : "false",
               cfg.decode_prob,
               cfg.imm_random_prob,
               cfg.r_weight_base_alu,
               cfg.r_weight_m,
               cfg.isa_name.c_str());
}

} // namespace

ISAMutator::ISAMutator() = default;

void ISAMutator::initFromEnv() {
  FunctionTracer tracer(__FILE__, "ISAMutator::initFromEnv");
  
  // Load config from environment
  cfg_ = loadConfigFromEnvOrDie();
  
  MutatorDebug::init_from_env();
  logConfigSnapshot(cfg_);
  
  // Load ISA schema (required)
  if (cfg_.isa_name.empty()) {
  std::fprintf(hwfuzz::harness_log(), "[ERROR] No ISA name specified in config\n");
    std::exit(1);
  }
  
  try {
    // ISA loader reads its own environment variables (SCHEMA_DIR, SCHEMA_MAP, SCHEMA_OVERRIDE)
    isa_ = isa::load_isa_config(cfg_.isa_name);
    
    if (isa_.instructions.empty()) {
  std::fprintf(hwfuzz::harness_log(), "[ERROR] No instructions in schema for ISA '%s'\n", cfg_.isa_name.c_str());
      std::exit(1);
    }
    
    use_schema_ = true;
    word_bytes_ = std::max<uint32_t>(1, isa_.base_width / 8);
    //FIXME - Have to Fix Later
    std::fprintf(hwfuzz::harness_log(), "[INFO] Loaded ISA '%s': %zu instructions\n", isa_.isa_name.c_str(), isa_.instructions.size());
  } catch (const std::exception &ex) {
    std::fprintf(hwfuzz::harness_log(), "[ERROR] Schema load failed: %s\n", ex.what());
    std::exit(1);
  }
}

unsigned char *ISAMutator::mutateStream(unsigned char *in, size_t in_len,
                                           unsigned char *out_buf, size_t max_size) {
  FunctionTracer tracer(__FILE__, "ISAMutator::mutateStream");
  (void)out_buf;
  return use_schema_ ? mutateWithSchema(in, in_len, nullptr, max_size)
                     : mutateFallback(in, in_len, nullptr, max_size);
}

unsigned char *ISAMutator::mutateWithSchema(unsigned char *in, size_t in_len,
                                               unsigned char *, size_t max_size) {
  FunctionTracer tracer(__FILE__, "ISAMutator::mutateWithSchema");
  if (isa_.instructions.empty())
    return nullptr;

  size_t word_bytes = std::max<size_t>(1, word_bytes_);
  size_t cap = max_size ? max_size : std::max(in_len, word_bytes);
  cap = std::max(cap, word_bytes);

  unsigned char *out = static_cast<unsigned char *>(std::malloc(cap));
  if (!out)
    return nullptr;

  // Copy input or zero-initialize
  size_t cur_len = std::min(in_len, cap);
  if (cur_len && in) {
    std::memcpy(out, in, cur_len);
  } else {
    std::memset(out, 0, cap);
    cur_len = word_bytes;
  }

  // Ensure minimum size
  if (cur_len < word_bytes) {
    std::memset(out + cur_len, 0, word_bytes - cur_len);
    cur_len = word_bytes;
  }

  size_t nwords = std::max<size_t>(1, cur_len / word_bytes);
  unsigned nmuts = 1 + (Random::rnd32() % 3);
  
  for (unsigned i = 0; i < nmuts; ++i) {
    size_t idx = Random::range(static_cast<uint32_t>(nwords));
    uint32_t encoded = encodeInstruction(pickInstruction());
    
    if (!isa_.fields.empty() && !is_legal_instruction(encoded, isa_))
      continue;
    
    writeWord(out, idx * word_bytes, encoded);
  }

  last_len_ = cur_len;
  return out;
}

unsigned char *ISAMutator::mutateFallback(unsigned char *in, size_t in_len,
                                             unsigned char *, size_t max_size) {
  FunctionTracer tracer(__FILE__, "ISAMutator::mutateFallback");
  size_t cap = max_size ? max_size : std::max<size_t>(in_len, 1);
  
  unsigned char *out = static_cast<unsigned char *>(std::malloc(cap));
  if (!out)
    return nullptr;

  size_t cur_len = std::min(in_len, cap);
  if (cur_len && in) {
    std::memcpy(out, in, cur_len);
  } else {
    out[0] = 0;
    cur_len = 1;
  }

  uint32_t total_weight = 0;
  for (const auto &rule : rules_)
    total_weight += rule.weight;
  total_weight = std::max<uint32_t>(1, total_weight);

  unsigned nmuts = 1 + (Random::rnd32() % 3);
  for (unsigned i = 0; i < nmuts; ++i) {
    uint32_t pick = Random::range(total_weight);
    uint32_t acc = 0;
    
    for (const auto &rule : rules_) {
      acc += rule.weight;
      if (pick < acc) {
        applyRule(rule, out, cur_len, cap);
        break;
      }
    }
  }

  last_len_ = std::max<size_t>(cur_len, 1);
  return out;
}

const isa::InstructionSpec &ISAMutator::pickInstruction() const {
  FunctionTracer tracer(__FILE__, "ISAMutator::pickInstruction");
  if (isa_.instructions.empty())
    throw std::runtime_error("No instructions available");
  uint32_t idx = Random::range(static_cast<uint32_t>(isa_.instructions.size()));
  return isa_.instructions[idx];
}

uint32_t ISAMutator::encodeInstruction(const isa::InstructionSpec &spec) const {
  FunctionTracer tracer(__FILE__, "ISAMutator::encodeInstruction");
  auto fmt_it = isa_.formats.find(spec.format);
  if (fmt_it == isa_.formats.end())
    return Random::rnd32();

  uint32_t word = 0;
  for (const auto &field_name : fmt_it->second.fields) {
    auto field_it = isa_.fields.find(field_name);
    if (field_it == isa_.fields.end())
      continue;

    // Use fixed field value if available, otherwise generate random
    auto fixed_it = spec.fixed_fields.find(field_name);
    uint32_t value = (fixed_it != spec.fixed_fields.end()) 
                     ? fixed_it->second 
                     : randomFieldValue(field_name, field_it->second);
    
    applyField(word, field_it->second, value);
  }
  return word;
}

namespace {
uint32_t getUniformMaskedValue(uint32_t width, uint64_t mask) {
  return width >= 32 ? Random::rnd32() 
                     : Random::range(static_cast<uint32_t>(mask) + 1);
}

int64_t getSignedRandom(uint32_t width) {
  int64_t span = 1ll << (width - 1);
  int64_t range = (span << 1);
  return -span + static_cast<int64_t>(Random::range(static_cast<uint32_t>(range)));
}
} // namespace

uint32_t ISAMutator::randomFieldValue(const std::string &field_name,
                                        const isa::FieldEncoding &enc) const {
  FunctionTracer tracer(__FILE__, "ISAMutator::randomFieldValue");
  if (enc.width == 0)
    return 0;

  uint64_t mask = internal::mask_bits(enc.width);

  // Register fields
  if (enc.kind == isa::FieldKind::Register || enc.kind == isa::FieldKind::Floating) {
    uint32_t limit = isa_.register_count ? isa_.register_count : 32;
    uint32_t value = Random::range(limit);
    
    // Bias away from x0 for destination registers
    if ((field_name == "rd" || field_name == "rd_rs1") && value == 0 && limit > 1 && Random::chancePct(80))
      value = 1 + Random::range(limit - 1);
    
    return value & mask;
  }

  // Immediate fields
  if (enc.kind == isa::FieldKind::Immediate && enc.is_signed && enc.width < 32 && enc.width > 0) {
    int64_t pick = getSignedRandom(enc.width);
    
    // Bias towards small values
    if (isa_.defaults.hints.signed_immediates_bias) {
      if (Random::chancePct(30))
        pick = 0;
      else if (Random::chancePct(30))
        pick = Random::chancePct(50) ? 1 : -1;
    }
    return static_cast<uint32_t>(pick) & mask;
  }

  // Default: random value for field width
  return enc.is_signed && enc.width < 32 && enc.width > 0
         ? static_cast<uint32_t>(getSignedRandom(enc.width)) & mask
         : getUniformMaskedValue(enc.width, mask);
}

void ISAMutator::applyField(uint32_t &word, const isa::FieldEncoding &enc, uint32_t value) const {
  FunctionTracer tracer(__FILE__, "ISAMutator::applyField");
  if (enc.segments.empty())
    return;

  uint64_t masked = (enc.width && enc.width < 32) ? (value & internal::mask_bits(enc.width)) : value;
  uint64_t w = word;
  
  for (const auto &seg : enc.segments) {
    uint64_t seg_mask = internal::mask_bits(seg.width);
    uint64_t seg_value = (masked >> seg.value_lsb) & seg_mask;
    w = (w & ~(seg_mask << seg.word_lsb)) | ((seg_value & seg_mask) << seg.word_lsb);
  }
  
  word = static_cast<uint32_t>(w);
}

void ISAMutator::writeWord(unsigned char *buf, size_t offset, uint32_t word) const {
  FunctionTracer tracer(__FILE__, "ISAMutator::writeWord");
  if (word_bytes_ == 2)
    internal::store_u16_le(buf, offset, static_cast<uint16_t>(word));
  else if (word_bytes_ == 4)
    internal::store_u32_le(buf, offset, word);
  else
    for (uint32_t i = 0; i < word_bytes_ && i < 4; ++i)
      buf[offset + i] = static_cast<unsigned char>((word >> (8 * i)) & 0xFF);
}

std::string ISAMutator::trim(const std::string &s) {
  FunctionTracer tracer(__FILE__, "ISAMutator::trim");
  auto begin = s.find_first_not_of(" \t\n\r");
  if (begin == std::string::npos)
    return "";
  auto end = s.find_last_not_of(" \t\n\r");
  return s.substr(begin, end - begin + 1);
}

bool ISAMutator::loadFallbackConfig(const std::string &path) {
  FunctionTracer tracer(__FILE__, "ISAMutator::loadFallbackConfig");
  std::ifstream ifs(path);
  if (!ifs)
    return false;

  rules_.clear();
  Rule *cur = nullptr;
  std::string line;
  
  while (std::getline(ifs, line)) {
    std::string s = trim(line);
    if (s.empty() || s[0] == '#')
      continue;
      
    if (s.find("type:") == 0 || s.find("- type:") == 0) {
      rules_.push_back(Rule{});
      cur = &rules_.back();
      cur->type = trim(s.substr(s.find(':') + 1));
    } else if (cur) {
      if (s.find("weight:") == 0)
        cur->weight = std::stoul(trim(s.substr(7)));
      else if (s.find("min:") == 0)
        cur->min = std::stoul(trim(s.substr(4)));
      else if (s.find("max:") == 0)
        cur->max = std::stoul(trim(s.substr(4)));
      else if (s.find("count:") == 0) {
        std::string v = trim(s.substr(6));
        size_t dash = v.find('-');
        if (dash == std::string::npos)
          cur->min = cur->max = std::stoul(v);
        else {
          cur->min = std::stoul(trim(v.substr(0, dash)));
          cur->max = std::stoul(trim(v.substr(dash + 1)));
        }
      } else if (s.find("pattern:") == 0)
        cur->pattern = internal::parse_pattern(trim(s.substr(8)));
    }
  }
  return !rules_.empty();
}

void ISAMutator::applyRule(const Rule &r, unsigned char *buf, size_t &len, size_t cap) {
  FunctionTracer tracer(__FILE__, "ISAMutator::applyRule");
  uint32_t n = r.min + (r.max > r.min ? Random::range(r.max - r.min + 1) : 0);
  
  if (r.type == "byte_flip") {
    for (uint32_t i = 0; i < n && len > 0; ++i) {
      size_t idx = Random::range(static_cast<uint32_t>(len));
      buf[idx] ^= (1 << Random::range(8));
    }
  }
  else if (r.type == "insert_pattern" && !r.pattern.empty() && len < cap) {
    size_t pos = len ? Random::range(static_cast<uint32_t>(len + 1)) : 0;
    size_t patlen = std::min(r.pattern.size(), cap - len);
    std::memmove(buf + pos + patlen, buf + pos, len - pos);
    std::memcpy(buf + pos, r.pattern.data(), patlen);
    len = std::min(len + patlen, cap);
  }
  else if (r.type == "swap_chunks" && len >= 2) {
    size_t a = Random::range(static_cast<uint32_t>(len));
    size_t b = Random::range(static_cast<uint32_t>(len));
    size_t sz = 1 + Random::range(static_cast<uint32_t>(std::max<size_t>(1, len / 8)));
    size_t asz = std::min(sz, len - a);
    size_t bsz = std::min(sz, len - b);
    std::vector<uint8_t> tmp(asz);
    std::memcpy(tmp.data(), buf + a, asz);
    std::memmove(buf + a, buf + b, bsz);
    std::memcpy(buf + b, tmp.data(), asz);
  }
  else if (r.type == "truncate" && len > 0) {
    len = (n >= len) ? 1 : len - n;
  }
  else if (r.type == "duplicate_chunk" && len > 0 && len < cap) {
    size_t pos = Random::range(static_cast<uint32_t>(len));
    size_t sz = 1 + Random::range(static_cast<uint32_t>(std::min<size_t>(len - pos, 4)));
    size_t inspos = Random::range(static_cast<uint32_t>(len + 1));
    size_t copylen = std::min(sz, cap - len);
    std::memmove(buf + inspos + copylen, buf + inspos, len - inspos);
    std::memcpy(buf + inspos, buf + pos, copylen);
    len = std::min(len + copylen, cap);
  }
}

} // namespace fuzz::mutator
