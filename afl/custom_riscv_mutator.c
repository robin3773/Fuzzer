// rv32_custom_mutator.cpp
// Grammar-aware AFL++ custom mutator for RV32 (I/M/A/F/D/C + E support)
// Produces instruction-preserving mutations and compressed-aware edits.
// Build: g++ -O2 -fPIC -shared -o librv32_custom_mutator.so rv32_custom_mutator.cpp
// Usage: AFL_CUSTOM_MUTATOR_LIBRARY=./librv32_custom_mutator.so afl-fuzz ...

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


#define IMM_RANDOM_PROB 30.0f // probability of immediate mutation
#define IMM_DELTA_MAX 16     // max delta for immediate mutation
#define R_WEIGHT_BASE_ALU 70  // % chance to pick base ALU for R-type (ADD/SUB/AND/OR/..)
#define R_WEIGHT_M        30  // % chance to pick M-extension for R-type (MUL/DIV/REM)
#define I_SHIFT_WEIGHT    30  // % chance to prefer a shift-imm opcode in I-type
// -----------------------------
// Config / global flags
// -----------------------------
static int is_rv32e = 0; // 0 => RV32I (32 regs), 1 => RV32E (16 regs)

// -----------------------------
// Simple PRNG (xorshift32)
// -----------------------------
static uint32_t rng_state = 123456789u;
static inline void rng_seed(uint32_t s) { rng_state = s ? s : (uint32_t)time(nullptr); }
static inline uint32_t rnd32(void) {
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state = x;
  return x;
}
static inline uint32_t rnd_range(uint32_t n) { return n ? (rnd32() % n) : 0; }

// -----------------------------
// Formats
// -----------------------------
typedef enum {
  FMT_R, FMT_I, FMT_S, FMT_B, FMT_U, FMT_J,
  FMT_R4,    // FP fused (fmadd)
  FMT_A,     // atomic
  // Compressed subformats:
  FMT_C16,
  FMT_C_CR, FMT_C_CI, FMT_C_CSS, FMT_C_CIW, FMT_C_CL, FMT_C_CS, FMT_C_CB, FMT_C_CJ,
  FMT_UNKNOWN
} fmt_t;

// -----------------------------
// Byte/word helpers (little-endian)
// -----------------------------
static inline uint32_t get_u32_le(const unsigned char *b, size_t i) {
  return (uint32_t)b[i] | ((uint32_t)b[i+1] << 8) | ((uint32_t)b[i+2] << 16) | ((uint32_t)b[i+3] << 24);
}

static inline void put_u32_le(unsigned char *b, size_t i, uint32_t v) {
  b[i]   = v & 0xff;
  b[i+1] = (v >> 8) & 0xff;
  b[i+2] = (v >> 16) & 0xff;
  b[i+3] = (v >> 24) & 0xff;
}

// -----------------------------
// Extractors for 32-bit encodings
// -----------------------------
static inline uint32_t opcode32(uint32_t insn) { return insn & 0x7f; }
static inline uint32_t rd32(uint32_t insn) { return (insn >> 7) & 0x1f; }
static inline uint32_t funct332(uint32_t insn) { return (insn >> 12) & 0x7; }
static inline uint32_t rs132(uint32_t insn) { return (insn >> 15) & 0x1f; }
static inline uint32_t rs232(uint32_t insn) { return (insn >> 20) & 0x1f; }
static inline uint32_t funct732(uint32_t insn) { return (insn >> 25) & 0x7f; }

// -----------------------------
// U-type helpers
// -----------------------------
static inline uint32_t u_get_imm20(uint32_t insn) {
  return (insn >> 12) & 0xFFFFF;
}
static inline uint32_t u_set_imm20(uint32_t insn, uint32_t imm20) {
  return (insn & 0x00000FFFu) | ((imm20 & 0xFFFFFu) << 12);
}
static inline uint32_t u_toggle_op(uint32_t insn) {
  uint32_t op = opcode32(insn);
  uint32_t newop = (op == 0x37) ? 0x17u : 0x37u; // LUI <-> AUIPC
  return (insn & ~0x7Fu) | newop;
}
static inline uint32_t u_mutate_imm_small(uint32_t insn, int32_t pages_delta) {
  int32_t imm20 = (int32_t)u_get_imm20(insn);
  // sign-extend 20-bit:
  if (imm20 & 0x80000) imm20 |= ~0xFFFFF;
  imm20 += pages_delta;
  uint32_t out = u_set_imm20(insn, (uint32_t)imm20);
  return out;
}

// -----------------------------
// Instruction format detection (32-bit + compressed 16-bit subformats)
// -----------------------------
static fmt_t get_format(uint32_t insn32) {
  // First check if this is a 16-bit compressed instruction (lowest 2 bits not 11)
  uint16_t low16 = (uint16_t)(insn32 & 0xFFFFu);
  if ((low16 & 0x3u) != 0x3u) {               // compressed instruction
    uint8_t op_lo = low16 & 0x3u;               // bits [1:0]
    uint8_t funct3 = (uint8_t)((low16 >> 13) & 0x7u); // bits [15:13]
    switch (op_lo) { 
      case 0x0: // quadrant 0 
        switch (funct3) {
          case 0b000: return FMT_C_CIW; // c.addi4spn
          case 0b010: return FMT_C_CL;  // c.lw
          case 0b110: return FMT_C_CS;  // c.sw
          default:    return FMT_C16;
        }
      case 0x1: // quadrant 1
        switch (funct3) {
          case 0b000: return FMT_C_CI;  // c.addi / c.nop
          case 0b010: return FMT_C_CI;  // c.li
          case 0b011: return FMT_C_CI;  // c.lui / c.addi16sp
          case 0b100: return FMT_C_CB;  // c.srli / srai / andi family
          case 0b110: return FMT_C_CB;  // c.beqz
          case 0b111: return FMT_C_CB;  // c.bnez
          case 0b001: return FMT_C_CJ;  // c.jal
          case 0b101: return FMT_C_CJ;  // c.j
          default:    return FMT_C16;
        }
      case 0x2: // quadrant 2
        switch (funct3) {
          case 0b000: return FMT_C_CI;  // c.slli
          case 0b010: return FMT_C_CL;  // c.lwsp
          case 0b100: return FMT_C_CR;  // c.mv / c.add / c.jr / c.jalr / ebreak
          case 0b110: return FMT_C_CSS; // c.swsp
          default:    return FMT_C16;
        }
      default:
        return FMT_C16;
    }
  }

  // Standard 32-bit opcodes
  uint32_t op = opcode32(insn32); // bits [6:0] 
  switch (op) { 
    // RV32I base
    case 0x33: return FMT_R;  // OP (also M subspace)
    case 0x13: return FMT_I;  // OP-IMM
    case 0x03: return FMT_I;  // LOAD
    case 0x23: return FMT_S;  // STORE
    case 0x63: return FMT_B;  // BRANCH
    case 0x37: return FMT_U;  // LUI
    case 0x17: return FMT_U;  // AUIPC
    case 0x6F: return FMT_J;  // JAL
    case 0x67: return FMT_I;  // JALR
    case 0x0F: return FMT_I;  // FENCE
    case 0x73: return FMT_I;  // SYSTEM / CSR / ECALL

    // Atomic (A extension)
    case 0x2F: return FMT_A;

    // Floating point (F/D)
    case 0x07: return FMT_I;   // FLW/FLD
    case 0x27: return FMT_S;   // FSW/FSD
    case 0x53: return FMT_R;   // FP arithmetic
    case 0x43: case 0x47: case 0x4B: case 0x4F:
      return FMT_R4;           // FMADD/FMSUB/FNMSUB/FNMADD

    // 64-bit variants (treat similarly)
    case 0x1B: return FMT_I;
    case 0x3B: return FMT_R;

    // Vector/custom
    case 0x57: return FMT_R;
    case 0x0B: case 0x2B: case 0x5B: case 0x7B:
      return FMT_R;

    default:
      return FMT_UNKNOWN;
  }
}

// -----------------------------
// Helper: prefer registers
// -----------------------------
static inline uint32_t pick_reg(void) {
  // rarely choose x0, usually pick a low numbered register for embedded targets
  if ((rnd32() & 127u) == 0u) return 0u;
  uint32_t limit = is_rv32e ? 16u : 32u; // RV32E -> x0..x15
  // pick x1 .. x(limit-1)
  uint32_t r = 1u + (rnd32() % (limit - 1u));
  return r & 0x1Fu;
}

// -----------------------------
// Mutations for 32-bit formats
// -----------------------------
// Replace the old mutate_regs32 with this new version.
// This implements 8 distinct choices (bits 0..2 -> rd, rs1, rs2).
static void mutate_regs32(uint32_t &v) {
  fmt_t f = get_format(v);

  // Choice bits: bit0 -> rd, bit1 -> rs1, bit2 -> rs2
  // We sample 3 random bits giving 0..7 possibilities.
  uint32_t choice = rnd32() & 7u; // 3 low bits => 8 choices

  // For safety, if choice == 0 we'll remap later to mutate at least one applicable field.
  // Determine what fields exist for this format:
  bool has_rd  = (f == FMT_R || f == FMT_I || f == FMT_U || f == FMT_J || f == FMT_R4 || f == FMT_A);
  bool has_rs1 = (f == FMT_R || f == FMT_I || f == FMT_S || f == FMT_B || f == FMT_A || f == FMT_R4);
  bool has_rs2 = (f == FMT_R || f == FMT_S || f == FMT_B || f == FMT_A || f == FMT_R4);

  // If none of the chosen bits correspond to an available field, remap choice to a single-field mutation:
  uint32_t applicable_mask = (has_rd  ? 0x1u : 0u) | (has_rs1 ? 0x2u : 0u) | (has_rs2 ? 0x4u : 0u);
  if ((choice & applicable_mask) == 0u) {
    // If possible prefer rd, else rs1, else rs2
    if (has_rd)  choice = 0x1u;
    else if (has_rs1) choice = 0x2u;
    else if (has_rs2) choice = 0x4u;
    else choice = 0x1u; // fallback: mutate rd field bits even if format uncommon
  }

  // Now apply mutations according to the bits selected.
  // Each mutated field gets its own fresh pick_reg() value.
  if (choice & 0x1u) { // mutate rd
    uint32_t newrd = pick_reg();
    v = (v & ~(0x1Fu << 7)) | (newrd << 7);
  }

  if (choice & 0x2u) { // mutate rs1
    // Only apply if rs1 exists for this format
    if (has_rs1) {
      uint32_t newrs1 = pick_reg();
      v = (v & ~(0x1Fu << 15)) | (newrs1 << 15);
    }
  }

  if (choice & 0x4u) { // mutate rs2
    // Only apply if rs2 exists for this format
    if (has_rs2) {
      uint32_t newrs2 = pick_reg();
      v = (v & ~(0x1Fu << 20)) | (newrs2 << 20);
    }
  }

  // Special-case: for I-type formats which have rs1 but not rs2, the user might have selected rs2 bit.
  // The above logic ignores rs2 when not present; we already remapped choice to ensure at least one
  // applicable field was selected.

  // Ensure register fields respect RV32E if enabled
  if (is_rv32e) {
    uint32_t rdv  = (v >> 7) & 0x1Fu;  rdv  &= 0xFu;
    uint32_t rs1v = (v >> 15) & 0x1Fu; rs1v &= 0xFu;
    uint32_t rs2v = (v >> 20) & 0x1Fu; rs2v &= 0xFu;
    v &= ~(((0x1Fu)<<7) | ((0x1Fu)<<15) | ((0x1Fu)<<20));
    v |= (rdv << 7) | (rs1v << 15) | (rs2v << 20);
  }
}


// -----------------------------------------------------------------------------
// mutate_imm32(): Hybrid immediate mutation (delta + random)
// Probability of random scheme is set by IMM_RANDOM_PERCENT (0–100)
// -----------------------------------------------------------------------------
sstatic void mutate_imm32(uint32_t &v) {
  // Nonzero symmetric delta in [-IMM_DELTA_MAX, +IMM_DELTA_MAX]
  auto rand_delta = []() -> int32_t {
    const int32_t M = (int32_t)IMM_DELTA_MAX;
    if (M <= 0) return 1; // fallback
    int32_t d = (int32_t)(rnd32() % (2u * (uint32_t)M + 1u)) - M;
    if (d == 0) d = (rnd32() & 1u) ? 1 : -1;
    return d;
  };

  fmt_t f = get_format(v);
  bool use_random = ((rnd32() % 100u) < (uint32_t)IMM_RANDOM_PERCENT);

  // ---------------------- Δ-scheme helper (contiguous field) ----------------------
  auto mutate_delta_field = [&](int bits, int shift, uint32_t mask) {
    int32_t delta = rand_delta();
    int32_t imm = (int32_t)((v >> shift) & mask);
    if (bits > 0 && (imm & (1 << (bits - 1)))) imm |= ~((1 << bits) - 1); // sign-extend
    imm += delta;
    uint32_t newimm = (uint32_t)(imm & ((bits >= 32) ? 0xFFFFFFFFu : ((1u << bits) - 1)));
    v = (v & ~(mask << shift)) | (newimm << shift);
  };

  if (f == FMT_I) {
    if (use_random) {
      uint32_t newimm = rnd32() & 0xFFFu;
      v = (v & ~(0xFFFu << 20)) | (newimm << 20);
    } else {
      mutate_delta_field(12, 20, 0xFFFu);
    }
  }

  else if (f == FMT_S) {
    if (use_random) {
      uint32_t newimm = rnd32() & 0xFFFu;
      uint32_t hi = (newimm >> 5) & 0x7Fu, lo = newimm & 0x1Fu;
      v &= ~(((uint32_t)0x7Fu << 25) | ((uint32_t)0x1Fu << 7));
      v |= (hi << 25) | (lo << 7);
    } else {
      int32_t imm = ((v >> 25) << 5) | ((v >> 7) & 0x1F);
      if (imm & 0x800) imm |= 0xFFFFF000;
      imm += rand_delta();
      uint32_t newimm = (uint32_t)imm & 0xFFFu;
      uint32_t hi = (newimm >> 5) & 0x7Fu, lo = newimm & 0x1Fu;
      v &= ~(((uint32_t)0x7Fu << 25) | ((uint32_t)0x1Fu << 7));
      v |= (hi << 25) | (lo << 7);
    }
  }

  else if (f == FMT_B) {
    if (use_random) {
      uint32_t newimm = rnd32() & 0x1FFFu; newimm &= ~1u; // even
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
      imm = (uint32_t)((int32_t)imm + (d << 1)); // keep even alignment
      uint32_t newimm = imm & 0x1FFFu;
      uint32_t b31 = (newimm >> 12) & 1, b30_25 = (newimm >> 5) & 0x3F;
      uint32_t b11_8 = (newimm >> 1) & 0xF, b7 = (newimm >> 11) & 1;
      v &= ~(((uint32_t)1 << 31) | ((uint32_t)0x3F << 25) | ((uint32_t)0xF << 8) | ((uint32_t)1 << 7));
      v |= (b31 << 31) | (b30_25 << 25) | (b11_8 << 8) | (b7 << 7);
    }
  }

  else if (f == FMT_U) {
    if (use_random) {
      uint32_t newimm = rnd32() & 0xFFFFFu;
      v = (v & 0x00000FFFu) | (newimm << 12);
    } else {
      int32_t imm20 = (int32_t)((v >> 12) & 0xFFFFF);
      if (imm20 & 0x80000) imm20 |= ~0xFFFFF;
      imm20 += rand_delta();
      uint32_t newimm = (uint32_t)imm20 & 0xFFFFFu;
      v = (v & 0x00000FFFu) | (newimm << 12);
    }
  }

  else if (f == FMT_J) {
    if (use_random) {
      uint32_t newimm = rnd32() & 0x1FFFFFu; newimm &= ~1u;
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
      int32_t d = rand_delta();
      imm = (uint32_t)((int32_t)imm + (d << 1)); // even alignment
      uint32_t newimm = imm & 0x1FFFFFu;
      uint32_t bit20 = (newimm >> 20) & 1, bit11 = (newimm >> 11) & 1;
      uint32_t bits10_1 = (newimm >> 1) & 0x3FF, bits19_12 = (newimm >> 12) & 0xFF;
      v &= ~(((uint32_t)1 << 31) | ((uint32_t)0x3FF << 21) | ((uint32_t)1 << 20) | ((uint32_t)0xFF << 12));
      v |= (bit20 << 31) | (bits19_12 << 12) | (bit11 << 20) | (bits10_1 << 21);
    }
  }

  else {
    // Unknown/other: keep a tiny perturbation
    if (use_random) v ^= (1u << (rnd32() & 31u));
    else            v ^= (1u << (rnd32() & 7u));
  }
}

