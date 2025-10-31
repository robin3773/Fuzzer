#include <fuzz/mutator/ISAMutator.hpp>

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
#include <fuzz/mutator/LegalCheck.hpp>
#include <fuzz/mutator/MutatorDebug.hpp>
#include <fuzz/mutator/Random.hpp>

#include "encode_helpers.hpp"

namespace {
class FunctionTracer {
public:
  FunctionTracer(const char *file, const char *func)
      : base_(basename(file)), func_(func) {
    std::fprintf(stderr, "[Fn Start  ]  %s::%s\n", base_, func_);
  }
  ~FunctionTracer() {
    std::fprintf(stderr, "[Fn End    ]  %s::%s\n", base_, func_);
  }

private:
  static const char *basename(const char *path) {
    if (!path)
      return "";
    const char *slash = std::strrchr(path, '/');
    const char *backslash = std::strrchr(path, '\\');
    const char *last = slash;
    if (!last || (backslash && backslash > last))
      last = backslash;
    return last ? last + 1 : path;
  }

  const char *base_;
  const char *func_;
};
}

namespace fuzz::mutator {

namespace {

const char *strategyToString(Strategy strategy) {
  switch (strategy) {
  case Strategy::RAW:
    return "RAW";
  case Strategy::IR:
    return "IR";
  case Strategy::HYBRID:
    return "HYBRID";
  case Strategy::AUTO:
    return "AUTO";
  }
  return "IR";
}

void logConfigSnapshot(const Config &cfg) {
  std::fprintf(stderr,
               "[mutator-config] strategy=%s verbose=%s enable_c=%s decode_prob=%u imm_random_prob=%u r_weight_base_alu=%u r_weight_m=%u isa_name=%s schema_dir=%s schema_map=%s schema_override=%s\n",
               strategyToString(cfg.strategy),
               cfg.verbose ? "true" : "false",
               cfg.enable_c ? "true" : "false",
               cfg.decode_prob,
               cfg.imm_random_prob,
               cfg.r_weight_base_alu,
               cfg.r_weight_m,
               cfg.isa_name.c_str(),
               cfg.schema_dir.c_str(),
               cfg.schema_map.c_str(),
               cfg.schema_override.c_str());
}

} // namespace

ISAMutator::ISAMutator() {
  FunctionTracer tracer(__FILE__, "ISAMutator::ISAMutator");
}

void ISAMutator::initFromEnv() {
  FunctionTracer tracer(__FILE__, "ISAMutator::initFromEnv");
  std::vector<std::string> applied_configs;
  std::string env_config_path;
  if (const char *cfg_path = std::getenv("MUTATOR_CONFIG"))
    env_config_path = cfg_path;

  auto attempt_load = [&](const std::string &path, bool required) {
    if (path.empty())
      return false;
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path resolved(path);
    if (!fs::exists(resolved, ec)) {
      if (required) {
        std::fprintf(stderr,
                     "[mutator] Requested config '%s' not found\n",
                     path.c_str());
      }
      return false;
    }
    try {
      if (cfg_.loadFromFile(resolved.string())) {
        applied_configs.push_back(resolved.string());
        return true;
      }
    } catch (const std::exception &ex) {
      std::fprintf(stderr,
                   "[mutator] Failed to load config '%s': %s\n",
                   resolved.string().c_str(), ex.what());
      return false;
    }
    if (required) {
      std::fprintf(stderr,
                   "[mutator] Config '%s' was empty\n",
                   resolved.string().c_str());
    }
    return false;
  };

  auto load_profile_config = [&](const std::string &isa_name) -> bool {
    if (isa_name.empty())
      return false;
    std::string path = "afl/isa_mutator/config/" + isa_name + ".yaml";
    return attempt_load(path, false);
  };

  std::string profile_loaded;
  attempt_load("afl/isa_mutator/config/mutator.default.yaml", false);
  attempt_load("afl/isa_mutator/config/mutator.yaml", false);
  if (load_profile_config(cfg_.isa_name))
    profile_loaded = cfg_.isa_name;

  if (!env_config_path.empty()) {
    if (attempt_load(env_config_path, true) && cfg_.isa_name != profile_loaded) {
      if (load_profile_config(cfg_.isa_name))
        profile_loaded = cfg_.isa_name;
    }
  }

  if (!cli_config_path_.empty()) {
    if (attempt_load(cli_config_path_, true) && cfg_.isa_name != profile_loaded) {
      if (load_profile_config(cfg_.isa_name))
        profile_loaded = cfg_.isa_name;
    }
  }

  cfg_.applyEnvironment();
  if (!cfg_.isa_name.empty() && cfg_.isa_name != profile_loaded) {
    if (load_profile_config(cfg_.isa_name)) {
      profile_loaded = cfg_.isa_name;
      cfg_.applyEnvironment();
    }
  }
  MutatorDebug::init_from_env();

  logConfigSnapshot(cfg_);

  const char *dump_target = std::getenv("MUTATOR_EFFECTIVE_CONFIG");
  if (dump_target && *dump_target) {
    try {
      cfg_.dumpToFile(dump_target);
    } catch (const std::exception &ex) {
      std::fprintf(stderr,
                   "[mutator] Failed to dump effective config to %s: %s\n",
                   dump_target,
                   ex.what());
    }
  }

  std::string config_summary;
  if (applied_configs.empty()) {
    config_summary = "<built-in defaults>";
  } else if (applied_configs.size() == 1) {
    config_summary = applied_configs.back();
  } else {
    config_summary = applied_configs.back() +
                     " (+" + std::to_string(applied_configs.size() - 1) + " layered)";
  }

  std::fprintf(stderr,
               "[mutator] config: %s (env/CLI overrides applied)\n",
               config_summary.c_str());

  std::string schema_root = cfg_.schema_dir.empty() ? "./schemas" : cfg_.schema_dir;
  if (cfg_.isa_name.empty()) {
    std::fprintf(stderr,
                 "[mutator] No ISA specified in configuration; schema mutator disabled.\n");
    use_schema_ = false;
  } else {
    isa::SchemaLocator locator;
    locator.root_dir = schema_root;
    locator.isa_name = cfg_.isa_name;
    locator.map_path = cfg_.schema_map;
    locator.override_path = cfg_.schema_override;

    try {
      isa_ = isa::load_isa_config(locator);
      if (isa_.instructions.empty())
        throw std::runtime_error("schema contained no instructions");
      use_schema_ = true;
      word_bytes_ = std::max<uint32_t>(1, isa_.base_width / 8);
      std::fprintf(stderr,
                   "[mutator] schema: %s base_width=%u regs=%u formats=%zu instructions=%zu\n",
                   isa_.isa_name.c_str(),
                   isa_.base_width,
                   isa_.register_count,
                   isa_.formats.size(),
                   isa_.instructions.size());
    } catch (const std::exception &ex) {
      std::fprintf(stderr,
                   "[mutator] Schema load failed (%s). Falling back to rule mutator.\n",
                   ex.what());
      use_schema_ = false;
    }
  }

  if (!use_schema_) {
    const char *path = std::getenv("MUTATOR_YAML");
    if (!(path && loadFallbackConfig(path))) {
      const char *dir = std::getenv("MUTATOR_DIR");
      const char *isa_name = std::getenv("MUTATOR_ISA");
      if (dir && isa_name) {
        std::string p = std::string(dir) + "/" + std::string(isa_name) + ".yaml";
        if (!loadFallbackConfig(p)) {
          std::string p2 = std::string(dir) + "/riscv/" + std::string(isa_name) + ".yaml";
          loadFallbackConfig(p2);
        }
      }
    }
    if (rules_.empty())
      loadFallbackConfig("mutator.yaml");
  }

  if (!use_schema_ && rules_.empty()) {
    Rule r1{.type = "byte_flip", .weight = 50, .min = 1, .max = 4, .pattern = {}};
    Rule r2{.type = "insert_pattern", .weight = 25, .min = 1, .max = 1, .pattern = {0x13}};
    Rule r3{.type = "swap_chunks", .weight = 15, .min = 1, .max = 4, .pattern = {}};
    Rule r4{.type = "truncate", .weight = 10, .min = 1, .max = 4, .pattern = {}};
    rules_ = {r1, r2, r3, r4};
  }
}

unsigned char *ISAMutator::mutateStream(unsigned char *in,
                                           size_t in_len,
                                           unsigned char *out_buf,
                                           size_t max_size) {
  FunctionTracer tracer(__FILE__, "ISAMutator::mutateStream");
  (void)out_buf;
  return use_schema_ ? mutateWithSchema(in, in_len, nullptr, max_size)
                     : mutateFallback(in, in_len, nullptr, max_size);
}

unsigned char *ISAMutator::mutateWithSchema(unsigned char *in,
                                               size_t in_len,
                                               unsigned char * /*out_buf*/,
                                               size_t max_size) {
  FunctionTracer tracer(__FILE__, "ISAMutator::mutateWithSchema");
  last_len_ = 0;
  if (isa_.instructions.empty())
    return nullptr;

  const size_t word_bytes = std::max<size_t>(1, word_bytes_);
  size_t cap = max_size ? max_size : (in_len ? in_len : word_bytes);
  if (cap < word_bytes)
    cap = word_bytes;

  unsigned char *out = static_cast<unsigned char *>(std::malloc(cap));
  if (!out)
    return nullptr;

  size_t cur_len = std::min(in_len, cap);
  if (cur_len && in)
    std::memcpy(out, in, cur_len);
  else
    std::memset(out, 0, cap);

  if (cur_len < word_bytes) {
    std::memset(out + cur_len, 0, std::min(word_bytes - cur_len, cap - cur_len));
    cur_len = std::min(word_bytes, cap);
  }

  size_t nwords = cur_len / word_bytes;
  if (nwords == 0) {
    nwords = 1;
    cur_len = word_bytes;
  }

  unsigned nmuts = 1 + (Random::rnd32() % 3u);
  for (unsigned i = 0; i < nmuts; ++i) {
  size_t idx = Random::range(static_cast<uint32_t>(nwords));
    const isa::InstructionSpec &spec = pickInstruction();
    uint32_t encoded = encodeInstruction(spec);
    if (!isa_.instructions.empty() && !isa_.fields.empty()) {
      if (!is_legal_instruction(encoded, isa_))
        continue;
    }
    writeWord(out, idx * word_bytes, encoded);
  }

  last_len_ = cur_len;
  return out;
}

unsigned char *ISAMutator::mutateFallback(unsigned char *in,
                                             size_t in_len,
                                             unsigned char * /*out_buf*/,
                                             size_t max_size) {
  FunctionTracer tracer(__FILE__, "ISAMutator::mutateFallback");
  last_len_ = 0;
  size_t cap = max_size ? max_size : (in_len ? in_len : 1);
  if (cap == 0)
    cap = 1;

  unsigned char *out = static_cast<unsigned char *>(std::malloc(cap));
  if (!out)
    return nullptr;

  if (in && in_len)
    std::memmove(out, in, std::min(in_len, cap));
  size_t cur_len = std::min(in_len, cap);
  if (cur_len == 0) {
    out[0] = 0;
    last_len_ = 1;
    return out;
  }

  uint32_t total = 0;
  for (const auto &rule : rules_)
    total += rule.weight;
  if (total == 0)
    total = 1;

  unsigned nmuts = 1 + (Random::rnd32() % 3u);
  for (unsigned i = 0; i < nmuts; ++i) {
  uint32_t pick = Random::range(total);
    uint32_t acc = 0;
    const Rule *selected = rules_.empty() ? nullptr : &rules_[0];
    for (const auto &rule : rules_) {
      acc += rule.weight;
      if (pick < acc) {
        selected = &rule;
        break;
      }
    }
    if (selected)
      applyRule(*selected, out, cur_len, cap);
  }

  last_len_ = cur_len ? cur_len : 1;
  return out;
}

const isa::InstructionSpec &ISAMutator::pickInstruction() const {
  FunctionTracer tracer(__FILE__, "ISAMutator::pickInstruction");
  size_t count = isa_.instructions.size();
  if (count == 0)
    throw std::runtime_error("ISA instructions list is empty");
  uint32_t idx = Random::range(static_cast<uint32_t>(count));
  return isa_.instructions[idx % count];
}

uint32_t ISAMutator::encodeInstruction(const isa::InstructionSpec &spec) const {
  FunctionTracer tracer(__FILE__, "ISAMutator::encodeInstruction");
  auto fmt_it = isa_.formats.find(spec.format);
  if (fmt_it == isa_.formats.end())
  return Random::rnd32();

  uint32_t word = 0;
  const isa::FormatSpec &fmt = fmt_it->second;

  for (const auto &field_name : fmt.fields) {
    auto field_it = isa_.fields.find(field_name);
    if (field_it == isa_.fields.end())
      continue;

    uint32_t value = 0;
    auto fixed_it = spec.fixed_fields.find(field_name);
    if (fixed_it != spec.fixed_fields.end()) {
      value = fixed_it->second;
    } else {
      value = randomFieldValue(field_name, field_it->second);
    }
    applyField(word, field_it->second, value);
  }

  return word;
}

uint32_t ISAMutator::randomFieldValue(const std::string &field_name,
                                         const isa::FieldEncoding &enc) const {
  FunctionTracer tracer(__FILE__, "ISAMutator::randomFieldValue");
  if (enc.width == 0)
    return 0;

  uint64_t mask = internal::mask_bits(enc.width);

  auto uniform_masked = [&](uint64_t width_mask) {
    if (enc.width >= 32)
      return Random::rnd32();
    return static_cast<uint32_t>(Random::range(static_cast<uint32_t>(width_mask) + 1));
  };

  switch (enc.kind) {
  case isa::FieldKind::Register:
  case isa::FieldKind::Floating: {
    uint32_t limit = isa_.register_count ? isa_.register_count : 32;
    if (limit == 0)
      limit = 32;
    uint32_t value = limit > 0 ? Random::range(limit) : 0;
    if (isa_.defaults.hints.reg_prefers_zero_one_hot && limit > 1) {
      if (Random::chancePct(40)) {
        value = 0;
      } else {
        value = 1 + Random::range(limit - 1);
      }
    } else if ((field_name == "rd" || field_name == "rd_rs1") && limit > 1 && value == 0 &&
               Random::chancePct(80)) {
      value = 1 + Random::range(limit - 1);
    }
    return static_cast<uint32_t>(value & mask);
  }

  case isa::FieldKind::Immediate: {
    if (enc.is_signed && enc.width < 32 && enc.width > 0) {
      int64_t span = 1ll << (enc.width - 1);
      int64_t low = -span;
      int64_t high = span - 1;
      int64_t range = high - low + 1;
      int64_t pick = range > 0
                         ? low + static_cast<int64_t>(Random::range(static_cast<uint32_t>(range)))
                         : 0;
      if (isa_.defaults.hints.signed_immediates_bias) {
        if (Random::chancePct(30)) {
          pick = 0;
        } else if (Random::chancePct(30)) {
          pick = Random::chancePct(50) ? 1 : -1;
        }
      }
      return static_cast<uint32_t>(pick) & static_cast<uint32_t>(mask);
    }
    return uniform_masked(mask);
  }

  case isa::FieldKind::Opcode:
  case isa::FieldKind::Enum:
  case isa::FieldKind::Predicate:
  case isa::FieldKind::Memory:
  default:
    break;
  }

  if (enc.is_signed && enc.width < 32 && enc.width > 0) {
    int64_t span = 1ll << (enc.width - 1);
    int64_t low = -span;
    int64_t high = span - 1;
    int64_t range = high - low + 1;
    int64_t pick = range > 0
                       ? low + static_cast<int64_t>(Random::range(static_cast<uint32_t>(range)))
                       : 0;
    return static_cast<uint32_t>(pick) & static_cast<uint32_t>(mask);
  }

  return uniform_masked(mask);
}

void ISAMutator::applyField(uint32_t &word,
                               const isa::FieldEncoding &enc,
                               uint32_t value) const {
  FunctionTracer tracer(__FILE__, "ISAMutator::applyField");
  if (enc.segments.empty())
    return;

  uint64_t masked = value;
  if (enc.width && enc.width < 32)
    masked &= internal::mask_bits(enc.width);

  uint64_t w = word;
  for (const auto &seg : enc.segments) {
  uint64_t seg_mask = internal::mask_bits(seg.width);
    uint64_t seg_value = (masked >> seg.value_lsb) & seg_mask;
    uint64_t clear_mask = ~(seg_mask << seg.word_lsb) & 0xFFFFFFFFull;
    w &= clear_mask;
    w |= (seg_value & seg_mask) << seg.word_lsb;
  }
  word = static_cast<uint32_t>(w & 0xFFFFFFFFull);
}

void ISAMutator::writeWord(unsigned char *buf,
                              size_t offset,
                              uint32_t word) const {
  FunctionTracer tracer(__FILE__, "ISAMutator::writeWord");
  if (word_bytes_ == 2) {
    internal::store_u16_le(buf, offset, static_cast<uint16_t>(word & 0xFFFFu));
    return;
  }
  if (word_bytes_ == 4) {
    internal::store_u32_le(buf, offset, word);
    return;
  }
  for (uint32_t i = 0; i < word_bytes_ && i < 4; ++i)
    buf[offset + i] = static_cast<unsigned char>((word >> (8 * i)) & 0xFFu);
}

std::string ISAMutator::trim(const std::string &s) {
  FunctionTracer tracer(__FILE__, "ISAMutator::trim");
  size_t begin = 0;
  while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin])))
    ++begin;
  size_t end = s.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])))
    --end;
  return s.substr(begin, end - begin);
}

