#include "RV32Mutator.hpp"
#include "CompressedMutator.hpp"
#include "LegalCheck.hpp"
#include "MutatorDebug.hpp"
#include "RV32Decoder.hpp"
#include "RV32Encoder.hpp"
#include "Random.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

//
// Debug macro: compile with -DMUTDBG_TRACE to print verbose logs to stderr.
// Example: CXXFLAGS="-O0 -g -DMUTDBG_TRACE" make
//
#ifdef MUTDBG_TRACE
  #define DBG(...) std::fprintf(stderr, __VA_ARGS__)
#else
  #define DBG(...) do {} while (0)
#endif

namespace mut {

// ---------- lifecycle ----------

void RV32Mutator::initFromEnv() {
  cfg_.initFromEnv();
  MutatorDebug::init_from_env();
  DBG("[mut] initFromEnv: strategy=%d decode_prob=%u enable_c=%d\n",
      (int)cfg_.strategy, (unsigned)cfg_.decode_prob, (int)cfg_.enable_c);
}

// ---------- helpers ----------

uint32_t RV32Mutator::pickReg() const {
  // Prefer non-x0 for writes by skewing away from 0 occasionally
  uint32_t r = Random::range(32);
  if (r == 0 && Random::chancePct(80)) r = 1 + Random::range(31);
  return r;
}

void RV32Mutator::uToggleOp(uint32_t &v) const {
  // Flip a small subset of opcode/func bits conservatively
  if (Random::chancePct(50))
    v ^= (1u << (7 + (Random::rnd32() % 3)));  // in rd/funct3 area
  else
    v ^= (1u << (25 + (Random::rnd32() % 3))); // in funct7 area
}

void RV32Mutator::uMutateImmSmall(uint32_t &v, int32_t pages_delta) const {
  Fmt f = RV32Decoder::getFormat(v);
  if (f == Fmt::I) {
    int32_t imm = (int32_t)((v >> 20) & 0xFFF);
    if (imm & 0x800) imm |= ~0xFFF;
    imm += (int32_t)(Random::rnd32() % 7) - 3;
    v = (v & ~(0xFFFu << 20)) | (((uint32_t)imm & 0xFFFu) << 20);

  } else if (f == Fmt::S) {
    int32_t imm = ((int32_t)(v >> 25) << 5) | ((v >> 7) & 0x1F);
    if (imm & 0x800) imm |= ~0xFFF;
    imm += (int32_t)(Random::rnd32() % 7) - 3;
    uint32_t u = (uint32_t)imm & 0xFFFu;
    v &= ~(((uint32_t)0x7Fu << 25) | ((uint32_t)0x1Fu << 7));
    v |= ((u >> 5) << 25) | ((u & 0x1F) << 7);

  } else if (f == Fmt::B) {
    int32_t imm = 0;
    imm |= ((v >> 7) & 1) << 11;
    imm |= ((v >> 8) & 0xF) << 1;
    imm |= ((v >> 25) & 0x3F) << 5;
    imm |= ((v >> 31) & 1) << 12;
    if (imm & 0x1000) imm |= ~0x1FFF;
    imm += pages_delta;
    uint32_t u = (uint32_t)imm & 0x1FFFu;
    uint32_t bit12 = (u >> 12) & 1, bit11 = (u >> 11) & 1;
    uint32_t bits10_5 = (u >> 5) & 0x3F, bits4_1 = (u >> 1) & 0xF;
    v &= ~(((uint32_t)1 << 31) | ((uint32_t)1 << 7) | ((uint32_t)0x3F << 25) |
           ((uint32_t)0xF << 8));
    v |= (bit12 << 31) | (bit11 << 7) | (bits10_5 << 25) | (bits4_1 << 8);

  } else if (f == Fmt::J) {
    int32_t imm = 0;
    imm |= ((v >> 21) & 0x3FF) << 1;
    imm |= ((v >> 20) & 1) << 11;
    imm |= ((v >> 12) & 0xFF) << 12;
    imm |= ((v >> 31) & 1) << 20;
    if (imm & 0x100000) imm |= ~0x1FFFFF;
    imm += (pages_delta << 1);
    uint32_t u = (uint32_t)imm & 0x1FFFFFu;
    uint32_t bit20 = (u >> 20) & 1, bit11 = (u >> 11) & 1;
    uint32_t bits10_1 = (u >> 1) & 0x3FF, bits19_12 = (u >> 12) & 0xFF;
    v &= ~(((uint32_t)1 << 31) | ((uint32_t)0x3FF << 21) | ((uint32_t)1 << 20) |
           ((uint32_t)0xFF << 12));
    v |= (bit20 << 31) | (bits19_12 << 12) | (bit11 << 20) | (bits10_1 << 21);
  }
}

void RV32Mutator::mutateRegs32(uint32_t &v) const {
  Fmt f = RV32Decoder::getFormat(v);
  if (f == Fmt::UNKNOWN || f == Fmt::C16) return;

  // Decode, tweak, re-encode to stay sane
  IR32 ir = RV32Decoder::decode(v);
  if (f == Fmt::R || f == Fmt::I || f == Fmt::S || f == Fmt::B || f == Fmt::U || f == Fmt::J) {
    switch (Random::rnd32() % 3) {
      case 0: ir.rd  = (uint8_t)pickReg(); break;
      case 1: ir.rs1 = (uint8_t)pickReg(); break;
      case 2: ir.rs2 = (uint8_t)pickReg(); break;
    }
    v = RV32Encoder::encode(ir);
  }
}

void RV32Mutator::mutateImm32(uint32_t &v) const {
  uint32_t before = v;
  if (Random::chancePct(cfg_.imm_random_prob)) {
    Fmt f = RV32Decoder::getFormat(v);
    if (f == Fmt::I) {
      v = (v & ~(0xFFFu << 20)) | ((Random::rnd32() & 0xFFFu) << 20);
    } else if (f == Fmt::S) {
      uint32_t newimm = Random::rnd32() & 0xFFFu;
      uint32_t hi = (newimm >> 5) & 0x7Fu, lo = newimm & 0x1Fu;
      v &= ~(((uint32_t)0x7Fu << 25) | ((uint32_t)0x1Fu << 7));
      v |= (hi << 25) | (lo << 7);
    } else {
      uMutateImmSmall(v, (int32_t)((Random::rnd32() % 7) - 3));
    }
  } else {
    uMutateImmSmall(v, (int32_t)((Random::rnd32() % 7) - 3));
  }

  if (!is_legal_instruction(v)) {
    MUTDBG_ILLEGAL(before, v, "mutateImm32");
  }
}

void RV32Mutator::replaceWithSameFmt32(uint32_t &v) const {
  uint32_t before = v;
  Fmt f = RV32Decoder::getFormat(v);
  uint32_t op = v & 0x7f;

  if (f == Fmt::R && op == 0x33u) {
    static const uint8_t R_BASE[][2] = {
        {0x0, 0x00}, {0x0, 0x20}, {0x1, 0x00}, {0x2, 0x00}, {0x3, 0x00},
        {0x4, 0x00}, {0x5, 0x00}, {0x5, 0x20}, {0x6, 0x00}, {0x7, 0x00}};
    static const uint8_t R_M[][2] = {
        {0x0, 0x01}, {0x1, 0x01}, {0x2, 0x01}, {0x3, 0x01},
        {0x4, 0x01}, {0x5, 0x01}, {0x6, 0x01}, {0x7, 0x01}};

    uint32_t w_base = clamp_pct((int)cfg_.r_weight_base_alu);
    uint32_t w_m    = clamp_pct((int)cfg_.r_weight_m);
    uint32_t total  = w_base + w_m;
    if (!total) { w_base = 100; total = 100; }
    uint32_t pick = Random::rnd32() % total;

    const uint8_t (*tbl)[2];
    size_t n;
    if (pick < w_base) { tbl = R_BASE; n = sizeof(R_BASE) / sizeof(R_BASE[0]); }
    else               { tbl = R_M;    n = sizeof(R_M)    / sizeof(R_M[0]); }

    const uint8_t *sel = tbl[Random::rnd32() % n];
    uint32_t f3 = sel[0], f7 = sel[1];

    v &= ~(((uint32_t)0x7u << 12) | ((uint32_t)0x7Fu << 25));
    v |= (f3 << 12) | (f7 << 25);
  } else {
    // As a fallback, toggle a small opcode neighborhood
    uToggleOp(v);
  }

  if (!is_legal_instruction(v)) {
    MUTDBG_ILLEGAL(before, v, "replaceWithSameFmt32");
  }
}

void RV32Mutator::mutateRaw32(uint32_t &v) const {
  switch (Random::rnd32() % 4) {
    case 0: uToggleOp(v); break;
    case 1: mutateRegs32(v); break;
    case 2: mutateImm32(v); break;
    case 3: uMutateImmSmall(v, (int32_t)((Random::rnd32() % 5) - 2)); break;
  }
}

void RV32Mutator::mutateIR32(uint32_t &v) const {
  if (Random::chancePct(50)) mutateRegs32(v);
  else                       mutateImm32(v);
}

bool RV32Mutator::shouldDecodeIR() const {
  switch (cfg_.strategy) {
    case Strategy::RAW:    return false;
    case Strategy::IR:     return true;
    case Strategy::HYBRID: return Random::chancePct(cfg_.decode_prob);
    case Strategy::AUTO:
    default:               return Random::chancePct(cfg_.decode_prob); // stub
  }
}

// ---------- main API ----------

unsigned char *RV32Mutator::mutateStream(unsigned char *in, size_t in_len,
                                         unsigned char * /*out_buf*/,
                                         size_t max_size) {
  last_len_ = 0;

  DBG("[mut] mutateStream in_len=%zu max_size=%zu\n", in_len, max_size);

  // Empty input → minimal valid buffer
  if (in_len == 0) {
    size_t cap = max_size ? max_size : 1;
    unsigned char *tmp = (unsigned char *)std::malloc(cap);
    if (!tmp) return nullptr;
    if (cap > 0) tmp[0] = 0;
    last_len_ = std::min((size_t)1, cap);
    DBG("[mut] empty input -> len=%zu\n", last_len_);
    return tmp;
  }

  // Capacity we can safely write to. We always allocate our own buffer
  // to avoid aliasing with AFL's memory.
  size_t cap = max_size ? max_size : in_len;
  if (cap == 0) cap = in_len;

  unsigned char *out = (unsigned char *)std::malloc(cap);
  if (!out) return nullptr;

  const size_t copy_len = in ? std::min(in_len, cap) : 0;
  if (copy_len) std::memmove(out, in, copy_len);
  size_t cur_len = copy_len;

  DBG("[mut] cap=%zu copy_len=%zu\n", cap, copy_len);

  // Tiny (compressed-only) payload
  size_t nwords = cur_len / 4;
  if (nwords == 0 && cur_len >= 2 && cfg_.enable_c) {
    DBG("[mut] compressed-only path: cur_len=%zu\n", cur_len);
    CompressedMutator::mutateAt(out, 0, cur_len, cfg_);
    last_len_ = cur_len ? cur_len : 1;
    return out;
  }

  // Main bounded mutation loop
  unsigned nmuts = 1 + (Random::rnd32() % 3u);
  DBG("[mut] nmuts=%u nwords=%zu cur_len=%zu\n", nmuts, nwords, cur_len);

  for (unsigned mi = 0; mi < nmuts; ++mi) {
    if (nwords == 0) {
      // Insert 32-bit NOP if we have space
      if (cap >= 4) {
        const uint32_t nop = 0x00000013u;
        put_u32_le(out, 0, nop);
        cur_len = std::max(cur_len, (size_t)4);
        nwords  = cur_len / 4;
        DBG("[mut] inserted NOP, cur_len=%zu nwords=%zu\n", cur_len, nwords);
      }
      last_len_ = cur_len ? cur_len : 1;
      return out;
    }

    // Safe index [0..nwords-1]
    size_t wi = (size_t)(Random::rnd32() % nwords);
    size_t byte_i = wi * 4; // byte_i+3 < 4*nwords <= cur_len
    uint32_t insn = get_u32_le(out, byte_i);
    Fmt f = RV32Decoder::getFormat(insn);

    bool use_ir = shouldDecodeIR();
    if (f == Fmt::B || f == Fmt::J || f == Fmt::C16 || f == Fmt::C_CR ||
        f == Fmt::C_CI || f == Fmt::C_CSS || f == Fmt::C_CIW ||
        f == Fmt::C_CL || f == Fmt::C_CS || f == Fmt::C_CB || f == Fmt::C_CJ) {
      use_ir = true;
    }

    uint32_t kind = Random::rnd32() % 8u;
    DBG("[mut] mi=%u wi=%zu kind=%u use_ir=%d\n", mi, wi, kind, (int)use_ir);

    switch (kind) {
      case 0: mutateRegs32(insn); break;
      case 1: mutateImm32(insn); break;
      case 2: replaceWithSameFmt32(insn); break;
      case 3: insn = 0x00000013u; break; // NOP
      case 4: insn = 0x00000013u; break; // NOP (delete-as-NOP)
      case 5:
        if (cfg_.enable_c && byte_i + 1 < cur_len) {
          CompressedMutator::mutateAt(out, byte_i, cur_len, cfg_);
        }
        break;
      case 6: uMutateImmSmall(insn, (int32_t)((Random::rnd32() % 3) - 1)); break;
      case 7: if (use_ir) mutateIR32(insn); else mutateRaw32(insn); break;
      default: break;
    }

    if (!is_legal_instruction(insn)) {
      MUTDBG_ILLEGAL(get_u32_le(out, byte_i), insn, "mutateStream");
    }

    // Write back safely within current length (NOT in_len!)
    if (byte_i + 3 < cur_len) {
      put_u32_le(out, byte_i, insn);
    }
  }

  last_len_ = cur_len ? cur_len : 1;
  DBG("[mut] done cur_len=%zu\n", last_len_);
  return out;
}

} // namespace mut
// vim: set ts=2 sw=2 sts=2 et ai: