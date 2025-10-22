
#pragma once
// Placeholder for RV64 expansion: stubs and TODOs.
// Future direction:
//  - IR structure may need XLEN-aware shamt rules
//  - Decoder/Encoder variants for 64-bit opcodes (e.g., OP-IMM-32/OP-32)
//  - Sign-extension semantics for 32-bit ops on RV64
//  - Additional opcodes (LD/SD, etc.)

namespace mut {
struct RV64Placeholder {
  static void note() {
    // Intentionally empty; used to keep the link boundary in place.
  }
};
} // namespace mut
