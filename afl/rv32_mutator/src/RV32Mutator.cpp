
#include "RV32Mutator.hpp"
#include "Random.hpp"
#include "Instruction.hpp"
#include "RV32Decoder.hpp"
#include "RV32Encoder.hpp"
#include "CompressedMutator.hpp"
#include "LegalCheck.hpp"
#include "MutatorDebug.hpp"
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace mut {

void RV32Mutator::initFromEnv() {
  cfg_.loadFromEnv();
  if (cfg_.verbose) {
    std::fprintf(stderr, "[mutator] strategy=%d decode_prob=%u rv32e=%d c=%d\n",
      (int)cfg_.strategy, cfg_.decode_prob, cfg_.rv32e_mode, cfg_.enable_c);
  }
}

// --- helpers ---
uint32_t RV32Mutator::pickReg() const {
  // rarely choose x0
  if ((Random::rnd32() & 127u) == 0u) return 0u;
  uint32_t limit = cfg_.rv32e_mode ? 16u : 32u;
  uint32_t r = 1u + (Random::rnd32() % (limit - 1u));
  return r & 0x1Fu;
}

void RV32Mutator::uToggleOp(uint32_t& v) const {
  uint32_t op = v & 0x7f;
  uint32_t newop = (op == 0x37u) ? 0x17u : 0x37u; // LUI <-> AUIPC
  v = (v & ~0x7Fu) | newop;
}

void RV32Mutator::uMutateImmSmall(uint32_t& v, int32_t pages_delta) const {
  int32_t imm20 = (int32_t)((v >> 12) & 0xFFFFF);
  if (imm20 & 0x80000) imm20 |= ~0xFFFFF;
  imm20 += pages_delta;
  uint32_t newimm = (uint32_t)imm20 & 0xFFFFF;
  v = (v & 0x00000FFFu) | (newimm << 12);
}

bool RV32Mutator::shouldDecodeIR() const {
  switch (cfg_.strategy) {
    case Strategy::RAW: return false;
    case Strategy::IR:  return true;
    case Strategy::HYBRID:
      return Random::chancePct(cfg_.decode_prob);
    case Strategy::AUTO:
      // Stub: For now behave like HYBRID
      return Random::chancePct(cfg_.decode_prob);
  }
  return false;
}

// --- raw path mutations (fast) ---
void RV32Mutator::mutateRegs32(uint32_t& v) const {
  uint32_t before = v;
  Fmt f = RV32Decoder::getFormat(v);
  uint32_t choice = Random::rnd32() & 7u; // rd/rs1/rs2 bits
  bool has_rd  = (f == Fmt::R || f == Fmt::I || f == Fmt::U || f == Fmt::J || f == Fmt::R4 || f == Fmt::A);
  bool has_rs1 = (f == Fmt::R || f == Fmt::I || f == Fmt::S || f == Fmt::B || f == Fmt::A || f == Fmt::R4);
  bool has_rs2 = (f == Fmt::R || f == Fmt::S || f == Fmt::B || f == Fmt::A || f == Fmt::R4);

  uint32_t mask = (has_rd?1u:0u) | (has_rs1?2u:0u) | (has_rs2?4u:0u);
  if ((choice & mask) == 0u) {
    if (has_rd)      choice = 1u;
    else if (has_rs1) choice = 2u;
    else if (has_rs2) choice = 4u;
    else choice = 1u;
  }

  if (choice & 1u) { // rd
    uint32_t rd = pickReg();
    v = (v & ~(0x1Fu << 7)) | (rd << 7);
  }
  if ((choice & 2u) && has_rs1) {
    uint32_t rs1 = pickReg();
    v = (v & ~(0x1Fu << 15)) | (rs1 << 15);
  }
  if ((choice & 4u) && has_rs2) {
    uint32_t rs2 = pickReg();
    v = (v & ~(0x1Fu << 20)) | (rs2 << 20);
  }

  if (cfg_.rv32e_mode) {
    uint32_t rdv  = (v >> 7) & 0x1F;  rdv  &= 0x0F;
    uint32_t rs1v = (v >> 15) & 0x1F; rs1v &= 0x0F;
    uint32_t rs2v = (v >> 20) & 0x1F; rs2v &= 0x0F;
    v &= ~(((0x1Fu)<<7) | ((0x1Fu)<<15) | ((0x1Fu)<<20));
    v |= (rdv << 7) | (rs1v << 15) | (rs2v << 20);
  }

    // Debug-only: if illegal, log
  if (!is_legal_instruction(v)) {
    MUTDBG_ILLEGAL(before, v, "mutate_regs32");
  }
}

void RV32Mutator::mutateImm32(uint32_t& v) const {
  auto rand_delta = [&]() -> int32_t {
    int32_t M = cfg_.imm_delta_max;
    if (M <= 0) return 1;
    int32_t d = (int32_t)(Random::rnd32() % (2u * (uint32_t)M + 1u)) - M;
    if (d == 0) d = (Random::rnd32() & 1u) ? 1 : -1;
    return d;
  };
  auto mutate_delta_field = [&](int bits, int shift, uint32_t mask) {
    int32_t delta = rand_delta();
    int32_t imm = (int32_t)((v >> shift) & mask);
    if (bits > 0 && (imm & (1 << (bits - 1)))) imm |= ~((1 << bits) - 1);
    imm += delta;
    uint32_t newimm = (uint32_t)(imm & ((bits >= 32) ? 0xFFFFFFFFu : ((1u << bits) - 1)));
    v = (v & ~(mask << shift)) | (newimm << shift);
  };
  uint32_t before = v;
  Fmt f = RV32Decoder::getFormat(v);
  bool use_random = Random::chancePct(cfg_.imm_random_prob);

  if (f == Fmt::I) {
    if (use_random) {
      uint32_t newimm = Random::rnd32() & 0xFFFu;
      v = (v & ~(0xFFFu << 20)) | (newimm << 20);
    } else {
      mutate_delta_field(12, 20, 0xFFFu);
    }
    return;
  }
  if (f == Fmt::S) {
    if (use_random) {
      uint32_t newimm = Random::rnd32() & 0xFFFu;
      uint32_t hi = (newimm >> 5) & 0x7Fu, lo = newimm & 0x1Fu;
      v &= ~(((uint32_t)0x7Fu << 25) | ((uint32_t)0x1Fu << 7));
      v |= (hi << 25) | (lo << 7);
    } else {
      int32_t imm = ((v >> 25) << 5) | ((v >> 7) & 0x1F);
      if (imm & 0x800) imm |= ~0xFFF;
      imm += rand_delta();
      uint32_t newimm = (uint32_t)imm & 0xFFFu;
      uint32_t hi = (newimm >> 5) & 0x7Fu, lo = newimm & 0x1Fu;
      v &= ~(((uint32_t)0x7Fu << 25) | ((uint32_t)0x1Fu << 7));
      v |= (hi << 25) | (lo << 7);
    }
    return;
  }
  if (f == Fmt::B) {
    if (use_random) {
      uint32_t newimm = Random::rnd32() & 0x1FFFu; newimm &= ~1u;
      uint32_t b31 = (newimm >> 12) & 1, b30_25 = (newimm >> 5) & 0x3F;
      uint32_t b11_8 = (newimm >> 1) & 0xF, b7 = (newimm >> 11) & 1;
      v &= ~(((uint32_t)1 << 31) | ((uint32_t)0x3F << 25) | ((uint32_t)0xF << 8) | ((uint32_t)1 << 7));
      v |= (b31 << 31) | (b30_25 << 25) | (b11_8 << 8) | (b7 << 7);
    } else {
      uint32_t imm = ((v >> 31) & 1) << 12 |
                     ((v >> 25) & 0x3F) << 5 |
                     ((v >> 8)  & 0xF)  << 1 |
                     ((v >> 7)  & 1)   << 11;
      if (imm & 0x1000) imm |= 0xFFFFE000;
      int32_t d = rand_delta();
      imm = (uint32_t)((int32_t)imm + (d << 1));
      uint32_t newimm = imm & 0x1FFFu;
      uint32_t b31 = (newimm >> 12) & 1, b30_25 = (newimm >> 5) & 0x3F;
      uint32_t b11_8 = (newimm >> 1) & 0xF, b7 = (newimm >> 11) & 1;
      v &= ~(((uint32_t)1 << 31) | ((uint32_t)0x3F << 25) | ((uint32_t)0xF << 8) | ((uint32_t)1 << 7));
      v |= (b31 << 31) | (b30_25 << 25) | (b11_8 << 8) | (b7 << 7);
    }

    return;
  }
  if (f == Fmt::U) {
    if (use_random) {
      uint32_t newimm = Random::rnd32() & 0xFFFFFu;
      v = (v & 0x00000FFFu) | (newimm << 12);
    } else {
      int32_t imm20 = (int32_t)((v >> 12) & 0xFFFFF);
      if (imm20 & 0x80000) imm20 |= ~0xFFFFF;
      imm20 += rand_delta();
      uint32_t newimm = (uint32_t)imm20 & 0xFFFFFu;
      v = (v & 0x00000FFFu) | (newimm << 12);
    }
    return;
  }
  if (f == Fmt::J) {
    if (use_random) {
      uint32_t newimm = Random::rnd32() & 0x1FFFFFu; newimm &= ~1u;
      uint32_t bit20 = (newimm >> 20) & 1, bit11 = (newimm >> 11) & 1;
      uint32_t bits10_1 = (newimm >> 1) & 0x3FF, bits19_12 = (newimm >> 12) & 0xFF;
      v &= ~(((uint32_t)1 << 31) | ((uint32_t)0x3FF << 21) | ((uint32_t)1 << 20) | ((uint32_t)0xFF << 12));
      v |= (bit20 << 31) | (bits19_12 << 12) | (bit11 << 20) | (bits10_1 << 21);
    } else {
      uint32_t imm = ((v >> 31) & 1) << 20 |
                     ((v >> 21) & 0x3FF) << 1 |
                     ((v >> 20) & 1) << 11 |
                     ((v >> 12) & 0xFF) << 12;
      if (imm & 0x100000) imm |= 0xFFE00000;
      int32_t d = (int32_t)cfg_.imm_delta_max;
      imm = (uint32_t)((int32_t)imm + (((Random::rnd32()% (2u*d+1u))-d) << 1));
      uint32_t newimm = imm & 0x1FFFFFu;
      uint32_t bit20 = (newimm >> 20) & 1, bit11 = (newimm >> 11) & 1;
      uint32_t bits10_1 = (newimm >> 1) & 0x3FF, bits19_12 = (newimm >> 12) & 0xFF;
      v &= ~(((uint32_t)1 << 31) | ((uint32_t)0x3FF << 21) | ((uint32_t)1 << 20) | ((uint32_t)0xFF << 12));
      v |= (bit20 << 31) | (bits19_12 << 12) | (bit11 << 20) | (bits10_1 << 21);
    }
    return;
  }

  // Unknown/other small perturbation
  if (use_random) v ^= (1u << (Random::rnd32() & 31u));
  else            v ^= (1u << (Random::rnd32() & 7u));

  if (!is_legal_instruction(v)) {
    MUTDBG_ILLEGAL(before, v, "mutate_imm32");
  }
}

void RV32Mutator::replaceWithSameFmt32(uint32_t& v) const {
  uint32_t before = v;
  Fmt f  = RV32Decoder::getFormat(v);
  uint32_t op = v & 0x7f;

  if (f == Fmt::R && op == 0x33u) {
    static const uint8_t R_BASE[][2] = {
      {0x0,0x00},{0x0,0x20},{0x1,0x00},{0x2,0x00},{0x3,0x00},
      {0x4,0x00},{0x5,0x00},{0x5,0x20},{0x6,0x00},{0x7,0x00}
    };
    static const uint8_t R_M[][2] = {
      {0x0,0x01},{0x1,0x01},{0x2,0x01},{0x3,0x01},{0x4,0x01},{0x5,0x01},{0x6,0x01},{0x7,0x01}
    };
    uint32_t w_base = clamp_pct((int)cfg_.r_weight_base_alu);
    uint32_t w_m    = clamp_pct((int)cfg_.r_weight_m);
    uint32_t total  = w_base + w_m; if (!total) { w_base = 100; total = 100; }
    uint32_t pick   = Random::rnd32() % total;

    const uint8_t (*tbl)[2]; size_t n;
    if (pick < w_base) { tbl = R_BASE; n = sizeof(R_BASE)/sizeof(R_BASE[0]); }
    else               { tbl = R_M;    n = sizeof(R_M)/sizeof(R_M[0]); }
    const uint8_t *sel = tbl[Random::rnd32() % n];
    uint32_t f3 = sel[0], f7 = sel[1];
    v = (v & ~(((uint32_t)0x7u << 12) | ((uint32_t)0x7Fu << 25))) | (f3 << 12) | (f7 << 25);
    return;
  }

  if (f == Fmt::R) {
    static const uint8_t FBACK[][2] = {
      {0x0,0x00},{0x0,0x20},{0x4,0x00},{0x6,0x00},{0x7,0x00}
    };
    const uint8_t *sel = FBACK[Random::rnd32() % (sizeof(FBACK)/sizeof(FBACK[0]))];
    uint32_t f3 = sel[0], f7 = sel[1];
    v = (v & ~(((uint32_t)0x7u << 12) | ((uint32_t)0x7Fu << 25))) | (f3 << 12) | (f7 << 25);
    return;
  }

  if (f == Fmt::I && op == 0x13u) {
    static const uint8_t I_ALL[] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7};
    uint32_t w_shift = clamp_pct((int)cfg_.i_shift_weight);
    bool choose_shift = Random::chancePct(w_shift);
    uint32_t f3 = choose_shift ? ((Random::rnd32() & 1u) ? 0x1u : 0x5u)
                               : I_ALL[Random::rnd32() % (sizeof(I_ALL)/sizeof(I_ALL[0]))];
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    if (f3 == 0x1u) {
      v &= ~((uint32_t)1u << 30); // SLLI
    } else if (f3 == 0x5u) {
      if (Random::rnd32() & 1u) v |=  ((uint32_t)1u << 30); // SRAI
      else                       v &= ~((uint32_t)1u << 30); // SRLI
    }
    return;
  }

  if (f == Fmt::I) {
    uint32_t f3 = Random::rnd32() & 0x7u;
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    return;
  }

  if (f == Fmt::S && op == 0x23u) {
    static const uint8_t S_VALID[] = {0x0,0x1,0x2}; // SB, SH, SW
    uint32_t f3 = S_VALID[Random::rnd32() % (sizeof(S_VALID)/sizeof(S_VALID[0]))];
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    return;
  }
  if (f == Fmt::S) {
    uint32_t f3 = Random::rnd32() & 0x7u;
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    return;
  }

  if (f == Fmt::B && op == 0x63u) {
    static const uint8_t B_VALID[] = {0x0,0x1,0x4,0x5,0x6,0x7};
    uint32_t f3 = B_VALID[Random::rnd32() % (sizeof(B_VALID)/sizeof(B_VALID[0]))];
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    return;
  }
  if (f == Fmt::B) {
    uint32_t f3 = Random::rnd32() & 0x7u;
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    return;
  }

  if (f == Fmt::U) {
    if ((Random::rnd32() & 3u) == 0u) uToggleOp(v);
    return;
  }

  if (!is_legal_instruction(v)) {
    MUTDBG_ILLEGAL(before, v, "replace_with_same_fmt32");
  }
}