// -----------------------------

// ---------- Helper to clamp a percent ----------
static inline uint32_t clamp_pct(int x) {
  if (x < 0) return 0; if (x > 100) return 100; return (uint32_t)x;
}

// replace with different same-format instruction (funct3/funct7)
static void replace_with_same_fmt32(uint32_t &v) {
  fmt_t f  = get_format(v);
  uint32_t op = opcode32(v);

  // -----------------------------
  // R-type (integer OP opcode 0x33): base ALU and M-extension
  // -----------------------------
  if (f == FMT_R && op == 0x33u) {
    // Base ALU: (funct3, funct7)
    static const uint8_t R_BASE[][2] = {
      {0x0,0x00}, // ADD
      {0x0,0x20}, // SUB
      {0x1,0x00}, // SLL
      {0x2,0x00}, // SLT
      {0x3,0x00}, // SLTU
      {0x4,0x00}, // XOR
      {0x5,0x00}, // SRL
      {0x5,0x20}, // SRA
      {0x6,0x00}, // OR
      {0x7,0x00}  // AND
    };
    // M-extension (funct3 with funct7=0x01)
    static const uint8_t R_M[][2] = {
      {0x0,0x01}, // MUL
      {0x1,0x01}, // MULH
      {0x2,0x01}, // MULHSU
      {0x3,0x01}, // MULHU
      {0x4,0x01}, // DIV
      {0x5,0x01}, // DIVU
      {0x6,0x01}, // REM
      {0x7,0x01}  // REMU
    };

    uint32_t w_base = clamp_pct(R_WEIGHT_BASE_ALU);
    uint32_t w_m    = clamp_pct(R_WEIGHT_M);
    uint32_t total  = w_base + w_m; if (!total) { w_base = 100; total = 100; }

    uint32_t pick = rnd32() % total;
    const uint8_t (*tbl)[2]; size_t n;
    if (pick < w_base) { tbl = R_BASE; n = sizeof(R_BASE)/sizeof(R_BASE[0]); }
    else               { tbl = R_M;    n = sizeof(R_M)/sizeof(R_M[0]); }

    const uint8_t *sel = tbl[rnd32() % n];
    uint32_t f3 = sel[0], f7 = sel[1];

    // write funct3 and funct7
    v = (v & ~(((uint32_t)0x7u << 12) | ((uint32_t)0x7Fu << 25)))
        | (f3 << 12) | (f7 << 25);
    return;
  }

  // If this looks like R-type but not integer OP (e.g., FP opcode 0x53), fall through to a conservative tweak
  if (f == FMT_R) {
    // Keep prior simple set as a fallback
    static const uint8_t FALLBACK[][2] = {
      {0x0,0x00},{0x0,0x20},{0x4,0x00},{0x6,0x00},{0x7,0x00}
    };
    const uint8_t *sel = FALLBACK[rnd32() % (sizeof(FALLBACK)/sizeof(FALLBACK[0]))];
    uint32_t f3 = sel[0], f7 = sel[1];
    v = (v & ~(((uint32_t)0x7u << 12) | ((uint32_t)0x7Fu << 25)))
        | (f3 << 12) | (f7 << 25);
    return;
  }

  // -----------------------------
  // I-type arithmetic/logic (OP-IMM opcode 0x13)
  // Proper handling of shifts: SRLI vs SRAI uses bit 30 in funct7
  // -----------------------------
  if (f == FMT_I && op == 0x13u) {
    // Valid funct3 set: 000 ADDI, 010 SLTI, 011 SLTIU, 100 XORI, 110 ORI, 111 ANDI, 001 SLLI, 101 SRLI/SRAI
    static const uint8_t I_ALL[] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7};
    // Bias toward shifts (SLLI/SRLI/SRAI) using I_SHIFT_WEIGHT
    uint32_t w_shift = clamp_pct(I_SHIFT_WEIGHT);
    bool choose_shift = ((rnd32() % 100u) < w_shift);
    uint32_t f3;
    if (choose_shift) {
      // pick among {001, 101}
      f3 = (rnd32() & 1u) ? 0x1u : 0x5u;
    } else {
      f3 = I_ALL[rnd32() % (sizeof(I_ALL)/sizeof(I_ALL[0]))];
    }

    // Set funct3
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);

    // If shift-immediate:
    if (f3 == 0x1u) {
      // SLLI must have funct7 bit30=0 on RV32
      v &= ~((uint32_t)1u << 30);
    } else if (f3 == 0x5u) {
      // SRLI/SRAI: bit30=0 -> SRLI, bit30=1 -> SRAI
      if (rnd32() & 1u) v |=  ((uint32_t)1u << 30);  // SRAI
      else               v &= ~((uint32_t)1u << 30); // SRLI
    }
    return;
  }

  // Generic I-type (loads/JALR/FENCE/SYSTEM) -> keep prior safe change to funct3 only
  if (f == FMT_I) {
    // modest pool that maps to known I-type families on many opcodes
    static const uint8_t I_OPT[] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7};
    uint32_t f3 = I_OPT[rnd32() % (sizeof(I_OPT)/sizeof(I_OPT[0]))];
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    return;
  }

  // -----------------------------
  // S-type (STORE opcode 0x23): restrict to valid store widths
  // -----------------------------
  if (f == FMT_S && op == 0x23u) {
    static const uint8_t S_VALID[] = {0x0,0x1,0x2}; // SB, SH, SW
    uint32_t f3 = S_VALID[rnd32() % (sizeof(S_VALID)/sizeof(S_VALID[0]))];
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    return;
  }
  if (f == FMT_S) {
    // fallback: still keep in 3-bit range
    uint32_t f3 = rnd32() & 0x7u;
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    return;
  }

  // -----------------------------
  // B-type (BRANCH opcode 0x63): restrict to valid branch funct3
  // -----------------------------
  if (f == FMT_B && op == 0x63u) {
    static const uint8_t B_VALID[] = {0x0,0x1,0x4,0x5,0x6,0x7}; // BEQ,BNE,BLT,BGE,BLTU,BGEU
    uint32_t f3 = B_VALID[rnd32() % (sizeof(B_VALID)/sizeof(B_VALID[0]))];
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    return;
  }
  if (f == FMT_B) {
    uint32_t f3 = rnd32() & 0x7u;
    v = (v & ~((uint32_t)0x7u << 12)) | (f3 << 12);
    return;
  }

  // -----------------------------
  // U-type (LUI/AUIPC): occasional toggle
  // -----------------------------
  if (f == FMT_U) {
    if ((rnd32() & 3u) == 0u) v = u_toggle_op(v); // ~25%
    return;
  }

  // Other formats: leave unchanged (or flip a tiny bit if you prefer)
}

