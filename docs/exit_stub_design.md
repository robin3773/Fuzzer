# EXIT_STUB Design — YAML-Driven Program Termination

## Purpose
Provide a **deterministic, ISA-compliant end-of-stream marker** for fuzzed programs to prevent infinite loops and timeouts during fuzzing campaigns.

## Mechanism
The exit stub is a **5-instruction sequence** appended to every fuzzed program:

```asm
# Materialize TOHOST_ADDR (0x80001000) in t0
lui  t0, 0x80001        # t0 = 0x80001000 (upper 20 bits)
addi t0, t0, 0          # t0 = t0 + 0 (lower 12 bits, sign-corrected)

# Write exit marker (1) to TOHOST_ADDR
addi t1, x0, 1          # t1 = 1
sw   t1, 0(t0)          # mem[TOHOST_ADDR] = 1

# Secondary stop signal
ebreak                  # Trap to signal clean exit
```

**Total size**: 5 words (20 bytes)

---

## Integration Points

### 1. YAML Schema (`schemas/riscv/riscv32_exit.yaml`)
- **Constants**: Defines `TOHOST_ADDR = 0x80001000` (must match harness config)
- **Macros**: `HI20` and `LO12` for sign-correct LUI/ADDI splitting
- **Pseudo-instruction**: `EXIT_STUB` expands to the 5-instruction sequence
- **Locked**: Marked to prevent mutations from corrupting the stub

### 2. Custom Mutator (`afl/isa_mutator/`)
- **ExitStub module** (`include/fuzz/mutator/ExitStub.hpp`):
  - Instruction encoders: `encode_lui()`, `encode_addi()`, `encode_sw()`
  - `append_exit_stub()`: Appends 5-instruction sequence to buffer
  - `is_tail_locked()`: Guards to protect last 5 instructions
- **ISAMutator** (`src/ISAMutator.cpp`):
  - **Post-mutation hook**: Calls `append_exit_stub()` after all mutations complete
  - **Size reservation**: Allocates extra 20 bytes for stub before mutation
  - **Tail protection**: Skips mutations that would overlap stub region
  - Applied to both schema-based and fallback mutation paths

### 3. Harness (`afl_harness/`)
- **DutExit module** (`include/DutExit.hpp`, `src/DutExit.cpp`):
  - **Validation only**: No longer appends stub (mutator handles it)
  - `apply_program_envelope()`: Size checking and alignment padding
  - Detects and logs EBREAK presence for debugging
- **Exit detection** in `HarnessMain.cpp` main loop:
  ```cpp
  // Detect MMIO write to TOHOST_ADDR
  if ((rec.mem_wmask & 0xF) != 0 && (rec.mem_addr & ~0x3u) == (cfg.tohost_addr & ~0x3u)) {
      exit_reason = ExitReason::Tohost;
      graceful_exit = true;
      break;
  }
  
  // Detect EBREAK trap
  if (rec.trap) {
      exit_reason = ExitReason::Ecall;
      graceful_exit = true;
      break;
  }
  ```

### 4. Spike Golden Model
- Treats store to `TOHOST_ADDR` as normal RAM write (no special handling)
- Stop lockstep comparison when DUT signals "done" via MMIO write or EBREAK trap
- Spike fatal trap detection implemented to prevent infinite exception loops

---

## Configuration

### Environment Variables
| Variable | Default | Description |
|----------|---------|-------------|
| `TOHOST_ADDR` | `0x80001000` | Magic MMIO address for exit detection |
| `APPEND_EXIT_STUB` | `yes` | Enable/disable stub generation |
| `USE_TOHOST` | `yes` | Use tohost write (vs. ECALL-only) |
| `MAX_PROGRAM_WORDS` | `256` | Max payload size (excluding stub) |

### Harness Config (`HarnessConfig`)
```cpp
struct HarnessConfig {
  uint32_t tohost_addr = 0x80001000;  // Must match YAML constant
  bool append_exit_stub = true;
  bool use_tohost = true;
  size_t max_program_words = 256;
  // ...
};
```

---

## Safety Features

1. **Size Enforcement**: Payload trimmed to `MAX_PROGRAM_WORDS - 5` before appending stub
2. **Alignment Padding**: Partial trailing bytes padded with NOPs
3. **Fallback NOP**: Empty inputs get a single NOP to prevent crashes
4. **Tail Locking**: Last 5 instructions protected from mutations (TODO: implement in mutator)
5. **Watchdogs**: `MAX_CYCLES` and PC-stagnation limits remain as backstops

---

## Benefits

- **No infinite loops**: Every program has a guaranteed exit path
- **Fast fuzzing**: Programs terminate quickly instead of timing out
- **ISA-agnostic**: Exit stub defined in YAML, not hardcoded
- **Golden model compatible**: Spike treats tohost write as normal RAM access
- **Mutation-resistant**: Tail-locking prevents stub corruption
- **AFL++-native**: Mutator appends stub, so AFL++ corpus files are self-contained

---

## Workflow

```
┌─────────────────┐
│  AFL++ Fuzzer   │
└────────┬────────┘
         │ (raw bytes)
         ▼
┌─────────────────────────────┐
│  Custom Mutator             │
│  (afl/isa_mutator/)         │
│                             │
│  1. Mutate instructions     │
│  2. Append exit stub (5w)   │
│  3. Protect tail region     │
└────────┬────────────────────┘
         │ (mutated + stub)
         ▼
┌─────────────────────────────┐
│  Harness                    │
│  (afl_harness/)             │
│                             │
│  1. Validate size/alignment │
│  2. Load into DUT           │
│  3. Detect tohost write     │
│  4. Detect EBREAK trap      │
└─────────────────────────────┘
```

---

## Future Work

1. **Environment overrides**: Allow runtime config of `TOHOST_ADDR` via env
2. **Multi-ISA support**: Extend to RV64, ARM, etc.
3. **Unit tests**: Verify encoding, disassembly, and harness behavior
4. **Compressed support**: Handle RVC (16-bit) exit stub variant

---

## References

- YAML schema: [`schemas/riscv/riscv32_exit.yaml`](../schemas/riscv/riscv32_exit.yaml)
- Harness module: [`afl_harness/include/DutExit.hpp`](../afl_harness/include/DutExit.hpp)
- Configuration: [`afl_harness/include/HarnessConfig.hpp`](../afl_harness/include/HarnessConfig.hpp)
