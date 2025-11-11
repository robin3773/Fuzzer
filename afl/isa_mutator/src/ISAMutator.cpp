#include <fuzz/mutator/ISAMutator.hpp>

#include <hwfuzz/Debug.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fuzz/mutator/EncodeHelpers.hpp>
#include <fuzz/mutator/ExitStub.hpp>
#include <fuzz/mutator/LegalCheck.hpp>
#include <fuzz/mutator/Random.hpp>

namespace fuzz::mutator {

ISAMutator::ISAMutator() = default;

void ISAMutator::initFromEnv() {
  // Initialize debug system first (enables FunctionTracer)
  // Note: debug system initializes automatically on first use
  
  hwfuzz::debug::FunctionTracer tracer(__FILE__, "ISAMutator::initFromEnv");
  
  // Load config from environment (with automatic display when DEBUG=1)
  cfg_ = loadConfig();
  
  // Load ISA schema (required)
  if (cfg_.isa_name.empty()) {
    hwfuzz::debug::logError("No ISA name specified in config\n");
    std::exit(1);
  }
  
  // Schema directory is automatically resolved to PROJECT_ROOT/schemas
  isa_ = isa::load_isa_config(cfg_.isa_name);
  
  if (isa_.instructions.empty()) {
    hwfuzz::debug::logError("No instructions in schema for ISA '%s'\n", cfg_.isa_name.c_str());
    std::exit(1);
  }
  
  word_bytes_ = std::max<uint32_t>(1, isa_.base_width / 8);
  hwfuzz::debug::logInfo("Loaded ISA '%s': %zu instructions\n", isa_.isa_name.c_str(), isa_.instructions.size());
}

unsigned char *ISAMutator::mutateStream(unsigned char *in, size_t in_len,
                                           unsigned char *out_buf, size_t max_size) {
  hwfuzz::debug::FunctionTracer tracer(__FILE__, "ISAMutator::mutateStream");
  (void)out_buf;
  return applyMutations(in, in_len, nullptr, max_size);
}

/**
 * @brief Apply ISA-aware mutations to an instruction stream
 * 
 * This is the core mutation engine that transforms input instruction streams through
 * intelligent, schema-driven mutations. It operates in several phases:
 * 1. Memory allocation and size constraint enforcement
 * 2. Input processing and exit stub detection
 * 3. Dynamic payload size randomization
 * 4. Random mutation application (REPLACE/INSERT/DELETE/DUPLICATE)
 * 5. Exit stub appending for clean termination
 * 
 * @param in Input instruction buffer to mutate (unmodified by this function)
 * @param in_len Length of input buffer in bytes
 * @param out_buf Unused output buffer parameter (ignored, function allocates own buffer)
 * @param max_size Maximum size constraint for output (soft limit, may be exceeded if too small)
 * 
 * @return Pointer to newly allocated mutated buffer, or nullptr on allocation failure
 * 
 * @note Caller must free() the returned buffer
 * @note Function enforces minimum 16 instructions (64 bytes) and maximum 512 instructions (2048 bytes)
 * @note Exit stub (16 bytes) is always appended after mutations
 * 
 * @details
 * **Phase 1: Buffer Setup**
 * - Allocates output buffer with capacity for max payload (2048 bytes) + exit stub (16 bytes)
 * - Enforces minimum capacity of 80 bytes (16 instructions + exit stub)
 * - Ignores max_size if it's too small for valid programs
 * 
 * **Phase 2: Input Processing**
 * - Copies input to output buffer (up to capacity limits)
 * - Detects and excludes existing exit stub from mutation region
 * - Zero-initializes if no valid input provided
 * 
 * **Phase 3: Size Randomization**
 * - Dynamically adjusts program length between 16-512 instructions
 * - Growth bias: 50% chance to grow by 0-200 instructions
 * - Shrink bias: 30% chance to shrink by 0-100 instructions
 * - Keep size: 20% chance to maintain current size
 * - Always grows when at minimum (avoids staying at 16 instructions)
 * 
 * **Phase 4: Mutation Application**
 * - Applies 1-50 random mutations per call
 * - Four mutation strategies (25% probability each):
 *   - REPLACE (strategy 0): Overwrites existing instruction with new random instruction
 *   - INSERT (strategy 1): Inserts new instruction at random position, shifts subsequent instructions forward
 *   - DELETE (strategy 2): Removes instruction at random position, shifts subsequent instructions backward
 *   - DUPLICATE (strategy 3): Copies existing instruction to new position, shifts subsequent instructions forward
 * - Validates mutated instructions against ISA legality rules (skips illegal mutations)
 * 
 * **Phase 5: Finalization**
 * - Appends 4-instruction exit stub (16 bytes) at end of payload
 * - Updates last_len_ for AFL++ to query via last_out_len()
 * 
 * @example Basic Usage
 * @code
 * ISAMutator mutator;
 * mutator.initFromEnv();
 * 
 * // Input: 20 instructions (80 bytes for RV32)
 * unsigned char input[80] = { ... };
 * 
 * // Perform mutation
 * unsigned char* output = mutator.applyMutations(input, 80, nullptr, 4096);
 * if (output) {
 *   size_t output_size = mutator.last_out_len();
 *   // output_size might be: (20 ± mutations + 4 exit stub) * 4 bytes
 *   // Example: 25 payload + 4 stub = 29 instructions = 116 bytes
 *   
 *   // Use mutated buffer...
 *   execute_testcase(output, output_size);
 *   
 *   // Free allocated buffer
 *   std::free(output);
 * }
 * @endcode
 * 
 * @example Mutation Flow Example
 * @code
 * // Input: 20 instructions [I0, I1, I2, ..., I19]
 * 
 * // Phase 3: Size randomization decides to grow to 25 instructions
 * // - Adds 5 random instructions at end: [I0...I19, R0, R1, R2, R3, R4]
 * 
 * // Phase 4: Apply 10 mutations
 * // Mutation 1 (REPLACE): Replace I5 with new random instruction
 * //   Result: [I0, I1, I2, I3, I4, R5, I6, ..., R4]
 * 
 * // Mutation 2 (INSERT): Insert new instruction at position 3
 * //   Result: [I0, I1, I2, R6, I3, I4, R5, I6, ..., R4] (now 26 instructions)
 * 
 * // Mutation 3 (DELETE): Remove instruction at position 10
 * //   Result: [I0, I1, I2, R6, I3, I4, R5, I6, I7, I8, I10, ..., R4] (now 25 instructions)
 * 
 * // Mutation 4 (DUPLICATE): Copy instruction at position 5 to position 15
 * //   Result: [I0, I1, I2, R6, I3, I4, R5, I6, I7, I8, I10, ..., I4, ...] (now 26 instructions)
 * 
 * // ... 6 more mutations ...
 * 
 * // Phase 5: Append exit stub (4 instructions)
 * //   Final: [mutated 26 instructions, EXIT_0, EXIT_1, EXIT_2, EXIT_3]
 * //   Total: 30 instructions = 120 bytes
 * @endcode
 * 
 * @example Edge Cases
 * @code
 * // Case 1: Empty input
 * unsigned char* out = mutator.applyMutations(nullptr, 0, nullptr, 4096);
 * // Result: Generates 16-512 random instructions + exit stub
 * 
 * // Case 2: Input with existing exit stub
 * unsigned char input_with_stub[84];  // 20 instructions + stub
 * unsigned char* out = mutator.applyMutations(input_with_stub, 84, nullptr, 4096);
 * // Result: Detects stub, mutates only first 20 instructions, appends fresh stub
 * 
 * // Case 3: Too large input
 * unsigned char huge_input[3000];  // 750 instructions (exceeds max)
 * unsigned char* out = mutator.applyMutations(huge_input, 3000, nullptr, 4096);
 * // Result: Truncates to 512 instructions (2048 bytes), then mutates
 * 
 * // Case 4: Tiny max_size constraint
 * unsigned char input[80];
 * unsigned char* out = mutator.applyMutations(input, 80, nullptr, 32);  // max_size too small
 * // Result: Ignores max_size, uses default capacity (2064 bytes) to generate valid programs
 * @endcode
 * 
 * @warning Exit stub is ALWAYS appended - do not include it in input payload
 * @warning Returned buffer is malloc'd - caller MUST free() it to prevent memory leaks
 * @warning Function may return nullptr on allocation failure - always check return value
 * 
 * @see mutateStream() - Public wrapper that calls this function
 * @see pickInstruction() - Selects random instruction from ISA schema
 * @see encodeInstruction() - Encodes instruction spec to binary format
 * @see exit_stub::append_exit_stub() - Appends termination sequence
 */
unsigned char *ISAMutator::applyMutations(unsigned char *in, size_t in_len,
                                          unsigned char *, size_t max_size) {
  hwfuzz::debug::FunctionTracer tracer(__FILE__, "ISAMutator::applyMutations");
  
  size_t word_bytes = std::max<size_t>(1, word_bytes_);
  constexpr size_t exit_stub_bytes = exit_stub::EXIT_STUB_INSN_COUNT * 4;
  constexpr size_t min_payload_insns = 16;
  constexpr size_t max_payload_insns = 512;
  constexpr size_t min_payload_bytes = min_payload_insns * 4;
  constexpr size_t max_payload_bytes = max_payload_insns * 4;
  
  // Reserve space for exit stub + max payload
  // NOTE: Ignore max_size if it's too small - we need room to generate proper programs
  size_t required_min = min_payload_bytes + exit_stub_bytes;
  size_t cap = (max_size && max_size >= required_min) ? max_size : (max_payload_bytes + exit_stub_bytes);
  cap = std::max(cap, required_min);

  unsigned char *out = static_cast<unsigned char *>(std::malloc(cap));
  if (!out)
    return nullptr;

  // Copy input or zero-initialize (leave room for exit stub)
  size_t payload_cap = std::min(cap - exit_stub_bytes, max_payload_bytes);
  size_t cur_len = std::min(in_len, payload_cap);
  
  // Check if input already has exit stub at the end
  bool has_exit_stub = false;
  if (cur_len >= exit_stub_bytes && in) {
    has_exit_stub = exit_stub::has_exit_stub(in + cur_len - exit_stub_bytes);
  }
  
  // If input has exit stub, exclude it from mutation region
  if (has_exit_stub) {
    cur_len -= exit_stub_bytes;
  }
  
  // Enforce maximum payload size (512 instructions)
  if (cur_len > max_payload_bytes) {
    cur_len = max_payload_bytes;
  }
  
  if (cur_len && in) {
    std::memcpy(out, in, cur_len);
  } else {
    std::memset(out, 0, cap);
    cur_len = word_bytes;
  }

  // Randomize initial payload size between min and max
  size_t cur_insns = cur_len / word_bytes;
  size_t target_insns;
  
  if (cur_len < min_payload_bytes) {
    // Too small, pick random target size (full range 16-512)
    uint32_t rand_offset = Random::range(static_cast<uint32_t>(max_payload_insns - min_payload_insns + 1));
    target_insns = min_payload_insns + rand_offset;
  } else if (cur_insns == min_payload_insns) {
    // At minimum, ALWAYS grow (avoid staying at minimum)
    size_t max_growth = max_payload_insns - cur_insns;
    size_t growth = 1 + Random::range(static_cast<uint32_t>(max_growth));
    target_insns = cur_insns + growth;
  } else {
    // Randomly adjust current size
    uint32_t action = Random::range(10);  // 0-9 for finer control
    if (action < 5 && cur_insns < max_payload_insns) {
      // Grow: 50% chance
      size_t max_growth = std::min<size_t>(max_payload_insns - cur_insns, 200);
      size_t growth = Random::range(static_cast<uint32_t>(max_growth + 1));
      target_insns = cur_insns + growth;
    } else if (action >= 5 && action < 8 && cur_insns > min_payload_insns) {
      // Shrink: 30% chance
      size_t max_shrink = std::min<size_t>(cur_insns - min_payload_insns, 100);
      size_t shrink = Random::range(static_cast<uint32_t>(max_shrink + 1));
      target_insns = cur_insns - shrink;
    } else {
      // Keep current size: 20% chance
      target_insns = cur_insns;
    }
  }
  
  size_t target_bytes = std::min(target_insns * word_bytes, payload_cap);
  
  // Adjust to target size
  if (cur_len < target_bytes) {
    // Grow by adding random instructions
    while (cur_len < target_bytes && (cur_len + word_bytes) <= payload_cap) {
      uint32_t encoded = encodeInstruction(pickInstruction());
      writeWord(out, cur_len, encoded);
      cur_len += word_bytes;
    }
  } else if (cur_len > target_bytes) {
    // Shrink by truncating
    cur_len = target_bytes;
  }

  size_t nwords = std::max<size_t>(1, cur_len / word_bytes);
  unsigned nmuts = 1 + (Random::rnd32() % 50);  // Increased from 20 to 50
  
  // Apply mutations (ALL payload instructions can be mutated - stub appended after)
  for (unsigned i = 0; i < nmuts; ++i) {
    // Randomly choose mutation strategy (0=replace, 1=insert, 2=delete, 3=duplicate)
    unsigned strategy = Random::rnd32() % 4;
    
    if (strategy == 0 && nwords > 0) {
      // REPLACE: mutate existing instruction
      size_t idx = Random::range(static_cast<uint32_t>(nwords));
      
      uint32_t encoded = encodeInstruction(pickInstruction());
      
      if (!isa_.fields.empty() && !is_legal_instruction(encoded, isa_))
        continue;
      
      writeWord(out, idx * word_bytes, encoded);
      
    } else if (strategy == 1 && nwords < max_payload_insns && (cur_len + word_bytes + exit_stub_bytes <= cap)) {
      // INSERT: add new instruction at random position (enforce max 512 instructions)
      size_t idx = Random::range(static_cast<uint32_t>(nwords + 1));
      
      uint32_t encoded = encodeInstruction(pickInstruction());
      
      if (!isa_.fields.empty() && !is_legal_instruction(encoded, isa_))
        continue;
      
      // Shift instructions after insertion point
      std::memmove(out + (idx + 1) * word_bytes, out + idx * word_bytes, 
                   cur_len - idx * word_bytes);
      writeWord(out, idx * word_bytes, encoded);
      cur_len += word_bytes;
      nwords++;
      
    } else if (strategy == 2 && nwords > min_payload_insns) {
      // DELETE: remove instruction at random position (enforce min 16 instructions)
      size_t idx = Random::range(static_cast<uint32_t>(nwords));
      
      // Shift instructions after deletion point
      std::memmove(out + idx * word_bytes, out + (idx + 1) * word_bytes,
                   cur_len - (idx + 1) * word_bytes);
      cur_len -= word_bytes;
      nwords--;
      
    } else if (strategy == 3 && nwords < max_payload_insns && (cur_len + word_bytes + exit_stub_bytes <= cap) && nwords > 0) {
      // DUPLICATE: copy existing instruction to new position (enforce max 512 instructions)
      size_t src_idx = Random::range(static_cast<uint32_t>(nwords));
      size_t dst_idx = Random::range(static_cast<uint32_t>(nwords + 1));
      
      uint32_t insn = readWord(out, src_idx * word_bytes);
      
      // Shift instructions after insertion point
      std::memmove(out + (dst_idx + 1) * word_bytes, out + dst_idx * word_bytes,
                   cur_len - dst_idx * word_bytes);
      writeWord(out, dst_idx * word_bytes, insn);
      cur_len += word_bytes;
      nwords++;
    }
  }

  // Append exit stub after mutations
  exit_stub::append_exit_stub(out, cur_len);
  cur_len += exit_stub_bytes;

  last_len_ = cur_len;
  return out;
}

const isa::InstructionSpec &ISAMutator::pickInstruction() const {
  hwfuzz::debug::FunctionTracer tracer(__FILE__, "ISAMutator::pickInstruction");
  uint32_t idx = Random::range(static_cast<uint32_t>(isa_.instructions.size()));
  return isa_.instructions[idx];
}

uint32_t ISAMutator::encodeInstruction(const isa::InstructionSpec &spec) const {
  hwfuzz::debug::FunctionTracer tracer(__FILE__, "ISAMutator::encodeInstruction");
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
  hwfuzz::debug::FunctionTracer tracer(__FILE__, "ISAMutator::randomFieldValue");
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
  hwfuzz::debug::FunctionTracer tracer(__FILE__, "ISAMutator::applyField");
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

uint32_t ISAMutator::readWord(const unsigned char *buf, size_t offset) const {
  hwfuzz::debug::FunctionTracer tracer(__FILE__, "ISAMutator::readWord");
  if (word_bytes_ == 2)
    return internal::load_u16_le(buf, offset);
  else if (word_bytes_ == 4)
    return internal::load_u32_le(buf, offset);
  else {
    uint32_t word = 0;
    for (uint32_t i = 0; i < word_bytes_ && i < 4; ++i)
      word |= static_cast<uint32_t>(buf[offset + i]) << (8 * i);
    return word;
  }
}

void ISAMutator::writeWord(unsigned char *buf, size_t offset, uint32_t word) const {
  hwfuzz::debug::FunctionTracer tracer(__FILE__, "ISAMutator::writeWord");
  if (word_bytes_ == 2)
    internal::store_u16_le(buf, offset, static_cast<uint16_t>(word));
  else if (word_bytes_ == 4)
    internal::store_u32_le(buf, offset, word);
  else
    for (uint32_t i = 0; i < word_bytes_ && i < 4; ++i)
      buf[offset + i] = static_cast<unsigned char>((word >> (8 * i)) & 0xFF);
}

} // namespace fuzz::mutator