// -----------------------------
// Compressed (16-bit) helpers & simple mutations
// -----------------------------
// We'll treat compressed instructions as 16-bit units stored in the low 16 bits
// of a 32-bit word. For safety we mutate only fields we can parse trivially.

static inline uint16_t get_u16(const unsigned char *b, size_t i) {
  return (uint16_t)b[i] | ((uint16_t)b[i+1] << 8);
}
static inline void put_u16(unsigned char *b, size_t i, uint16_t v) {
  b[i]   = v & 0xff;
  b[i+1] = (v >> 8) & 0xff;
}

// Helpers to mutate compressed immediates / regs (basic & conservative)
static void mutate_compressed_at(unsigned char *buf, size_t byte_i, size_t buf_size) {
  if (byte_i + 1 >= buf_size) return;
  uint16_t c = get_u16(buf, byte_i);
  uint8_t op_lo = c & 0x3u;
  uint8_t funct3 = (c >> 13) & 0x7u;

  // simple mutation strategies per subformat
  if ((op_lo == 0x0 && funct3 == 0b010) || // c.lw
      (op_lo == 0x0 && funct3 == 0b110) || // c.sw
      (op_lo == 0x2 && funct3 == 0b010)) { // c.lwsp
    // adjust load/store immediate (bits scattered) lightly by +/-1 slot
    // For safety, flip one of the immediate bits instead of complex re-encoding
    uint16_t mask = (1u << (4 + (rnd32() & 3))); // flip one of some immediate bits
    c ^= mask;
    put_u16(buf, byte_i, c);
    return;
  }

  if ((op_lo == 0x1 && (funct3 == 0b001 || funct3 == 0b101)) || // c.jal/c.j
      (op_lo == 0x1 && (funct3 == 0b110 || funct3 == 0b111))) { // c.beqz/c.bnez
    // flip a bit in offset field
    uint16_t mask = 1u << (1 + (rnd32() % 10));
    c ^= mask;
    put_u16(buf, byte_i, c);
    return;
  }

  // CR/CI: change rd'/rs' fields (compressed 3-bit registers)
  if ((op_lo == 0x2 && funct3 == 0b100) || // CR-group
      (op_lo == 0x1 && funct3 == 0b000) || // CI-group (addi / nop)
      (op_lo == 0x2 && funct3 == 0b000)) { // SLLI etc.
    // flip one of rd' bits (bits 2:0 of compressed register)
    uint16_t mask = 1u << (2 + (rnd32() % 3));
    c ^= mask;
    put_u16(buf, byte_i, c);
    return;
  }

  // fallback: flip a low random bit (very small chance of making malformed instruction)
  c ^= (1u << (rnd32() & 15));
  put_u16(buf, byte_i, c);
}

