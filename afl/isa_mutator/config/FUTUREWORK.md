# Future Work: ISA Mutator Enhancements

This document captures planned enhancements and ideas for the ISA-aware custom mutator. Current implementation focuses on simplicity and correctness - these features are reserved for future development.

---

## 1. Configurable Mutation Parameters

### Problem
Currently, mutation behavior is hardcoded (e.g., 1-50 mutations per call, 25% probability per strategy, 50%/30%/20% size biases).

### Proposed Enhancement
Add tunable parameters to YAML config:

```yaml
mutation:
  min_mutations_per_call: 1        # Currently: hardcoded to 1
  max_mutations_per_call: 50       # Currently: hardcoded to 50
  
  # Strategy probabilities (must sum to 100)
  strategy_probabilities:
    replace: 25                     # Currently: uniform 25%
    insert: 25                      # Currently: uniform 25%
    delete: 25                      # Currently: uniform 25%
    duplicate: 25                   # Currently: uniform 25%
  
  # Size adjustment biases (must sum to 100)
  size_biases:
    grow: 50                        # Currently: hardcoded 50%
    shrink: 30                      # Currently: hardcoded 30%
    keep: 20                        # Currently: hardcoded 20%

constraints:
  min_payload_instructions: 16     # Currently: hardcoded constexpr
  max_payload_instructions: 512    # Currently: hardcoded constexpr
  max_grow_amount: 200             # Currently: hardcoded in applyMutations
  max_shrink_amount: 100           # Currently: hardcoded in applyMutations
```

### Implementation Notes
- Add fields to `Config` struct in `MutatorConfig.hpp`
- Use in `applyMutations()` instead of hardcoded values
- Validate that probabilities sum to 100
- Add sanity checks (e.g., min < max)

### Benefit Level: **LOW**
- AFL++ already adapts based on coverage feedback
- Uniform random provides good exploration
- Only needed for ISA-specific tuning experiments

---

## 2. Advanced Strategy: Weighted Instruction Selection

### Problem
Current implementation uses uniform random selection from ISA schema via `pickInstruction()`. Doesn't favor any instruction type.

### Proposed Enhancement
Add configurable weights for instruction selection:

```yaml
instruction_weights:
  # Weight by extension (higher = more likely to be selected)
  extensions:
    base_alu: 70        # ADD, SUB, AND, OR, XOR, SLL, SRL, etc.
    memory: 50          # LW, SW, LB, SB, etc.
    branches: 40        # BEQ, BNE, BLT, BGE, etc.
    multiply: 30        # MUL, MULH, DIV, REM (M extension)
    atomic: 20          # LR, SC, AMO* (A extension)
    floating: 10        # FADD, FMUL, etc. (F/D extensions)
  
  # OR: Weight by category
  categories:
    arithmetic: 60
    logical: 50
    memory_access: 40
    control_flow: 30
    privileged: 10
```

### Implementation
```cpp
// Add to ISAMutator class:
const isa::InstructionSpec& pickWeightedInstruction() const {
  // Build weighted distribution from cfg_.instruction_weights
  // Group instructions by extension/category
  // Use std::discrete_distribution or custom weighted random
  // Return selected instruction
}

// Replace in applyMutations():
uint32_t encoded = encodeInstruction(pickWeightedInstruction()); // Instead of pickInstruction()
```

### Benefit Level: **MEDIUM**
- Could help focus fuzzing on specific instruction types
- Useful for targeting specific hardware blocks
- But: AFL++ queue management already does this at test case level

---

## 3. ADAPTIVE Strategy Implementation

### Problem
`Strategy::ADAPTIVE` enum exists but does nothing - just falls through to INSTRUCTION_LEVEL.

### Proposed Enhancement
Make ADAPTIVE truly adaptive based on AFL++ coverage feedback:

```yaml
strategy: ADAPTIVE

adaptive_config:
  learning_rate: 0.1               # How fast to adapt weights
  exploration_rate: 0.2            # % of time to try non-optimal strategies
  update_interval: 100             # Update weights every N test cases
  
  # Track success rate of each mutation strategy
  track_metrics:
    - coverage_gain_per_strategy   # Which strategy finds new coverage
    - instruction_sensitivity      # Which instructions trigger new paths
    - size_effectiveness           # Optimal program sizes
```