bool ISAMutator::loadFallbackConfig(const std::string &path) {
  FunctionTracer tracer(__FILE__, "ISAMutator::loadFallbackConfig");
  std::ifstream ifs(path);
  if (!ifs)
    return false;

  rules_.clear();
  std::string line;
  Rule *cur = nullptr;
  while (std::getline(ifs, line)) {
    std::string s = trim(line);
    if (s.empty() || s[0] == '#')
      continue;
    if (s.rfind("- type:", 0) == 0 || s.rfind("type:", 0) == 0) {
      std::string name = s.substr(s.find(':') + 1);
      rules_.push_back(Rule());
      cur = &rules_.back();
      cur->type = trim(name);
      continue;
    }
    if (!cur)
      continue;
    if (s.rfind("weight:", 0) == 0) {
      cur->weight = static_cast<uint32_t>(std::stoul(trim(s.substr(7))));
    } else if (s.rfind("count:", 0) == 0) {
      std::string v = trim(s.substr(6));
      size_t dash = v.find('-');
      if (dash == std::string::npos) {
        cur->min = cur->max = static_cast<uint32_t>(std::stoul(v));
      } else {
        cur->min = static_cast<uint32_t>(std::stoul(trim(v.substr(0, dash))));
        cur->max = static_cast<uint32_t>(std::stoul(trim(v.substr(dash + 1))));
      }
    } else if (s.rfind("min:", 0) == 0) {
      cur->min = static_cast<uint32_t>(std::stoul(trim(s.substr(4))));
    } else if (s.rfind("max:", 0) == 0) {
      cur->max = static_cast<uint32_t>(std::stoul(trim(s.substr(4))));
    } else if (s.rfind("pattern:", 0) == 0) {
      cur->pattern = internal::parse_pattern(trim(s.substr(8)));
    }
  }
  return !rules_.empty();
}

