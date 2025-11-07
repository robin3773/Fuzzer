# YAML Configuration Migration

## Overview
Migrated all mutator configuration from environment variables to YAML-only configuration. This simplifies configuration management and makes settings more discoverable and maintainable.

## What Changed

### Before (Environment Variables)
Configuration was split between environment variables and YAML:
```bash
# Environment variables controlled key settings
export RV32_STRATEGY="HYBRID"
export RV32_ENABLE_C="1"
export RV32_DECODE_PROB="60"
export RV32_IMM_RANDOM="25"
export RV32_R_BASE="70"
export RV32_R_M="30"
export MUTATOR_ISA="rv32im"

# Plus YAML file for schemas
export MUTATOR_CONFIG="afl/isa_mutator/config/mutator.default.yaml"
```

### After (YAML Only)
All configuration now lives in the YAML file:
```yaml
# afl/isa_mutator/config/mutator.default.yaml
strategy: IR              # RAW | IR | HYBRID | AUTO
verbose: true
enable_c: false           # Enable compressed instructions

probabilities:
  decode: 60              # Schema-guided mutation chance
  imm_random: 25          # Random immediate weighting

weights:
  r_base_alu: 70          # ALU instruction weight
  r_m: 30                 # M-extension weight

schemas:
  isa: rv32im             # ISA name
  root: ./schemas
  map: isa_map.yaml
```

## Code Changes

### 1. MutatorConfig.cpp
**File**: `afl/isa_mutator/src/MutatorConfig.cpp`

Removed environment variable overrides from `Config::applyEnvironment()`:
```cpp
// Before: Applied env vars to override YAML settings
void Config::applyEnvironment() {
  const char *s = std::getenv("RV32_STRATEGY");
  if (s && *s)
    strategy = parse_strategy_token(s, strategy);
  // ... 30+ lines of env var processing
}

// After: No-op, kept for backward compatibility
void Config::applyEnvironment() {
  // All configuration now comes from YAML file only.
  // Environment variable overrides have been removed.
  // This function is kept for backward compatibility but does nothing.
}
```

### 2. fuzzer.env
**File**: `fuzzer.env`

Removed obsolete RV32_* variables:
```bash
# Removed:
# export RV32_STRATEGY="HYBRID"
# export RV32_ENABLE_C="1"

# Kept (points to YAML config):
export MUTATOR_CONFIG="afl/isa_mutator/config/mutator.default.yaml"
export SCHEMA_DIR="schemas"
export DEBUG_MUTATOR="0"
```

### 3. run.sh
**File**: `run.sh`

Removed RV32_* variable handling:
- Removed `STRATEGY` and `ENABLE_C` local variables
- Removed `--strategy` and `--enable-c` CLI arguments
- Removed RV32_STRATEGY and RV32_ENABLE_C from AFL_KEEP_ENV
- Removed Strategy/Enable C from banner output
- Simplified usage help text

### 4. Documentation Updates

**CONFIG.md**:
- Removed RV32_STRATEGY and RV32_ENABLE_C from environment variable table
- Added note: "Mutation strategy, compressed instructions, probabilities, and weights are now configured in the YAML file"

**afl/isa_mutator/README.md**:
- Replaced "Runtime knobs" section with "Configuration" section
- Documented YAML fields with examples
- Removed all references to RV32_* environment variables
- Updated usage examples to show MUTATOR_CONFIG instead

## Migration Guide

### For Users
If you were using environment variables to configure the mutator:

**Old approach**:
```bash
export RV32_STRATEGY="HYBRID"
export RV32_ENABLE_C="1"
./run.sh --cores 4
```

**New approach**:
1. Edit the YAML config file:
```bash
vim afl/isa_mutator/config/mutator.default.yaml
# Or create your own:
cp afl/isa_mutator/config/mutator.default.yaml my_config.yaml
vim my_config.yaml
```

2. Point to your config:
```bash
export MUTATOR_CONFIG="my_config.yaml"
./run.sh --cores 4
# OR
./run.sh --mutator-config my_config.yaml --cores 4
```

### For Developers
Environment variable processing has been removed from:
- `Config::applyEnvironment()` - Now a no-op
- All RV32_* variables throughout the codebase