### Implementation
```cpp
// 1. Add feedback tracking in AFLInterface.cpp:
extern "C" {
  void afl_custom_queue_new_entry(void *afl,
                                  const uint8_t *filename_new_queue,
                                  const uint8_t *filename_orig_queue) {
    // Called when AFL++ finds interesting test case
    // Track which mutation strategy was used
    // Update success statistics
    mutator.recordSuccessfulMutation(last_strategy_used);
  }
}

// 2. Add to ISAMutator class:
class ISAMutator {
  struct AdaptiveStats {
    std::array<size_t, 4> strategy_success_count;  // REPLACE/INSERT/DELETE/DUPLICATE
    std::map<std::string, size_t> instruction_coverage_gain;
    size_t total_mutations;
  };
  
  AdaptiveStats adaptive_stats_;
  
  // Select strategy based on learned weights
  unsigned selectAdaptiveStrategy() const {
    // Use exploration_rate for random selection
    if (Random::chancePct(cfg_.adaptive.exploration_rate * 100)) {
      return Random::rnd32() % 4;  // Explore
    }
    // Otherwise use learned probabilities
    return selectWeightedStrategy(adaptive_stats_);
  }
};

// 3. Use in applyMutations():
unsigned strategy = (cfg_.strategy == Strategy::ADAPTIVE) 
                    ? selectAdaptiveStrategy()
                    : Random::rnd32() % 4;
```

### Benefit Level: **HIGH**
- Learns what works for specific targets
- Adapts to ISA-specific characteristics
- Could significantly improve fuzzing efficiency
- Research opportunity - publishable results

---

## 4. Instruction Filtering and Constraints

### Problem
No way to exclude certain instructions or limit to specific subsets. Useful for:
- Excluding privileged/system instructions (CSR, ECALL, EBREAK)
- Focusing on specific extensions during debugging
- Handling incomplete hardware implementations

### Proposed Enhancement
```yaml
instruction_filters:
  # Exclude specific instructions by mnemonic
  exclude_instructions:
    - "ecall"           # System call
    - "ebreak"          # Breakpoint
    - "fence"           # Memory fence
    - "fence.i"         # Instruction fence
    - "wfi"             # Wait for interrupt
  
  # Exclude entire extensions
  exclude_extensions:
    - "zicsr"           # CSR instructions
    - "f"               # Floating point (if FPU not implemented)
    - "d"               # Double precision FP
    - "a"               # Atomics (if not supported)
  
  # Only include specific extensions (whitelist mode)
  include_only_extensions:
    - "i"               # Base integer only
    - "m"               # + Multiply/divide
  
  # ISA-specific constraints
  constraints:
    register_count: 32           # 32 for RV32, 16 for RV32E
    max_immediate_width: 12      # Limit immediate field sizes
    allow_compressed: true       # Enable/disable RVC instructions
    align_memory_accesses: true  # Force alignment for LW/SW
```

### Implementation
```cpp
// 1. Filter instructions during schema loading in IsaLoader.cpp:
isa::ISAConfig load_isa_config(const std::string& isa_name, const Config& cfg) {
  auto config = load_isa_config(isa_name);  // Load full schema
  
  // Apply filters from cfg.instruction_filters
  config.instructions.erase(
    std::remove_if(config.instructions.begin(), config.instructions.end(),
      [&](const InstructionSpec& spec) {
        return cfg.shouldExclude(spec);
      }),
    config.instructions.end()
  );
  
  return config;
}

// 2. Add validation in encodeInstruction():
uint32_t ISAMutator::encodeInstruction(const isa::InstructionSpec &spec) const {
  uint32_t word = /* ... existing encoding ... */;
  
  // Apply runtime constraints
  if (cfg_.constraints.align_memory_accesses && isMemoryInstruction(spec)) {
    // Ensure address alignment by masking immediate bits
  }
  
  return word;
}
```

### Benefit Level: **HIGH**
- Essential for incomplete hardware implementations
- Helps isolate bugs to specific instruction types
- Improves debugging efficiency
- Required for differential testing with simulators

---

## 5. MIXED_MODE Strategy Enhancement

### Problem
`Strategy::MIXED_MODE` exists but not implemented - would combine instruction-level and byte-level mutations.