// --- public entry ---
unsigned char* RV32Mutator::mutateStream(unsigned char* in, size_t in_len, unsigned char* out_buf, size_t max_size) {
  // Ensure a 4-byte multiple buffer for word-wise ops
  size_t nwords = (in_len + 3) / 4;
  size_t buf_size = nwords * 4;
  unsigned char* out = nullptr;

  bool use_external = (out_buf && buf_size <= max_size);
  if (use_external) {
    out = out_buf;
  } else {
    out = (unsigned char*)std::malloc(buf_size + 4);
    if (!out) return nullptr;
  }
  std::memset(out, 0, buf_size + 4);
  std::memcpy(out, in, in_len);

  unsigned nmuts = 1 + (Random::rnd32() % 3u);

  for (unsigned mi = 0; mi < nmuts; ++mi) {
    if (nwords == 0) {
      // create NOP if empty
      uint32_t nop = 0x00000013u;
      put_u32_le(out, 0, nop);
      return out;
    }

    size_t wi = Random::range((uint32_t)nwords);
    size_t byte_i = wi * 4;
    uint32_t insn = get_u32_le(out, byte_i);
    Fmt f = RV32Decoder::getFormat(insn);

    // HYBRID decision: per-instruction
    bool use_ir = shouldDecodeIR();
    // Force IR for split/complex or compressed
    if (f == Fmt::B || f == Fmt::J || f == Fmt::C16 || f == Fmt::C_CR || f == Fmt::C_CI ||
        f == Fmt::C_CSS || f == Fmt::C_CIW || f == Fmt::C_CL || f == Fmt::C_CS ||
        f == Fmt::C_CB || f == Fmt::C_CJ) {
      use_ir = true;
    }

    uint32_t kind = Random::rnd32() % 8u;
    switch (kind) {
      case 0: // mutate regs
        if (use_ir && (f != Fmt::UNKNOWN) && (f != Fmt::C16)) {
          IR32 ir = RV32Decoder::decode(insn);
          // simple IR reg tweaks
          if (ir.fmt == Fmt::R || ir.fmt == Fmt::I || ir.fmt == Fmt::U || ir.fmt == Fmt::J || ir.fmt == Fmt::A || ir.fmt == Fmt::R4) {
            if (Random::rnd32() & 1u) ir.rd  = (uint8_t)pickReg();
            if (Random::rnd32() & 1u) ir.rs1 = (uint8_t)pickReg();
            if (Random::rnd32() & 1u) ir.rs2 = (uint8_t)pickReg();
            if (cfg_.rv32e_mode) { ir.rd &= 0x0F; ir.rs1 &= 0x0F; ir.rs2 &= 0x0F; }
            insn = RV32Encoder::encode(ir);
          } else {
            mutateRegs32(insn);
          }
        } else {
          mutateRegs32(insn);
        }
        break;

      case 1: // mutate imm
        if (use_ir && (f != Fmt::UNKNOWN)) {
          IR32 ir = RV32Decoder::decode(insn);
          // simple immediate delta
          int32_t M = cfg_.imm_delta_max;
          if (M <= 0) M = 1;
          int32_t d = (int32_t)(Random::rnd32() % (2u*(uint32_t)M+1u)) - M;
          if (d == 0) d = 1;
          if (ir.fmt == Fmt::B || ir.fmt == Fmt::J) d <<= 1; // keep alignment
          ir.imm += d;
          insn = RV32Encoder::encode(ir);
        } else {
          mutateImm32(insn);
        }
        break;

      case 2: // replace funct fields within same format
        replaceWithSameFmt32(insn);
        break;

      case 3: // insert 32-bit NOP before
        {
          unsigned char* tmp = (unsigned char*)std::malloc(buf_size + 8);
          if (!tmp) break;
          std::memcpy(tmp, out, byte_i);
          put_u32_le(tmp, byte_i, 0x00000013u);
          std::memcpy(tmp + byte_i + 4, out + byte_i, buf_size - byte_i);
          if (!use_external) std::free(out);
          out = tmp;
          buf_size += 4;
          nwords += 1;
          insn = get_u32_le(out, byte_i + 4);
        }
        break;

      case 4: // delete -> replace with NOP
        insn = 0x00000013u;
        break;

      case 5: // compressed try
        if (((insn & 0xFFFFu) & 0x3u) != 0x3u) {
          CompressedMutator::mutateAt(out, byte_i, buf_size, cfg_);
        } else if ((((insn >> 16) & 0xFFFFu) & 0x3u) != 0x3u) {
          // upper half looks compressed -> mutate upper 16b
          CompressedMutator::mutateAt(out, byte_i + 2, buf_size, cfg_);
        } else {
          insn ^= (1u << (Random::rnd32() & 31u));
        }
        break;

      case 6: // U-type page nudge / toggle
        if (f == Fmt::U) {
          if (Random::rnd32() & 1u) uToggleOp(insn);
          else uMutateImmSmall(insn, (int32_t)((int32_t)(Random::rnd32()%9) - 4));
        } else {
          mutateImm32(insn);
        }
        break;

      case 7: // swap neighbor
      default:
        if (wi + 1 < nwords) {
          uint32_t other = get_u32_le(out, (wi+1)*4);
          put_u32_le(out, byte_i, other);
          put_u32_le(out, (wi+1)*4, insn);
          insn = get_u32_le(out, byte_i);
        } else {
          insn ^= (1u << (Random::rnd32() & 7u));
        }
        break;
    }

    // after 'insn' was updated (and before writing back)
    if (!is_legal_instruction(insn)) {
      const char* src = nullptr;
      switch (kind) {
        case 0: src = "mutate_regs32"; break;
        case 1: src = "mutate_imm32"; break;
        case 2: src = "replace_with_same_fmt32"; break;
        case 3: src = "insert_nop"; break;
        case 4: src = "delete_to_nop"; break;
        case 5: src = "mutate_compressed"; break;
        case 6: src = "utype_small"; break;
        case 7: src = "swap_neighbor"; break;
        default: src = "unknown_kind"; break;
      }
      MUTDBG_ILLEGAL(get_u32_le(out, byte_i), insn, src);
    }

    // Write back if within current buffer
    if (byte_i + 3 < buf_size) put_u32_le(out, byte_i, insn);
  }

  return out;
}

} // namespace mut