Configuration loading order:
1. Load YAML file specified by `MUTATOR_CONFIG`
2. Apply defaults for any missing fields
3. No environment variable overrides

## Benefits

### 1. Single Source of Truth
All mutator settings in one YAML file instead of split between env vars and YAML.

### 2. Better Discoverability
Users can see all available settings by looking at the YAML file with comments.

### 3. Easier Sharing
Share entire configuration profiles by copying a single YAML file:
```bash
# Share aggressive fuzzing config
cp mutator.aggressive.yaml /path/to/another/project/
```

### 4. Version Control Friendly
Configuration changes show up clearly in git diffs:
```diff
 strategy: IR
-enable_c: false
+enable_c: true
 probabilities:
-  decode: 60
+  decode: 80
```

### 5. No CLI Clutter
Removed `--strategy` and `--enable-c` from run.sh, simplifying the interface.

## Available YAML Profiles

The repository includes several pre-configured YAML profiles:

1. **mutator.default.yaml** - Balanced settings (IR strategy, no compressed)
2. **mutator.aggressive.yaml** - High mutation rates for stress testing
3. **mutator.riscv32e.yaml** - RV32E ISA variant (embedded subset)

Create custom profiles by copying and modifying these files.

## Backward Compatibility

### Removed Without Replacement
These environment variables are **no longer supported**:
- `RV32_STRATEGY` - Use `strategy:` in YAML
- `RV32_ENABLE_C` - Use `enable_c:` in YAML  
- `RV32_DECODE_PROB` - Use `probabilities.decode:` in YAML
- `RV32_IMM_RANDOM` - Use `probabilities.imm_random:` in YAML
- `RV32_R_BASE` - Use `weights.r_base_alu:` in YAML
- `RV32_R_M` - Use `weights.r_m:` in YAML
- `RV32_VERBOSE` - Use `verbose:` in YAML
- `MUTATOR_ISA` - Use `schemas.isa:` in YAML

### Still Supported
These environment variables remain:
- `MUTATOR_CONFIG` - Path to YAML config file (required)
- `SCHEMA_DIR` - Schema directory root
- `DEBUG_MUTATOR` - Enable debug logging
- `MUTATOR_EFFECTIVE_CONFIG` - Dump effective config path

## Build Verification

After migration, full rebuild succeeds:
```bash
make -C afl clean && make -C afl build
```

All components compile without warnings:
- ✅ Mutator library: `libisa_mutator.so`
- ✅ Harness binary: `afl_picorv32`
- ✅ No compilation errors or warnings

## Testing

Verify configuration loading:
```bash
export MUTATOR_CONFIG="afl/isa_mutator/config/mutator.default.yaml"
export SCHEMA_DIR="schemas"
./afl/afl_picorv32 seeds/firmware.bin
```

Check startup log for confirmation:
```
[INFO] Config file opened successfully: afl/isa_mutator/config/mutator.default.yaml
[INFO] Loaded config: afl/isa_mutator/config/mutator.default.yaml
[INFO] strategy=IR verbose=true enable_c=false decode_prob=60 imm_random_prob=25 ...
```

## Related Files

### Modified Files
- `afl/isa_mutator/src/MutatorConfig.cpp` - Removed env var processing
- `fuzzer.env` - Removed RV32_* variables
- `run.sh` - Removed strategy/enable_c CLI args and env exports
- `CONFIG.md` - Updated environment variable docs
- `afl/isa_mutator/README.md` - Replaced env var docs with YAML docs

### Unchanged Files
- `afl/isa_mutator/config/mutator.default.yaml` - Already had all settings
- `afl/isa_mutator/config/mutator.aggressive.yaml` - Pre-existing profile
- `afl/isa_mutator/config/mutator.riscv32e.yaml` - Pre-existing profile
- `afl/isa_mutator/include/fuzz/mutator/MutatorConfig.hpp` - Interface unchanged

## Summary

This migration consolidates all mutator configuration into YAML files, removing the need for multiple RV32_* environment variables. Configuration is now more discoverable, shareable, and maintainable, while the simplified CLI interface reduces user confusion.