### Proposed Enhancement
```yaml
strategy: MIXED_MODE

mixed_mode_config:
  instruction_level_probability: 70   # 70% instruction-level mutations
  byte_level_probability: 30          # 30% byte-level mutations
  
  byte_mutations:
    enable_bitflips: true              # Single bit flips
    enable_byte_flips: true            # Byte-level changes
    enable_arithmetic: true            # +1, -1, etc.
    enable_interesting_values: true    # 0, -1, MAX, MIN, etc.
    
    # Target regions for byte mutations
    mutate_opcodes: true               # Flip bits in opcode field
    mutate_immediates: true            # Fuzzy immediate values
    mutate_register_fields: false      # Usually keep registers valid
```

### Implementation
```cpp
unsigned char *ISAMutator::applyMutations(...) {
  // ... existing setup ...
  
  for (unsigned i = 0; i < nmuts; ++i) {
    if (cfg_.strategy == Strategy::MIXED_MODE) {
      // Decide: instruction-level or byte-level?
      if (Random::chancePct(cfg_.mixed_mode.instruction_level_probability)) {
        // Existing instruction-level mutation (REPLACE/INSERT/DELETE/DUPLICATE)
        unsigned strategy = Random::rnd32() % 4;
        // ... existing code ...
      } else {
        // Byte-level mutation
        size_t byte_offset = Random::range(cur_len);
        applyByteLevelMutation(out, byte_offset, cur_len);
      }
    } else {
      // Pure INSTRUCTION_LEVEL (existing code)
      unsigned strategy = Random::rnd32() % 4;
      // ...
    }
  }
}

void ISAMutator::applyByteLevelMutation(unsigned char* buf, size_t offset, size_t len) {
  if (cfg_.byte_mutations.enable_bitflips && Random::chancePct(40)) {
    // Flip random bit
    buf[offset] ^= (1 << Random::range(8));
  } else if (cfg_.byte_mutations.enable_arithmetic && Random::chancePct(30)) {
    // Arithmetic mutation
    buf[offset] += (Random::rnd32() % 3) - 1;  // -1, 0, or +1
  } else {
    // Random byte
    buf[offset] = Random::rnd32() & 0xFF;
  }
}
```

### Benefit Level: **MEDIUM**
- Helps find encoding bugs (invalid opcodes, reserved fields)
- Complements instruction-level mutations
- But: AFL++ already does byte-level mutations efficiently
- Only useful if you disable AFL++'s built-in mutations

---

## 6. Verbose Logging and Debugging

### Problem
No runtime logging of mutation decisions. Hard to debug why certain test cases are generated.

### Proposed Enhancement
```yaml
debug:
  verbose: true                      # Enable detailed logging
  log_mutations: true                # Log each mutation applied
  log_instruction_selection: false   # Log pickInstruction() calls (very verbose)
  log_encoding: false                # Log instruction encoding process
  
  log_file: "logs/mutator_verbose.log"
  
  # What to log
  log_events:
    - mutation_strategy               # REPLACE/INSERT/DELETE/DUPLICATE
    - size_adjustments                # Grow/shrink decisions
    - instruction_details             # Mnemonic, operands of generated instructions
    - legality_checks                 # Why instructions were rejected
```

### Implementation
```cpp
// Add to ISAMutator class:
void ISAMutator::logMutation(const char* strategy, size_t index, const InstructionSpec& spec) {
  if (!cfg_.debug.verbose || !cfg_.debug.log_mutations) return;
  
  std::fprintf(debug_log(), 
    "[MUTATION] %s at index %zu: %s (opcode=0x%x format=%s)\n",
    strategy, index, spec.mnemonic.c_str(), spec.opcode, spec.format.c_str());
}

// Use in applyMutations():
if (strategy == 0 && nwords > 0) {
  size_t idx = Random::range(static_cast<uint32_t>(nwords));
  auto& spec = pickInstruction();
  uint32_t encoded = encodeInstruction(spec);
  
  logMutation("REPLACE", idx, spec);  // ← Add logging
  
  writeWord(out, idx * word_bytes, encoded);
}
```

### Benefit Level: **MEDIUM**
- Essential for debugging mutator behavior
- Helps understand why AFL++ favors certain inputs
- But: Performance overhead when enabled
- Alternative: Use AFL_DEBUG flag and existing debug system

---

## 7. Multi-ISA Support in Single Binary

### Problem
Currently, mutator is compiled for one ISA at a time. Need to recompile to switch ISAs.

