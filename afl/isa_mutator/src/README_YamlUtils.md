# YamlUtils.cpp - YAML Parsing Utilities

## Overview

`YamlUtils.cpp` provides helper functions for parsing and manipulating YAML schema files. It handles complex YAML features like anchors, includes, merging, and custom integer parsing to support the ISA loader.

## Function Reference

### 1. parse_integer()

```cpp
uint64_t parse_integer(const YAML::Node &node)
```

**Purpose**: Parses integer literals in various formats (decimal, hex, binary).

**Supported Formats**:
- Decimal: `42`, `-100`
- Hexadecimal: `0x2A`, `0xFF`
- Binary: `0b101010`, `0b11111111`

**Algorithm**:
1. Check if node is scalar
2. Extract text string
3. Detect sign (+/-)
4. Detect base prefix (0x, 0b)
5. Parse digits in appropriate base
6. Apply sign if negative

**Example**:
```yaml
opcode: 0b0110011    # → 51
funct3: 0x7          # → 7
imm: -100            # → -100 (as uint64_t)
```

**Error Handling**: Throws `std::runtime_error` on invalid literal

---

### 2. merge_nodes()

```cpp
void merge_nodes(YAML::Node &base, const YAML::Node &overlay)
```

**Purpose**: Deep merges two YAML nodes, combining maps recursively.

**Merge Rules**:
- **Null base**: Replace with overlay
- **Map + Map**: Recursively merge all keys
- **Special key `<<`**: YAML merge key (includes anchors)
- **Keys starting with `__`**: Ignored (internal use)
- **Other types**: Overlay replaces base

**Example**:
```yaml
# Base
defaults:
  width: 32
  endian: little

# Overlay
defaults:
  width: 32
  registers: 32

# Merged result
defaults:
  width: 32
  endian: little
  registers: 32
```

**YAML Merge Key Example**:
```yaml
base_insn: &base_insn
  width: 32
  format: R

add:
  <<: *base_insn
  opcode: 0x33

# Result for 'add':
add:
  width: 32
  format: R
  opcode: 0x33
```

---

### 3. strip_quotes()

```cpp
std::string strip_quotes(std::string s)
```

**Purpose**: Removes leading/trailing quotes from strings.

**Example**:
```cpp
strip_quotes("\"hello\"") → "hello"
strip_quotes("'world'") → "world"
strip_quotes("no quotes") → "no quotes"
```

---

### 4. split_lines()

```cpp
std::vector<std::string> split_lines(const std::string &text)
```

**Purpose**: Splits text into lines on `\n` boundaries.

**Note**: Does NOT compress empty lines (preserves structure).

---

### 5. extract_paths_for_key()

```cpp
std::vector<std::string> extract_paths_for_key(const std::string &text,
                                                const std::string &key)
```

**Purpose**: Extracts file paths associated with a YAML key without full parsing.

**Why**: Needed to resolve includes before loading files (bootstrap problem).

**Supported Formats**:

**Inline Value**:
```yaml
extends: rv32_base.yaml
```

**Inline Array**:
```yaml
includes: [rv32i.yaml, rv32m.yaml]
```

**Multi-line List**:
```yaml
includes:
  - rv32i.yaml
  - rv32m.yaml
  - rv32c.yaml
```

**Algorithm**:
1. Search for lines matching `key:`
2. Check if value on same line (inline)
3. If not, scan subsequent indented lines for `-` list items
4. Strip quotes and comments
5. Return all extracted paths

**Example**:
```yaml
rv32im:
  includes:
    - riscv/rv32_base.yaml
    - riscv/rv32i.yaml  # Base instructions
    - riscv/rv32m.yaml  # Multiply/Divide

# extract_paths_for_key(text, "includes")
# → ["riscv/rv32_base.yaml", "riscv/rv32i.yaml", "riscv/rv32m.yaml"]
```

---

### 6. read_file_to_string()

```cpp
std::string read_file_to_string(const fs::path &path)
```

**Purpose**: Reads entire file into string.

**Error Handling**: Throws `std::runtime_error` if file cannot be opened.

---

### 7. extract_anchor_blocks()

```cpp
std::vector<std::pair<std::string, std::string>> 
extract_anchor_blocks(const std::string &text)
```

**Purpose**: Extracts YAML anchor definitions for cross-file reuse.

**What are Anchors?**: YAML references that can be included elsewhere.

**Example**:
```yaml
common_opcodes: &opcodes
  OP: 0b0110011
  OP_IMM: 0b0010011
  LOAD: 0b0000011
  STORE: 0b0100011

# Later in same file or another file:
instructions:
  add:
    opcode: *OP  # References common_opcodes.OP
```

