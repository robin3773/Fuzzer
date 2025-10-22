#include "RV32Mutator.hpp"
#include "CompressedMutator.hpp"
#include "LegalCheck.hpp"
#include "MutatorDebug.hpp"
#include "RV32Decoder.hpp"
#include "RV32Encoder.hpp"
#include "Random.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace mut {

void RV32Mutator::initFromEnv() {
  cfg_.initFromEnv();
  MutatorDebug::init_from_env();
}

uint32_t RV32Mutator::pickReg() const {
  // Prefer non-x0 for writes by skewing away from 0 occasionally
  uint32_t r = Random::range(32);
  if (r == 0 && Random::chancePct(80))
    r = 1 + Random::range(31);
  return r;
}

void RV32Mutator::uToggleOp(uint32_t &v) const {
  // Flip a small subset of opcode/func bits conservatively
  if (Random::chancePct(50))
    v ^= (1u << (7 + (Random::rnd32() % 3))); // in rd/funct3 area
  else
    v ^= (1u << (25 + (Random::rnd32() % 3))); // in funct7 area
}

void RV32Mutator::uMutateImmSmall(uint32_t &v, int32_t pages_delta) const {
  Fmt f = RV32Decoder::getFormat(v);
  if (f == Fmt::I) {
    int32_t imm = (int32_t)((v >> 20) & 0xFFF);
    if (imm & 0x800)
      imm |= ~0xFFF;
    imm += (int32_t)(Random::rnd32() % 7) - 3;
    v = (v & ~(0xFFFu << 20)) | (((uint32_t)imm & 0xFFFu) << 20);
  } else if (f == Fmt::S) {
    int32_t imm = ((int32_t)(v >> 25) << 5) | ((v >> 7) & 0x1F);
    if (imm & 0x800)
      imm |= ~0xFFF;
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
    if (imm & 0x1000)
      imm |= ~0x1FFF;
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
    if (imm & 0x100000)
      imm |= ~0x1FFFFF;
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
  if (f == Fmt::UNKNOWN || f == Fmt::C16)
    return;
  // Decode, tweak, re-encode to stay sane
  IR32 ir = RV32Decoder::decode(v);
  if (f == Fmt::R || f == Fmt::I || f == Fmt::S || f == Fmt::B || f == Fmt::U ||
      f == Fmt::J) {
    // Randomly tweak one of rd/rs1/rs2 if meaningful
    switch (Random::rnd32() % 3) {
    case 0:
      ir.rd = (uint8_t)pickReg();
      break;
    case 1:
      ir.rs1 = (uint8_t)pickReg();
      break;
    case 2:
      ir.rs2 = (uint8_t)pickReg();
      break;
    }
    v = RV32Encoder::encode(ir);
  }
}

void RV32Mutator::mutateImm32(uint32_t &v) const {
  uint32_t before = v;
  if (Random::chancePct(cfg_.imm_random_prob)) {
    // random immediate chunk based on format
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
    static const uint8_t R_M[][2] = {{0x0, 0x01}, {0x1, 0x01}, {0x2, 0x01},
                                     {0x3, 0x01}, {0x4, 0x01}, {0x5, 0x01},
                                     {0x6, 0x01}, {0x7, 0x01}};
    uint32_t w_base = clamp_pct((int)cfg_.r_weight_base_alu);
    uint32_t w_m = clamp_pct((int)cfg_.r_weight_m);
    uint32_t total = w_base + w_m;
    if (!total) {
      w_base = 100;
      total = 100;
    }
    uint32_t pick = Random::rnd32() % total;

    const uint8_t(*tbl)[2];
    size_t n;
    if (pick < w_base) {
      tbl = R_BASE;
      n = sizeof(R_BASE) / sizeof(R_BASE[0]);
    } else {
      tbl = R_M;
      n = sizeof(R_M) / sizeof(R_M[0]);
    }
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
  // Flip a small number of random bits or small fields
  switch (Random::rnd32() % 4) {
  case 0:
    uToggleOp(v);
    break;
  case 1:
    mutateRegs32(v);
    break;
  case 2:
    mutateImm32(v);
    break;
  case 3:
    uMutateImmSmall(v, (int32_t)((Random::rnd32() % 5) - 2));
    break;
  }
}

void RV32Mutator::mutateIR32(uint32_t &v) const {
  // Decode → tweak → encode
  if (Random::chancePct(50))
    mutateRegs32(v);
  else
    mutateImm32(v);
}

bool RV32Mutator::shouldDecodeIR() const {
  switch (cfg_.strategy) {
  case Strategy::RAW:
    return false;
  case Strategy::IR:
    return true;
  case Strategy::HYBRID:
    return Random::chancePct(cfg_.decode_prob);
  case Strategy::AUTO:
  default:
    return Random::chancePct(cfg_.decode_prob); // stub
  }
}

unsigned char *RV32Mutator::mutateStream(unsigned char *in, size_t in_len,
                                         unsigned char *out_buf,
                                         size_t max_size) {
  // Prepare output buffer
  unsigned char *out = out_buf;
  size_t buf_size = in_len;
  if (!out_buf || max_size < in_len) {
    out = (unsigned char *)std::malloc(in_len);
    if (!out)
      return nullptr;
    max_size = in_len;
  }
  std::memcpy(out, in, in_len);

  size_t nwords = in_len / 4;
  if (nwords == 0 && in_len >= 2 && cfg_.enable_c) {
    // If only compressed, try mutate first halfword
    CompressedMutator::mutateAt(out, 0, in_len, cfg_);
    return out;
  }

  unsigned nmuts = 1 + (Random::rnd32() % 3u);
  for (unsigned mi = 0; mi < nmuts; ++mi) {
    if (nwords == 0) {
      // Create NOP if empty
      if (max_size >= 4) {
        const uint32_t nop = 0x00000013u;
        put_u32_le(out, 0, nop);
      }
      return out;
    }

    size_t wi = Random::range((uint32_t)nwords);
    size_t byte_i = wi * 4;
    uint32_t insn = get_u32_le(out, byte_i);
    Fmt f = RV32Decoder::getFormat(insn);

    bool use_ir = shouldDecodeIR();
    if (f == Fmt::B || f == Fmt::J || f == Fmt::C16 || f == Fmt::C_CR ||
        f == Fmt::C_CI || f == Fmt::C_CSS || f == Fmt::C_CIW ||
        f == Fmt::C_CL || f == Fmt::C_CS || f == Fmt::C_CB || f == Fmt::C_CJ) {
      use_ir = true;
    }

    uint32_t kind = Random::rnd32() % 8u;
    switch (kind) {
    case 0:
      mutateRegs32(insn);
      break;
    case 1:
      mutateImm32(insn);
      break;
    case 2:
      replaceWithSameFmt32(insn);
      break;
    case 3: { // insert NOP nearby (turn current into NOP)
      insn = 0x00000013u;
      break;
    }
    case 4: { // delete to NOP (same as 3; kept for strategy variety)
      insn = 0x00000013u;
      break;
    }
    case 5: { // mutate compressed neighbor if present
      if (cfg_.enable_c && byte_i + 1 < in_len) {
        CompressedMutator::mutateAt(out, byte_i, in_len, cfg_);
      }
      break;
    }
    case 6: { // small U-type immediate nudge (AUIPC/LUI stay coarse)
      uMutateImmSmall(insn, (int32_t)((Random::rnd32() % 3) - 1));
      break;
    }
    case 7: { // raw or IR path
      if (use_ir)
        mutateIR32(insn);
      else
        mutateRaw32(insn);
      break;
    }
    default:
      break;
    }

    if (!is_legal_instruction(insn)) {
      MUTDBG_ILLEGAL(get_u32_le(out, byte_i), insn, "mutateStream");
    }
    if (byte_i + 3 < in_len)
      put_u32_le(out, byte_i, insn);
  }

  return out;
}

} // namespace mut