### Proposed Enhancement
```yaml
# Support multiple ISAs in single mutator binary
multi_isa:
  enabled: true
  
  # Define ISA variants
  isa_profiles:
    rv32i:
      schemas: ["rv32i"]
      register_count: 32
    
    rv32e:
      schemas: ["rv32e"]
      register_count: 16
    
    rv32im:
      schemas: ["rv32i", "rv32m"]
      register_count: 32
    
    rv64i:
      schemas: ["rv64i"]
      register_count: 32
      word_size: 8
  
  # Select at runtime based on input characteristics or environment variable
  auto_detect: true                   # Try to detect from input binary
  default_profile: "rv32im"
```

### Implementation
```cpp
class ISAMutator {
  std::map<std::string, isa::ISAConfig> isa_profiles_;
  std::string current_profile_;
  
  void initFromEnv() override {
    if (cfg_.multi_isa.enabled) {
      // Load all ISA profiles
      for (const auto& [name, profile] : cfg_.multi_isa.profiles) {
        isa_profiles_[name] = isa::load_isa_config(profile.schemas);
      }
      current_profile_ = cfg_.multi_isa.default_profile;
    } else {
      // Single ISA mode (existing behavior)
      isa_ = isa::load_isa_config(cfg_.isa_name);
    }
  }
  
  // Switch ISA profile based on input
  void selectProfile(const unsigned char* in, size_t in_len) {
    if (cfg_.multi_isa.auto_detect) {
      // Analyze input to detect ISA
      // Look for ISA-specific patterns (RVC compressed, RV64 instructions, etc.)
      current_profile_ = detectISA(in, in_len);
    }
  }
};
```

### Benefit Level: **LOW**
- Nice for convenience, but not essential
- Can just use separate mutator instances
- Complexity vs benefit tradeoff

---

## 8. Coverage-Guided Instruction Weighting

### Problem
Don't know which instructions are more effective at finding coverage.

### Proposed Enhancement
Track which instructions trigger new coverage, then favor them:

```yaml
strategy: ADAPTIVE

coverage_feedback:
  enabled: true
  
  # Track instruction effectiveness
  track_instruction_coverage: true
  
  # Adjust weights based on feedback
  weight_adjustment:
    increase_on_coverage: 1.2        # Multiply weight by 1.2 when finds coverage
    decrease_on_no_coverage: 0.95    # Multiply by 0.95 when doesn't find coverage
    min_weight: 0.1                  # Don't completely eliminate instructions
    max_weight: 10.0                 # Cap maximum weight
  
  # Save learned weights for reuse
  persist_weights: true
  weights_file: "logs/learned_weights.json"
```

### Implementation
```cpp
// Track instruction → coverage mapping
class ISAMutator {
  std::map<std::string, double> instruction_weights_;  // mnemonic → weight
  
  void updateInstructionWeights(const std::string& mnemonic, bool found_coverage) {
    if (found_coverage) {
      instruction_weights_[mnemonic] *= cfg_.coverage_feedback.weight_adjustment.increase_on_coverage;
    } else {
      instruction_weights_[mnemonic] *= cfg_.coverage_feedback.weight_adjustment.decrease_on_no_coverage;
    }
    // Clamp to min/max
    instruction_weights_[mnemonic] = std::clamp(
      instruction_weights_[mnemonic],
      cfg_.coverage_feedback.weight_adjustment.min_weight,
      cfg_.coverage_feedback.weight_adjustment.max_weight
    );
  }
  
  const isa::InstructionSpec& pickWeightedInstruction() const {
    // Use instruction_weights_ for weighted random selection
    // Higher weight = more likely to be selected
  }
};

// AFL++ callback to track coverage
extern "C" {
  void afl_custom_queue_new_entry(...) {
    // Parse filename_new_queue to get which instructions were in winning test case
    // Update weights for those instructions
  }
}
```

### Benefit Level: **HIGH**
- Learns which instructions matter for specific target
- Could dramatically improve fuzzing efficiency
- Research-grade feature - potential publication
- Requires significant implementation effort

---

## 9. Semantic-Aware Mutations

### Problem
Current mutations are syntactically valid but semantically random. No understanding of program logic.

### Proposed Enhancement
Generate mutations that preserve or intentionally violate semantic properties:

```yaml
semantic_mutations:
  enabled: false  # Experimental feature
  
  patterns:
    # Preserve data dependencies
    preserve_def_use_chains: false
    
    # Insert common patterns
    insert_loops: true                # for (i=0; i<N; i++)
    insert_conditional_branches: true # if (x > 0)
    insert_function_calls: true       # jal, jalr patterns
    
    # Violate specific properties (bug hunting)
    violate_alignment: true           # Misalign memory accesses
    violate_bounds: true              # Out-of-bounds array accesses
    violate_atomicity: true           # Break atomic sequences
```

