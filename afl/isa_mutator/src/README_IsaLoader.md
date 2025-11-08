# IsaLoader.cpp - Comprehensive Documentation

## Overview

`IsaLoader.cpp` implements the YAML-based ISA (Instruction Set Architecture) schema loading system for the AFL++ custom mutator. It parses YAML files defining instruction formats, fields, and encodings, converting them into C++ data structures that the mutator uses to generate valid, architecture-aware test cases.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Data Structures](#data-structures)
3. [Function Reference](#function-reference)
4. [Data Flow](#data-flow)
5. [Examples](#examples)
6. [Error Handling](#error-handling)

---

## Architecture Overview

### Purpose

The ISA loader enables **schema-driven fuzzing** by:
- Loading instruction definitions from human-readable YAML files
- Supporting multi-file schemas with includes and anchors
- Handling complex field encodings (multi-segment immediates, split fields)
- Providing complete ISA metadata for intelligent mutation

### Key Concepts

**Field**: A logical unit within an instruction (e.g., opcode, register, immediate)
- May be split into multiple segments across the instruction word
- Example: RISC-V S-type immediate split into `imm[11:5]` and `imm[4:0]`

**Segment**: A contiguous chunk of bits in the instruction word
- `word_lsb`: Bit position in instruction word (0-31 for 32-bit)
- `width`: Number of bits
- `value_lsb`: Bit position in the logical value

**Format**: Instruction encoding pattern (e.g., R-type, I-type, S-type)
- Defines ordered list of fields
- Specifies instruction width (16 for compressed, 32 for base)

**Instruction**: Specific operation with fixed and variable fields
- Fixed fields: opcode, funct3, funct7 (identify the instruction)
- Variable fields: rs1, rs2, rd, imm (mutated during fuzzing)

---

## Data Structures

### SchemaLocator (Internal)

```cpp
struct SchemaLocator {
  std::string root_dir;   // Schema directory (PROJECT_ROOT/schemas)
  std::string isa_name;   // ISA identifier (e.g., "rv32im")
  std::string map_path;   // ISA mapping file ("isa_map.yaml")
};
```

**Purpose**: Bundles information needed to locate and load schema files.

---

## Function Reference

### 1. resolve_schema_sources()

```cpp
std::vector<fs::path> resolve_schema_sources(const SchemaLocator &locator)
```

**Purpose**: Resolves which YAML files need to be loaded for a given ISA.

**Parameters**:
- `locator`: Contains ISA name and schema directory path

**Algorithm**:
1. Check if ISA name is provided
2. Construct path to `isa_map.yaml`
3. Look up ISA name in map to get list of schema files
4. For each schema file:
   - Convert relative path to absolute
   - Normalize path (resolve `.` and `..`)
   - Verify file exists
5. Collect dependencies recursively (if schemas include other schemas)
6. Return ordered list of schema files

**Returns**: 
- Vector of file paths in dependency order
- Empty vector on error (logs error message)

**Example**:

Input: `isa_name = "rv32im"`

isa_map.yaml contains:
```yaml
rv32im:
  - riscv/rv32_base.yaml
  - riscv/rv32i.yaml
  - riscv/rv32m.yaml
```

Output: `["/path/to/schemas/riscv/rv32_base.yaml", "/path/to/schemas/riscv/rv32i.yaml", "/path/to/schemas/riscv/rv32m.yaml"]`

**Error Cases**:
- Empty ISA name → logs error, returns `{}`
- Schema file not found → logs error, returns `{}`
- No schema files found → logs error, returns `{}`

---

### 2. compute_field_width()

```cpp
uint32_t compute_field_width(const std::vector<FieldSegment> &segments)
```

**Purpose**: Calculates total bit width of a field from its segments.

**Parameters**:
- `segments`: Vector of field segments

**Algorithm**:
Uses `std::accumulate` to find maximum extent:
```
for each segment:
  extent = segment.value_lsb + segment.width
  max_extent = max(max_extent, extent)
return max_extent
```

**Returns**: Total field width in bits

**Example 1 - Single Segment**:
```cpp
// rd field: bits [11:7] (5 bits)
FieldSegment seg = {.word_lsb=7, .width=5, .value_lsb=0};
compute_field_width({seg}) → 5
```

**Example 2 - Multi-Segment (S-type immediate)**:
```cpp
// imm[11:5] at bits [31:25], imm[4:0] at bits [11:7]
FieldSegment seg1 = {.word_lsb=25, .width=7, .value_lsb=5};  // bits 11-5 of value
FieldSegment seg2 = {.word_lsb=7,  .width=5, .value_lsb=0};  // bits 4-0 of value
compute_field_width({seg1, seg2}) → 12  // (5+7=12 bits total)
```

**Why Multiple Segments?**:
RISC-V splits immediates to keep register fields in consistent positions:
```
S-type: [imm[11:5] | rs2 | rs1 | funct3 | imm[4:0] | opcode]
        31      25   24 20  19 15  14  12  11     7   6    0
```

---

### 3. parse_segment()

```cpp
FieldSegment parse_segment(const YAML::Node &node, uint32_t default_value_lsb)
```

**Purpose**: Parses a field segment definition from YAML.

**Parameters**:
- `node`: YAML node containing segment definition
- `default_value_lsb`: Starting bit position in value (for sequential segments)

**Supported YAML Formats**:

**Format 1: Sequence [lsb, msb]**
```yaml
[7, 11]  # bits 7-11 in instruction word
```

**Format 2: Map with lsb/width**
```yaml
lsb: 7
width: 5
value_lsb: 0  # optional
```

**Format 3: Map with bits array**
```yaml
bits: [7, 11]
value_lsb: 0  # optional
```

**Algorithm**:
1. Check node type (sequence or map)
2. If sequence: extract [lsb, msb], calculate width
3. If map: read lsb/width or bits array
4. Validate msb >= lsb
5. Apply default_value_lsb if not specified
6. Return segment structure

**Returns**: `FieldSegment` with word position and value position

**Example 1 - Simple Segment**:
```yaml
# Input YAML
rd: [7, 11]

# Parsed result
FieldSegment {
  word_lsb: 7,    // starts at bit 7
  width: 5,       // 11-7+1 = 5 bits
  value_lsb: 0    // bits 0-4 of value
}
```

**Example 2 - Split Immediate**:
```yaml
# Input YAML (S-type immediate high bits)
segments:
  - { lsb: 25, width: 7, value_lsb: 5 }

# Parsed result
FieldSegment {
  word_lsb: 25,   // starts at bit 25 in instruction
  width: 7,       // 7 bits wide
  value_lsb: 5    // represents bits 5-11 of immediate value
}
```

**Error Cases**:
- Sequence not [lsb, msb] → logs error, returns segment with width=0
- msb < lsb → logs error, returns segment with width=0
- Not sequence or map → logs error, returns segment with width=0
- Missing width → logs error, returns segment with width=0

---

### 4. deduce_field_kind()

```cpp
FieldKind deduce_field_kind(const std::string &raw)
```

**Purpose**: Infers field type from its name using heuristics.

**Parameters**:
- `raw`: Field name or type string

**Algorithm**:
1. Convert string to lowercase
2. Check for keywords in order of specificity:
   - "opcode" → `FieldKind::Opcode`
   - "funct" or "flag" → `FieldKind::Enum`
   - "imm" → `FieldKind::Immediate`
   - "pred" → `FieldKind::Predicate`
   - "mem" → `FieldKind::Memory`
   - "csr" → `FieldKind::Enum`
   - "freg" or "fp_reg" → `FieldKind::Floating`
   - "reg", "rs", "rd", "rt" → `FieldKind::Register`
   - "aq_rl" → `FieldKind::Enum`
3. Default: `FieldKind::Unknown`

**Returns**: `FieldKind` enum value

**Examples**:
```cpp
deduce_field_kind("opcode") → FieldKind::Opcode
deduce_field_kind("rs1") → FieldKind::Register
deduce_field_kind("imm12") → FieldKind::Immediate
deduce_field_kind("funct3") → FieldKind::Enum
deduce_field_kind("rd") → FieldKind::Register
deduce_field_kind("custom_field") → FieldKind::Unknown
```

**Why Important?**:
The mutator uses field kind to apply appropriate mutation strategies:
- **Register**: Choose from valid register set (x0-x31)
- **Immediate**: Generate edge values, random values, small values
- **Opcode**: Keep fixed (don't mutate)
- **Enum**: Choose from valid function codes

---

### 5. parse_field()

```cpp
FieldEncoding parse_field(const std::string &name, const YAML::Node &node)
```

**Purpose**: Parses a complete field definition from YAML.

**Parameters**:
- `name`: Field name
- `node`: YAML node containing field specification

**Algorithm**:
1. Create `FieldEncoding` structure
2. Parse basic properties (signed, width, type)
3. Deduce field kind from type string
4. Parse segments using one of three methods:
   - **segments**: Explicit list of segment definitions
   - **bits**: Single segment or list of segments
   - **lsb + width**: Single contiguous segment
5. Compute field width from segments if not specified
6. Deduce field kind from name if still unknown
7. Return complete field encoding

**Lambda Function: append_segments**:
```cpp
auto append_segments = [&](const YAML::Node &source) {
  // Validates source is a sequence
  // Parses each segment with incremental value_lsb
  // Appends to encoding.segments vector
};
```

**Returns**: `FieldEncoding` with all properties populated

**Example 1 - Simple Register Field**:
```yaml
# Input YAML
rd:
  type: register
  bits: [7, 11]

# Parsed result
FieldEncoding {
  name: "rd",
  kind: FieldKind::Register,
  width: 5,
  is_signed: false,
  raw_type: "register",
  segments: [
    {word_lsb: 7, width: 5, value_lsb: 0}
  ]
}
```

**Example 2 - Multi-Segment Immediate**:
```yaml
# Input YAML (S-type immediate)
imm:
  type: immediate
  signed: true
  segments:
    - { lsb: 25, width: 7, value_lsb: 5 }  # imm[11:5]
    - { lsb: 7,  width: 5, value_lsb: 0 }  # imm[4:0]

# Parsed result
FieldEncoding {
  name: "imm",
  kind: FieldKind::Immediate,
  width: 12,  // computed from segments
  is_signed: true,
  raw_type: "immediate",
  segments: [
    {word_lsb: 25, width: 7, value_lsb: 5},
    {word_lsb: 7,  width: 5, value_lsb: 0}
  ]
}
```

**Example 3 - Compact Format**:
```yaml
# Input YAML
funct3:
  lsb: 12
  width: 3

# Parsed result
FieldEncoding {
  name: "funct3",
  kind: FieldKind::Enum,  // deduced from name
  width: 3,
  is_signed: false,
  raw_type: "",
  segments: [
    {word_lsb: 12, width: 3, value_lsb: 0}
  ]
}
```

**Error Cases**:
- Segments not a sequence → logs error, skips append
- Missing width/segments → logs error, continues with width=0

---

### 6. segments_equal()

```cpp
bool segments_equal(const std::vector<FieldSegment> &lhs,
                    const std::vector<FieldSegment> &rhs)
```

**Purpose**: Checks if two segment lists are identical.

**Parameters**:
- `lhs`: First segment list
- `rhs`: Second segment list

**Algorithm**:
1. Check sizes match
2. Compare each segment pairwise:
   - `word_lsb` must match
   - `width` must match
   - `value_lsb` must match
3. Return true if all match

**Returns**: `true` if segments are identical, `false` otherwise

**Example**:
```cpp
// Segment lists for S-type immediate
std::vector<FieldSegment> seg1 = {
  {.word_lsb=25, .width=7, .value_lsb=5},
  {.word_lsb=7,  .width=5, .value_lsb=0}
};

std::vector<FieldSegment> seg2 = {
  {.word_lsb=25, .width=7, .value_lsb=5},
  {.word_lsb=7,  .width=5, .value_lsb=0}
};

segments_equal(seg1, seg2) → true

std::vector<FieldSegment> seg3 = {
  {.word_lsb=25, .width=7, .value_lsb=0},  // Different value_lsb
  {.word_lsb=7,  .width=5, .value_lsb=7}
};

segments_equal(seg1, seg3) → false
```

---

### 7. ensure_field()

```cpp
void ensure_field(std::unordered_map<std::string, FieldEncoding> &fields,
                  const FieldEncoding &candidate)
```

**Purpose**: Merges field definitions, filling in missing information from multiple schema files.

**Parameters**:
- `fields`: Map of existing field definitions
- `candidate`: New field definition to merge

**Algorithm**:
1. Try to insert candidate into fields map
2. If inserted (new field): done
3. If exists: merge properties
   - Copy segments if existing has none
   - Copy width if existing is zero
   - Set signed flag if candidate is signed
4. Validate segments match if both defined

**Returns**: void (modifies `fields` map in-place)

**Why Needed?**:
Multiple schema files may partially define the same field:

```yaml
# rv32_base.yaml
fields:
  rd:
    width: 5

# rv32i.yaml
fields:
  rd:
    type: register
    bits: [7, 11]
```

Result after merging:
```cpp
FieldEncoding {
  name: "rd",
  width: 5,           // from base
  kind: Register,     // from rv32i
  segments: [...]     // from rv32i
}
```

**Example Merge Sequence**:
```cpp
// Step 1: Load rv32_base.yaml
fields["rd"] = {name: "rd", width: 5, segments: []};

// Step 2: Load rv32i.yaml
candidate = {name: "rd", width: 5, segments: [{7, 5, 0}]};
ensure_field(fields, candidate);
// Result: fields["rd"].segments now populated

// Step 3: Load another file with signed flag
candidate = {name: "rd", is_signed: false, ...};
ensure_field(fields, candidate);
// Result: no change (rd is register, not signed)
```

---

### 8. parse_format()

```cpp
FormatSpec parse_format(const std::string &name,
                        const YAML::Node &node,
                        std::unordered_map<std::string, FieldEncoding> &fields)
```

**Purpose**: Parses instruction format definition (R-type, I-type, etc.).

**Parameters**:
- `name`: Format name
- `node`: YAML node with format specification
- `fields`: Field definitions map (updated with inline fields)

**Algorithm**:
1. Create `FormatSpec` structure
2. Parse width (default from ISA base_width)
3. Validate fields list exists and is sequence
4. For each field entry:
   - If scalar: add field name to list
   - If map: parse inline field definition, add to fields map
5. Return complete format specification

**Returns**: `FormatSpec` with format name, width, and field list

**Example 1 - R-type Format (Register-Register)**:
```yaml
# Input YAML
R:
  width: 32
  fields:
    - opcode
    - rd
    - funct3
    - rs1
    - rs2
    - funct7

# Parsed result
FormatSpec {
  name: "R",
  width: 32,
  fields: ["opcode", "rd", "funct3", "rs1", "rs2", "funct7"]
}

# Bit layout:
# [31:25] funct7 | [24:20] rs2 | [19:15] rs1 | [14:12] funct3 | [11:7] rd | [6:0] opcode
```

**Example 2 - I-type Format (Immediate)**:
```yaml
# Input YAML
I:
  width: 32
  fields:
    - opcode
    - rd
    - funct3
    - rs1
    - imm12

# Parsed result
FormatSpec {
  name: "I",
  width: 32,
  fields: ["opcode", "rd", "funct3", "rs1", "imm12"]
}

# Bit layout:
# [31:20] imm12 | [19:15] rs1 | [14:12] funct3 | [11:7] rd | [6:0] opcode
```

**Example 3 - Inline Field Definition**:
```yaml
# Input YAML
CustomFormat:
  width: 32
  fields:
    - opcode
    - name: custom_field
      lsb: 7
      width: 5
    - rs1

# Parsed result
FormatSpec {
  name: "CustomFormat",
  width: 32,
  fields: ["opcode", "custom_field", "rs1"]
}
# Also adds custom_field to fields map
```

**Error Cases**:
- Missing fields → logs error, returns empty format
- Fields not sequence → logs error, returns empty format
- Invalid field entry → logs error, skips entry
- Missing inline field name → logs error, skips entry

---

### 9. parse_instruction()

```cpp
InstructionSpec parse_instruction(const std::string &name, const YAML::Node &node)
```

**Purpose**: Parses individual instruction specification.

**Parameters**:
- `name`: Instruction mnemonic (e.g., "add", "lw")
- `node`: YAML node with instruction specification

**Algorithm**:
1. Create `InstructionSpec` structure
2. Parse format reference (required)
3. Parse fixed fields from "fixed" map
4. Parse top-level fixed field values (backward compatibility)
5. Skip metadata keys (description, comment, notes, tags)
6. Return complete instruction specification

**Returns**: `InstructionSpec` with name, format, and fixed fields

**Example 1 - ADD Instruction (R-type)**:
```yaml
# Input YAML
add:
  format: R
  opcode: 0b0110011
  funct3: 0b000
  funct7: 0b0000000

# Parsed result
InstructionSpec {
  name: "add",
  format: "R",
  fixed_fields: {
    {"opcode", 0x33},
    {"funct3", 0x0},
    {"funct7", 0x0}
  }
}

# Variable fields (filled during mutation): rd, rs1, rs2
```

**Example 2 - LW Instruction (I-type)**:
```yaml
# Input YAML
lw:
  format: I
  opcode: 0b0000011
  funct3: 0b010

# Parsed result
InstructionSpec {
  name: "lw",
  format: "I",
  fixed_fields: {
    {"opcode", 0x3},
    {"funct3", 0x2}
  }
}

# Variable fields: rd, rs1, imm12
```

**Example 3 - Using "fixed" Map**:
```yaml
# Input YAML
sub:
  format: R
  fixed:
    opcode: 0b0110011
    funct3: 0b000
    funct7: 0b0100000

# Parsed result
InstructionSpec {
  name: "sub",
  format: "R",
  fixed_fields: {
    {"opcode", 0x33},
    {"funct3", 0x0},
    {"funct7", 0x20}  # Note: different funct7 from ADD
  }
}
```

**Skipped Keys**:
```yaml
add:
  format: R
  opcode: 0b0110011
  description: "Add two registers"  # Skipped
  comment: "rd = rs1 + rs2"          # Skipped
  tags: [arithmetic, basic]          # Skipped
  weight: 10                          # Skipped
```

**Error Cases**:
- Missing format → logs error, returns empty spec

---

### 10. load_isa_config_impl()

```cpp
ISAConfig load_isa_config_impl(const SchemaLocator &locator)
```

**Purpose**: Main implementation of schema loading - orchestrates entire process.

**Parameters**:
- `locator`: Schema location information (dir, ISA name, map path)

**Algorithm**:

**Phase 1: Resolve Schema Files**
```cpp
auto sources = resolve_schema_sources(locator);
if (sources.empty()) return ISAConfig{};
```

**Phase 2: Load and Merge YAML Files**
```cpp
for each source file:
  1. Read file content as string
  2. Build anchor context (YAML references from previous files)
  3. Combine context + content
  4. Parse YAML into node tree
  5. Extract anchor blocks for next file
  6. Count instructions (for logging)
  7. Merge into accumulated schema
```

**Phase 3: Parse Merged Schema**
```cpp
1. Extract ISA name, endianness, default_pc from meta section
2. Extract register_count, mutation hints from defaults section
3. Parse all field definitions → isa.fields
4. Parse all format definitions → isa.formats
5. Parse all instruction definitions → isa.instructions
```

**Phase 4: Apply Defaults**
```cpp
1. Calculate maximum format width
2. Set base_width (default: 32)
3. Set register_count (default: 32)
4. Fill missing format widths
```

**Returns**: Complete `ISAConfig` or empty config on error

**YAML Anchor System Example**:

**File 1: rv32_base.yaml**
```yaml
common_opcodes: &opcodes
  OP: 0b0110011
  OP_IMM: 0b0010011
  LOAD: 0b0000011
  STORE: 0b0100011
```

**File 2: rv32i.yaml**
```yaml
<<: *opcodes  # Includes common_opcodes

instructions:
  add:
    format: R
    opcode: *OP  # References common_opcodes.OP
```

**Phase 2 Details - Anchor Library**:
```cpp
// After loading rv32_base.yaml
anchor_library = [
  {"opcodes", "OP: 0b0110011\nOP_IMM: 0b0010011\n..."}
];

// Before loading rv32i.yaml, prepend:
context = "__anchors:\n  opcodes: &opcodes\n    OP: 0b0110011\n...";
combined = context + rv32i_content;
// Now rv32i.yaml can reference &opcodes
```

**Example Complete Flow**:

Input: `isa_name = "rv32im"`

```
1. Resolve sources:
   → [rv32_base.yaml, rv32i.yaml, rv32m.yaml]

2. Load rv32_base.yaml:
   → Parse meta: {isa_name: "RV32", endianness: "little"}
   → Parse fields: {opcode, rd, rs1, rs2, funct3, funct7}
   → Parse formats: {R, I, S, B, U, J}
   → Extract anchors: {common_rules, field_defs}

3. Load rv32i.yaml:
   → Apply anchors from rv32_base
   → Parse instructions: {add, sub, addi, lw, sw, ...}
   → Merge into accumulated schema

4. Load rv32m.yaml:
   → Apply anchors from previous files
   → Parse instructions: {mul, div, rem, ...}
   → Merge into accumulated schema

5. Parse merged schema:
   → isa.isa_name = "rv32im"
   → isa.base_width = 32
   → isa.register_count = 32
   → isa.fields = {opcode, rd, rs1, rs2, ...}
   → isa.formats = {R, I, S, B, U, J}
   → isa.instructions = [add, sub, mul, div, lw, sw, ...]

6. Return ISAConfig
```

**Error Handling**:
- No schema files → logs error, returns empty config
- YAML parse error → logs error, skips file, continues
- Empty merged schema → logs error, returns empty config

---

### 11. load_isa_config() [PUBLIC API]

```cpp
ISAConfig load_isa_config(const std::string &isa_name)
```

**Purpose**: Public API to load ISA configuration by name.

**Parameters**:
- `isa_name`: ISA identifier (e.g., "rv32im", "rv64gc")

**Algorithm**:
1. Compute schema directory from source file location:
   ```cpp
   __FILE__ → .../afl/isa_mutator/src/IsaLoader.cpp
   parent   → .../afl/isa_mutator/src/
   parent   → .../afl/isa_mutator/
   parent   → .../afl/
   parent   → .../  (PROJECT_ROOT)
   schema   → .../schemas/
   ```

2. Log schema directory path
3. Create SchemaLocator structure
4. Call implementation function
5. Return result

**Returns**: Complete `ISAConfig` structure

**Usage Example**:
```cpp
#include <fuzz/isa/IsaLoader.hpp>

// Load RV32I base instruction set
auto config = fuzz::isa::load_isa_config("rv32i");

std::cout << "ISA: " << config.isa_name << std::endl;
std::cout << "Instructions: " << config.instructions.size() << std::endl;

// Access instruction details
for (const auto& insn : config.instructions) {
  if (insn.name == "add") {
    std::cout << "ADD format: " << insn.format << std::endl;
    std::cout << "ADD opcode: 0x" << std::hex 
              << insn.fixed_fields.at("opcode") << std::endl;
  }
}

// Access field encoding
const auto& rd_field = config.fields.at("rd");
std::cout << "rd width: " << rd_field.width << " bits" << std::endl;
```

**Path Computation Example**:
```
Source file: /home/robin/HAVEN/Fuzz/afl/isa_mutator/src/IsaLoader.cpp
                                      └─ parent ─┘
                               └──── parent ────┘
                        └─────── parent ──────┘
                 └────────── parent ───────────┘
Result: /home/robin/HAVEN/Fuzz/schemas/
```

---

## Data Flow

### High-Level Flow

```
┌─────────────────────────────────────────────────────────┐
│  load_isa_config("rv32im")                             │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│  1. Compute schema directory                           │
│     __FILE__ → PROJECT_ROOT/schemas                    │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│  2. load_isa_config_impl(locator)                      │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│  3. resolve_schema_sources(locator)                    │
│     - Read isa_map.yaml                                │
│     - Get [rv32_base.yaml, rv32i.yaml, rv32m.yaml]    │
│     - Resolve dependencies                             │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│  4. For each schema file:                              │
│     ┌──────────────────────────────────────────────┐  │
│     │ Load YAML                                     │  │
│     │   ↓                                           │  │
│     │ Build anchor context                          │  │
│     │   ↓                                           │  │
│     │ Parse YAML::Node                              │  │
│     │   ↓                                           │  │
│     │ Extract anchors                               │  │
│     │   ↓                                           │  │
│     │ Merge into accumulated schema                 │  │
│     └──────────────────────────────────────────────┘  │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│  5. Parse merged schema:                               │
│     ┌──────────────────────────────────────────────┐  │
│     │ Parse fields → parse_field()                  │  │
│     │   ↓ parse_segment()                           │  │
│     │   ↓ deduce_field_kind()                       │  │
│     │   ↓ compute_field_width()                     │  │
│     ├──────────────────────────────────────────────┤  │
│     │ Parse formats → parse_format()                │  │
│     │   ↓ ensure_field()                            │  │
│     ├──────────────────────────────────────────────┤  │
│     │ Parse instructions → parse_instruction()      │  │
│     └──────────────────────────────────────────────┘  │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│  6. Apply defaults:                                     │
│     - base_width = 32                                  │
│     - register_count = 32                              │
│     - Fill missing format widths                       │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│  7. Return ISAConfig                                    │
│     {                                                   │
│       isa_name: "rv32im",                              │
│       base_width: 32,                                  │
│       register_count: 32,                              │
│       fields: {...},                                   │
│       formats: {...},                                  │
│       instructions: [...]                              │
│     }                                                   │
└─────────────────────────────────────────────────────────┘
```

### Detailed Field Parsing Flow

```
parse_field("imm", yaml_node)
  │
  ├─ Read basic properties
  │  ├─ signed: true
  │  ├─ width: (not specified)
  │  └─ type: "immediate"
  │
  ├─ deduce_field_kind("immediate")
  │  └─→ FieldKind::Immediate
  │
  ├─ Parse segments
  │  ├─ node["segments"] exists
  │  ├─ Call append_segments()
  │  │  │
  │  │  ├─ Segment 1: {lsb: 25, width: 7, value_lsb: 5}
  │  │  │  └─→ parse_segment() → FieldSegment{25, 7, 5}
  │  │  │
  │  │  └─ Segment 2: {lsb: 7, width: 5, value_lsb: 0}
  │  │     └─→ parse_segment() → FieldSegment{7, 5, 0}
  │  │
  │  └─ encoding.segments = [{25,7,5}, {7,5,0}]
  │
  ├─ compute_field_width([{25,7,5}, {7,5,0}])
  │  └─→ width = 12
  │
  └─ Return FieldEncoding {
       name: "imm",
       kind: Immediate,
       width: 12,
       is_signed: true,
       segments: [{25,7,5}, {7,5,0}]
     }
```

### Instruction Parsing Flow

```
parse_instruction("add", yaml_node)
  │
  ├─ Check format exists
  │  └─→ format: "R"
  │
  ├─ Parse fixed fields from "fixed" map
  │  └─→ None in this case
  │
  ├─ Parse top-level fixed fields
  │  ├─ opcode: 0b0110011 → 0x33
  │  ├─ funct3: 0b000 → 0x0
  │  └─ funct7: 0b0000000 → 0x0
  │
  ├─ Skip metadata keys
  │  ├─ "description" → skipped
  │  └─ "comment" → skipped
  │
  └─ Return InstructionSpec {
       name: "add",
       format: "R",
       fixed_fields: {
         opcode: 0x33,
         funct3: 0x0,
         funct7: 0x0
       }
     }
```

---

## Examples

### Example 1: Loading RV32I

```cpp
auto config = load_isa_config("rv32i");

// ISAConfig contents:
config.isa_name → "rv32i"
config.base_width → 32
config.register_count → 32

// Fields:
config.fields["opcode"] → {name: "opcode", width: 7, kind: Opcode, ...}
config.fields["rd"] → {name: "rd", width: 5, kind: Register, ...}
config.fields["rs1"] → {name: "rs1", width: 5, kind: Register, ...}
config.fields["imm12"] → {name: "imm12", width: 12, kind: Immediate, ...}

// Formats:
config.formats["R"] → {name: "R", width: 32, fields: ["opcode", "rd", "funct3", "rs1", "rs2", "funct7"]}
config.formats["I"] → {name: "I", width: 32, fields: ["opcode", "rd", "funct3", "rs1", "imm12"]}

// Instructions:
config.instructions[0] → {name: "add", format: "R", fixed_fields: {opcode: 0x33, funct3: 0x0, funct7: 0x0}}
config.instructions[1] → {name: "addi", format: "I", fixed_fields: {opcode: 0x13, funct3: 0x0}}
```

### Example 2: Multi-Segment Immediate (S-type)

**YAML Definition**:
```yaml
imm:
  type: immediate
  signed: true
  segments:
    - { lsb: 25, width: 7, value_lsb: 5 }
    - { lsb: 7,  width: 5, value_lsb: 0 }
```

**Parsed Result**:
```cpp
FieldEncoding imm = {
  .name = "imm",
  .kind = FieldKind::Immediate,
  .width = 12,
  .is_signed = true,
  .segments = {
    {.word_lsb = 25, .width = 7, .value_lsb = 5},
    {.word_lsb = 7,  .width = 5, .value_lsb = 0}
  }
};
```

**How Mutator Uses This**:

To generate immediate value `0xABC` (binary: `1010_1011_1100`):

```
Immediate value: 0xABC = 0b101010111100
                         │└─────┘└───┘
                         │  11-5  4-0
                         sign bit

Segment 1: bits [11:5] = 0b1010101
  → Place at instruction bits [31:25]

Segment 2: bits [4:0] = 0b11100
  → Place at instruction bits [11:7]

Final instruction (S-type SW):
  [31:25] = 0b1010101  (imm[11:5])
  [24:20] = rs2 value
  [19:15] = rs1 value
  [14:12] = 0b010      (funct3)
  [11:7]  = 0b11100    (imm[4:0])
  [6:0]   = 0b0100011  (STORE opcode)
```

### Example 3: Compressed Instructions (RV32C)

**YAML Definition**:
```yaml
# 16-bit compressed format
C_CR:
  width: 16
  fields:
    - opcode    # [1:0]
    - rs2       # [6:2]
    - rd_rs1    # [11:7]
    - funct4    # [15:12]

# Compressed instruction
C.ADD:
  format: C_CR
  opcode: 0b10
  funct4: 0b1001
```

**Parsed Result**:
```cpp
FormatSpec c_cr = {
  .name = "C_CR",
  .width = 16,  // Only 16 bits!
  .fields = {"opcode", "rs2", "rd_rs1", "funct4"}
};

InstructionSpec c_add = {
  .name = "C.ADD",
  .format = "C_CR",
  .fixed_fields = {
    {"opcode", 0b10},
    {"funct4", 0b1001}
  }
};
```

---

## Error Handling

All functions use **debug logging** instead of throwing exceptions:

### Error Categories

**1. File Not Found**
```cpp
resolve_schema_sources(locator)
  → Schema include 'rv32x.yaml' not found
  → fuzz::debug::logError("Schema include 'rv32x.yaml' referenced by ISA map not found\n")
  → Returns {}
```

**2. Parse Errors**
```cpp
load_isa_config_impl(locator)
  → Failed to parse schema file 'rv32i.yaml': invalid YAML syntax
  → fuzz::debug::logError("Failed to parse schema file 'rv32i.yaml': %s\n", ex.what())
  → Continues with next file
```

**3. Invalid YAML Structure**
```cpp
parse_segment(node, 0)
  → Segment sequence must contain [lsb, msb]
  → fuzz::debug::logError("Segment sequence must contain [lsb, msb]\n")
  → Returns segment with width=0

parse_format(name, node, fields)
  → Format 'R' missing fields
  → fuzz::debug::logError("Format 'R' missing fields\n")
  → Returns empty format
```

**4. Empty Results**
```cpp
load_isa_config_impl(locator)
  → Merged schema for ISA 'rv32im' is empty
  → fuzz::debug::logError("Merged schema for ISA 'rv32im' is empty\n")
  → Returns ISAConfig{}
```

### Graceful Degradation

Instead of crashing:
- **Missing files**: Skip, continue with available files
- **Parse errors**: Log error, skip file
- **Invalid fields**: Log error, use defaults
- **Empty results**: Return empty config, caller checks `instructions.empty()`

### Error Detection in Caller

```cpp
auto config = load_isa_config("rv32im");

if (config.instructions.empty()) {
  std::fprintf(stderr, "ERROR: No instructions loaded for rv32im\n");
  std::exit(1);
}

// Proceed with fuzzing...
```

### Debug Output

When `DEBUG=1`:
```
[INFO] Using schema directory: /home/robin/HAVEN/Fuzz/schemas
[INFO] Using ISA Map: /home/robin/HAVEN/Fuzz/schemas/isa_map.yaml
[INFO] Loading rv32i.yaml: 40 instructions
[INFO] Loading rv32m.yaml: 8 instructions
```

When error occurs:
```
[ERROR] Schema include 'rv32_invalid.yaml' referenced by ISA map not found
[ERROR] Unable to resolve schema sources for ISA 'rv32bad'
```

---

## Performance Considerations

### Caching
- **Not implemented**: Each call to `load_isa_config()` re-loads schemas
- **Recommendation**: Call once at mutator initialization, store result

### Memory Usage
- **YAML trees**: Kept in memory during loading, discarded after parsing
- **Final config**: ~1-2 MB for typical ISA (RV32IM: ~50 instructions)

### Loading Time
- **Typical**: 10-50ms for RV32IM (3 schema files)
- **Bottleneck**: YAML parsing, not filesystem I/O

---

## Related Files

- **IsaLoader.hpp**: Public API declarations
- **YamlUtils.cpp**: YAML parsing helpers (includes_from_map, merge_nodes)
- **Debug.cpp**: Debug logging system
- **ISAMutator.cpp**: Consumer of ISAConfig

---

## Version History

- **Initial Implementation**: October 2024 - Basic schema loading
- **November 2024**: 
  - Removed SCHEMA_DIR environment variable (hardcoded path)
  - Replaced exceptions with debug logging
  - Removed unused error_code variables
  - Simplified path resolution logic