**Algorithm**:
1. Scan lines for `&` character
2. Extract anchor name after `&`
3. Determine indentation level
4. Collect all lines at deeper indentation
5. Return map of anchor_name → YAML block

**Return Example**:
```cpp
[
  {"opcodes", "common_opcodes: &opcodes\n  OP: 0b0110011\n  OP_IMM: 0b0010011\n"}
]
```

---

### 8. build_anchor_context()

```cpp
std::string build_anchor_context(
  const std::vector<std::pair<std::string, std::string>> &anchors)
```

**Purpose**: Converts anchor list into YAML preamble for next file.

**Example**:

**Input Anchors**:
```cpp
[
  {"opcodes", "common_opcodes: &opcodes\n  OP: 0x33\n"},
  {"formats", "base_formats: &formats\n  R: {...}\n"}
]
```

**Output Context**:
```yaml
__anchors:
  common_opcodes: &opcodes
    OP: 0x33
  base_formats: &formats
    R: {...}

```

**Usage**: Prepended to next schema file so it can reference anchors from previous files.

---

### 9. collect_dependencies()

```cpp
void collect_dependencies(const fs::path &path,
                          std::vector<fs::path> &ordered,
                          std::unordered_set<std::string> &visited)
```

**Purpose**: Recursively collects schema file dependencies in correct order.

**Keys Checked**:
- `extends`: Parent schema files
- `include`: Additional schema files

**Algorithm** (Depth-First Search):
1. Normalize file path
2. Check if already visited (avoid cycles)
3. Mark as visited
4. Read file content
5. Extract paths for "extends" key → recurse
6. Extract paths for "include" key → recurse
7. Add current file to ordered list

**Example**:

**rv32im.yaml**:
```yaml
extends: rv32i.yaml
```

**rv32i.yaml**:
```yaml
extends: rv32_base.yaml
```

**rv32_base.yaml**:
```yaml
# No dependencies
```

**Result**: `[rv32_base.yaml, rv32i.yaml, rv32im.yaml]`

**Why Order Matters**: Base files must be loaded first so child files can override/extend definitions.

---

### 10. includes_from_map()

```cpp
std::vector<std::string> includes_from_map(const fs::path &map_path,
                                            const std::string &isa_name)
```

**Purpose**: Looks up which schema files to load for a given ISA name.

**isa_map.yaml Structure**:

**Simple Mapping**:
```yaml
rv32i: riscv/rv32i.yaml
```

**Multiple Files**:
```yaml
rv32im:
  - riscv/rv32_base.yaml
  - riscv/rv32i.yaml
  - riscv/rv32m.yaml
```

**With Includes Key**:
```yaml
rv64gc:
  includes:
    - riscv/rv64_base.yaml
    - riscv/rv64i.yaml
    - riscv/rv64m.yaml
    - riscv/rv64a.yaml
    - riscv/rv64f.yaml
    - riscv/rv64d.yaml
    - riscv/rv64c.yaml
```

**Family-Based**:
```yaml
isa_families:
  riscv:
    rv32i:
      - riscv/rv32_base.yaml
      - riscv/rv32i.yaml
    rv32im:
      - riscv/rv32_base.yaml
      - riscv/rv32i.yaml
      - riscv/rv32m.yaml
```

**Algorithm**:
1. Load isa_map.yaml
2. Check `isa_families` section first
3. If found, extract includes list
4. If not in families, check top-level mapping
5. Handle scalar, sequence, or map with includes key
6. Return list of file paths

**Example**:
```cpp
includes_from_map("schemas/isa_map.yaml", "rv32im")
→ ["riscv/rv32_base.yaml", "riscv/rv32i.yaml", "riscv/rv32m.yaml"]
```

**Error Handling**: Throws `std::runtime_error` on YAML parse failure.

---

## Data Flow

### Anchor Propagation Flow

```
File 1: rv32_base.yaml
  ┌─────────────────────────┐
  │ common_rules: &rules   │
  │   width: 32             │
  │   endian: little        │
  └─────────────────────────┘
           │
           │ extract_anchor_blocks()
           ▼
  ┌─────────────────────────┐
  │ anchor_library:         │
  │ [{"rules", "..."}]      │
  └─────────────────────────┘
           │
           │ build_anchor_context()
           ▼
  ┌─────────────────────────┐
  │ __anchors:              │
  │   common_rules: &rules  │
  │     width: 32           │
  │     endian: little      │
  └─────────────────────────┘
           │
           │ Prepend to next file
           ▼
File 2: rv32i.yaml (with context)
  ┌─────────────────────────┐
  │ __anchors:              │
  │   common_rules: &rules  │
  │                         │
  │ <<: *rules              │
  │ instructions: {...}     │
  └─────────────────────────┘
```

### Dependency Resolution Flow