### Implementation
```cpp
// Requires:
// 1. Basic program analysis (def-use chains, control flow)
// 2. Pattern recognition (detect loops, conditionals)
// 3. Pattern insertion (generate loop structures)

// Example: Insert loop pattern
void ISAMutator::insertLoop(unsigned char* buf, size_t& cur_len, size_t insert_pos) {
  // Generate:
  //   li   t0, 0        # Loop counter
  //   li   t1, 10       # Loop bound
  // loop:
  //   addi t0, t0, 1
  //   blt  t0, t1, loop
  
  std::vector<uint32_t> loop_pattern = {
    encodeInstruction("li", "t0", 0),
    encodeInstruction("li", "t1", 10),
    encodeInstruction("addi", "t0", "t0", 1),
    encodeInstruction("blt", "t0", "t1", -8)  // Branch back
  };
  
  // Insert pattern at insert_pos
  insertInstructions(buf, cur_len, insert_pos, loop_pattern);
}
```

### Benefit Level: **VERY HIGH (but very complex)**
- Could find deep semantic bugs
- Closer to program synthesis than fuzzing
- Requires significant research and implementation
- Potential for major publication/contribution

---

## 10. Differential Testing Support

### Problem
No support for differential testing (comparing RTL vs ISS vs formal model).

### Proposed Enhancement
Generate inputs specifically designed for differential testing:

```yaml
differential_testing:
  enabled: false
  
  # Generate test cases targeting common divergence points
  target_corner_cases:
    - edge_conditions        # MAX_INT, MIN_INT, zero, etc.
    - undefined_behavior     # What happens with reserved encodings?
    - implementation_defined # Unaligned access, atomic sequences
    
  # Add test oracles
  insert_assertions: true    # Check state after critical operations
  insert_markers: true       # Label test case sections for debugging
```

### Benefit Level: **HIGH**
- Essential for hardware verification
- Complements coverage-guided fuzzing
- Finds specification vs implementation bugs

---

## Priority Ranking

| Feature | Benefit | Complexity | Priority |
|---------|---------|------------|----------|
| Instruction Filtering (#4) | HIGH | LOW | **P0** |
| ADAPTIVE Strategy (#3) | HIGH | HIGH | **P1** |
| Coverage-Guided Weighting (#8) | HIGH | HIGH | **P1** |
| Verbose Logging (#6) | MEDIUM | LOW | **P2** |
| MIXED_MODE Enhancement (#5) | MEDIUM | MEDIUM | **P2** |
| Weighted Instruction Selection (#2) | MEDIUM | MEDIUM | **P3** |
| Configurable Parameters (#1) | LOW | LOW | **P3** |
| Differential Testing (#10) | HIGH | HIGH | **P3** |
| Semantic Mutations (#9) | VERY HIGH | VERY HIGH | **P4** |
| Multi-ISA Support (#7) | LOW | MEDIUM | **P4** |

---

## Implementation Phases

### Phase 1: Foundation (P0-P1)
- Instruction filtering and constraints
- Verbose logging for debugging
- Basic ADAPTIVE strategy skeleton

### Phase 2: Optimization (P1-P2)
- Full ADAPTIVE implementation with feedback
- Coverage-guided instruction weighting
- MIXED_MODE strategy

### Phase 3: Advanced (P3)
- Weighted instruction selection
- Differential testing support
- Configurable parameters

### Phase 4: Research (P4)
- Semantic-aware mutations
- Multi-ISA support
- Publication-worthy features

---

## References

- AFL++ Custom Mutator API: https://github.com/AFLplusplus/AFLplusplus/blob/stable/docs/custom_mutators.md
- RISC-V ISA Specification: https://riscv.org/specifications/
- Grammar-based Fuzzing: https://arxiv.org/abs/1812.04413
- ISA-aware Fuzzing Papers:
  - TheHuzz (ARM fuzzing): https://github.com/SoftSec-KAIST/TheHuzz
  - Agamotto (x86 hypervisor fuzzing): https://github.com/seclab-ucr/AGAMOTTO

---

**Last Updated:** November 10, 2025
**Status:** All features are ideas only - not implemented
**Current Focus:** Keep mutator simple, fast, correct