// -----------------------------
// Core mutation driver (treat input as sequence of 32-bit words,
// but we recognize and mutate compressed-in-16-bit layouts conservatively).
// -----------------------------
static unsigned char *mutate_instruction_stream(const unsigned char *inbuf, size_t in_len, size_t *out_len, unsigned int seed) {
  rng_seed(seed);
  // treat input as a sequence of 32-bit words; allow the last word to be partial
  size_t nwords = (in_len + 3) / 4;
  size_t buf_size = nwords * 4;
  unsigned char *out = (unsigned char *)malloc(buf_size + 4);
  if (!out) return nullptr;
  memset(out, 0, buf_size + 4);
  memcpy(out, inbuf, in_len);

  // number of simple mutations per invocation
  unsigned nmuts = 1 + (rnd32() % 3u);

  for (unsigned mi = 0; mi < nmuts; ++mi) {
    if (nwords == 0) {
      // create a small nop word if input empty
      uint32_t nop = 0x00000013u;
      put_u32_le(out, 0, nop);
      *out_len = 4;
      return out;
    }

    size_t wi = rnd_range(nwords);
    size_t byte_i = wi * 4;
    uint32_t insn = get_u32_le(out, byte_i);
    fmt_t f = get_format(insn);

    uint32_t kind = rnd32() % 8u;
    switch (kind) {
      case 0:
        mutate_regs32(insn);
        break;
      case 1:
        mutate_imm32(insn);
        break;
      case 2:
        replace_with_same_fmt32(insn);
        break;
      case 3:
        // insert nop (32-bit) before this word - expand buffer
        {
          unsigned char *tmp = (unsigned char *)malloc(buf_size + 8);
          if (!tmp) break;
          memcpy(tmp, out, byte_i);
          put_u32_le(tmp, byte_i, 0x00000013u); // nop
          memcpy(tmp + byte_i + 4, out + byte_i, buf_size - byte_i);
          free(out);
          out = tmp;
          buf_size += 4;
          nwords += 1;
          // sync insn to next position (we don't modify insn further here)
          insn = get_u32_le(out, byte_i + 4);
        }
        break;
      case 4:
        // delete -> safer to replace with nop
        insn = 0x00000013u;
        break;
      case 5:
        // If compressed detected in this 32-bit slot, mutate compressed halves
        if (((insn & 0xFFFFu) & 0x3u) != 0x3u) {
          mutate_compressed_at(out, byte_i, buf_size);
        } else if (((insn >> 16) & 0xFFFFu) & 0x3u) {
          // upper half compressed? (rare if constructed so)
          mutate_compressed_at(out, byte_i + 2, buf_size);
        } else {
          // fallback: flip a bit in the whole word
          insn ^= (1u << (rnd32() & 31u));
        }
        break;
      case 6:
        // convert low-risk: toggle U-type op or small page move
        if (f == FMT_U) {
          if (rnd32() & 1) insn = u_toggle_op(insn);
          else insn = u_mutate_imm_small(insn, (int32_t)((int32_t)(rnd32()%9) - 4));
        } else {
          // fallback minor tweak to immediate
          mutate_imm32(insn);
        }
        break;
      case 7:
      default:
        // cross-instruction small swap: swap with neighbor if exists
        if (wi + 1 < nwords) {
          uint32_t other = get_u32_le(out, (wi + 1) * 4);
          put_u32_le(out, byte_i, other);
          put_u32_le(out, (wi + 1) * 4, insn);
          insn = get_u32_le(out, byte_i); // after swap
        } else {
          // xor a low bit
          insn ^= (1u << (rnd32() & 7u));
        }
        break;
    }

    // write back mutated 32-bit instruction (if we didn't already adjust buffer)
    if (byte_i + 3 < buf_size) put_u32_le(out, byte_i, insn);
  }

  *out_len = buf_size;
  return out;
}