```
includes_from_map("rv32im")
  ↓
["rv32_base.yaml", "rv32i.yaml", "rv32m.yaml"]
  ↓
collect_dependencies(rv32_base.yaml)
  ├─ extends: (none)
  └─ ordered: [rv32_base.yaml]
  ↓
collect_dependencies(rv32i.yaml)
  ├─ extends: rv32_base.yaml (already visited)
  └─ ordered: [rv32_base.yaml, rv32i.yaml]
  ↓
collect_dependencies(rv32m.yaml)
  ├─ extends: rv32_base.yaml (already visited)
  └─ ordered: [rv32_base.yaml, rv32i.yaml, rv32m.yaml]
```

### Merge Flow Example

```
Base Node:               Overlay Node:
defaults:                defaults:
  width: 32                 registers: 32
  endian: little            endian: big

merge_nodes(base, overlay)
  ↓
For each key in overlay:
  - "defaults" → recurse
    - "registers" → add to base
    - "endian" → replace in base

Result:
defaults:
  width: 32          # kept from base
  endian: big        # overwritten by overlay
  registers: 32      # added from overlay
```

---

## Usage Examples

### Example 1: Parse Integer Literals

```cpp
YAML::Node node = YAML::Load("opcode: 0b0110011");
uint64_t opcode = yaml_utils::parse_integer(node["opcode"]);
// opcode = 51

YAML::Node hex_node = YAML::Load("addr: 0x80000000");
uint64_t addr = yaml_utils::parse_integer(hex_node["addr"]);
// addr = 2147483648
```

### Example 2: Merge Schema Files

```cpp
YAML::Node base = YAML::Load("width: 32\nendian: little");
YAML::Node overlay = YAML::Load("width: 32\nregisters: 32");

yaml_utils::merge_nodes(base, overlay);
// base now contains all keys from both
```

### Example 3: Extract Includes

```cpp
std::string yaml_content = R"(
rv32im:
  includes:
    - riscv/rv32_base.yaml
    - riscv/rv32i.yaml
    - riscv/rv32m.yaml
)";

auto paths = yaml_utils::extract_paths_for_key(yaml_content, "includes");
// paths = ["riscv/rv32_base.yaml", "riscv/rv32i.yaml", "riscv/rv32m.yaml"]
```

### Example 4: Collect Dependencies

```cpp
std::vector<fs::path> ordered;
std::unordered_set<std::string> visited;

yaml_utils::collect_dependencies("schemas/riscv/rv32im.yaml", ordered, visited);
// ordered = [rv32_base.yaml, rv32i.yaml, rv32m.yaml]
```

### Example 5: Load ISA Map

```cpp
auto includes = yaml_utils::includes_from_map(
  "schemas/isa_map.yaml", 
  "rv32im"
);
// includes = ["riscv/rv32_base.yaml", "riscv/rv32i.yaml", "riscv/rv32m.yaml"]
```

---

## Edge Cases

### Integer Parsing

**Negative Hex/Binary**:
```yaml
value: -0xFF  # Parsed as -(255) = -1 (as uint64_t: 18446744073709551615)
```

**Empty/Null**:
```yaml
value: null   # Returns 0
value: ""     # Returns 0
```

### Anchor Extraction

**Anchor in Comment**:
```yaml
# This is an anchor: &ignored
# NOT extracted (after #)
```

**Multiple Anchors Same Name**:
```yaml
rules: &rules
  width: 32

rules: &rules  # Overwrites previous
  width: 64
```
Latest definition wins in anchor_library.

### Dependency Cycles

```yaml
# a.yaml
extends: b.yaml

# b.yaml
extends: a.yaml
```

**Handled**: `visited` set prevents infinite recursion. First file processed wins.

---

## Error Handling

All functions throw `std::runtime_error` on errors:

**File Not Found**:
```cpp
read_file_to_string("missing.yaml")
→ throws "Failed to open schema file: missing.yaml"
```

**Invalid YAML**:
```cpp
includes_from_map("bad_map.yaml", "rv32i")
→ throws "Failed to parse ISA map 'bad_map.yaml': ..."
```

**Invalid Integer**:
```cpp
parse_integer("not_a_number")
→ throws "Invalid numeric literal: not_a_number"
```

---

## Performance Notes

- **Line-by-line parsing**: Used for include/extends extraction (fast, no full parse)
- **Anchor extraction**: Regex-like scanning (efficient for typical schema sizes)
- **Dependency resolution**: DFS with memoization (O(N) where N = number of schema files)
- **Typical overhead**: <10ms for RV32IM schema set

---

## Related Files

- **IsaLoader.cpp**: Main consumer of these utilities
- **isa_map.yaml**: ISA-to-file mapping configuration
- **schemas/riscv/*.yaml**: Schema files processed by these utilities