void ISAMutator::applyRule(const Rule &r,
                           unsigned char *buf,
                           size_t &len,
                           size_t cap) {
  FunctionTracer tracer(__FILE__, "ISAMutator::applyRule");
  if (r.type == "byte_flip") {
    uint32_t n = r.min + (r.max > r.min ? Random::range(r.max - r.min + 1) : 0);
    for (uint32_t i = 0; i < n; ++i) {
      if (len == 0)
        break;
      size_t idx = Random::range(static_cast<uint32_t>(len));
      unsigned bit = Random::range(8);
      buf[idx] ^= static_cast<unsigned char>(1u << bit);
    }
    return;
  }

  if (r.type == "insert_pattern") {
    if (r.pattern.empty() || len >= cap)
      return;
    size_t pos = len ? Random::range(static_cast<uint32_t>(len + 1)) : 0;
    size_t patlen = r.pattern.size();
    size_t newlen = internal::clamp_cap(len + patlen, cap);
    std::memmove(buf + pos + patlen, buf + pos, len - pos);
    for (size_t i = 0; i < patlen && pos + i < cap; ++i)
      buf[pos + i] = r.pattern[i];
    len = newlen;
    return;
  }

  if (r.type == "swap_chunks") {
    if (len < 2)
      return;
    size_t a = Random::range(static_cast<uint32_t>(len));
    size_t b = Random::range(static_cast<uint32_t>(len));
    size_t sz = 1 + Random::range(static_cast<uint32_t>(std::max<size_t>(1, len / 8)));
    size_t asz = std::min(sz, len - a);
    size_t bsz = std::min(sz, len - b);
    std::vector<uint8_t> tmp(asz);
    std::memcpy(tmp.data(), buf + a, asz);
    std::memmove(buf + a, buf + b, bsz);
    std::memcpy(buf + b, tmp.data(), asz);
    return;
  }

  if (r.type == "truncate") {
    if (len == 0)
      return;
    uint32_t n = r.min + (r.max > r.min ? Random::range(r.max - r.min + 1) : 0);
    if (n >= len) {
      len = 1;
      return;
    }
    len -= n;
    return;
  }

  if (r.type == "duplicate_chunk") {
    if (len == 0 || len >= cap)
      return;
    size_t pos = Random::range(static_cast<uint32_t>(len));
    size_t sz = 1 + Random::range(static_cast<uint32_t>(std::min<size_t>(len - pos, 4)));
    size_t inspos = Random::range(static_cast<uint32_t>(len + 1));
    size_t newlen = internal::clamp_cap(len + sz, cap);
    std::memmove(buf + inspos + sz, buf + inspos, len - inspos);
    std::memcpy(buf + inspos, buf + pos, std::min(sz, newlen - inspos));
    len = newlen;
    return;
  }
}

} // namespace fuzz::mutator