// -----------------------------
// AFL custom mutator API entrypoints (provide multiple common variants)
// -----------------------------
extern "C" {

// Variant A: modern signature (most AFL++)
unsigned char *afl_custom_fuzz(unsigned char *buf, size_t buf_size,
                               unsigned char *out_buf, size_t max_size,
                               unsigned int seed) {
  size_t out_len = 0;
  unsigned char *m = mutate_instruction_stream(buf, buf_size, &out_len, seed);
  if (!m) return nullptr;
  if (out_buf && out_len <= max_size) {
    memcpy(out_buf, m, out_len);
    free(m);
    // AFL expects a pointer to out_buf
    return out_buf;
  }
  // return heap-allocated buffer (AFL will free it)
  return m;
}

// Variant B: alternate older signature
size_t afl_custom_fuzz_b(unsigned char *data, size_t size, unsigned char **out_buf, unsigned int seed) {
  size_t out_len = 0;
  unsigned char *m = mutate_instruction_stream(data, size, &out_len, seed);
  if (!m) { *out_buf = nullptr; return 0; }
  *out_buf = m;
  return out_len;
}

// init / deinit
int afl_custom_init(void *afl) {
  (void)afl;
  const char *arch = getenv("RV32_MODE");
  if (arch && strchr(arch, 'E')) {
    is_rv32e = 1;
    fprintf(stderr, "[mutator] RV32E mode (16 regs)\n");
  } else {
    is_rv32e = 0;
    fprintf(stderr, "[mutator] RV32I mode (32 regs)\n");
  }
  rng_seed((uint32_t)time(nullptr));
  return 0;
}
void afl_custom_deinit(void) {
  // nothing to free globally
}

// havoc mutation alias
size_t afl_custom_havoc_mutation(unsigned char *data, size_t size, unsigned char **out_buf, unsigned int seed) {
  return afl_custom_fuzz_b(data, size, out_buf, seed);
}

// Provide weak alias to cover symbol name variations
__attribute__((weak, alias("afl_custom_fuzz"))) unsigned char *afl_custom_fuzz_alias(unsigned char *buf, size_t buf_size,
                                                                                     unsigned char *out_buf, size_t max_size,
                                                                                     unsigned int seed);
} // extern "C"
