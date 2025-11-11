# Fuzzer Configuration System

## Overview

The fuzzer now uses a centralized configuration file (`fuzzer.env`) for all environment variables. This makes it easier to customize fuzzer behavior without modifying `run.sh`.

## Configuration Files

### 1. `fuzzer.env` - Main Configuration
Contains all environment variables for the fuzzer:
- Mutator settings (strategy, compression, debug)
- Execution limits (cycles, timeouts, program size)
- Golden model configuration (Spike integration)
- Toolchain paths (objdump, objcopy, linker, etc.)
- Trace and logging settings

### 2. `tools/memory_config.mk` - Memory Map
Defines the RISC-V memory layout:
- RAM base address and size
- Reset vector, IRQ vector
- Stack pointer
- TOHOST MMIO address

## Usage

### Option 1: Use run.sh (Automatic)
```bash
./run.sh --cores 4 --strategy HYBRID
```
The script automatically loads `fuzzer.env` before starting AFL++.

### Option 2: Manual Execution
```bash
# Load environment
source fuzzer.env

# Run harness directly
afl/afl_picorv32 seeds/test_program.bin

# Or run AFL++ manually
afl-fuzz -i seeds -o corpora -- afl/afl_picorv32 @@
```

### Option 3: Custom Configuration
```bash
# Create a custom config
cp fuzzer.env my_config.env
nano my_config.env  # Edit settings

# Use it
source my_config.env
./run.sh
```

## Configuration Priority

Settings are applied in this order (later overrides earlier):

1. **Defaults in fuzzer.env** - Base configuration
2. **Pre-existing environment variables** - Set before running
3. **Command-line arguments to run.sh** - Highest priority

Example:
```bash
# fuzzer.env sets GOLDEN_MODE=live
# Override it:
GOLDEN_MODE=off ./run.sh

# Or via CLI:
./run.sh --golden off
```

## Key Environment Variables

### Mutator Configuration
| Variable | Default | Description |
|----------|---------|-------------|
| `MUTATOR_CONFIG` | `afl/isa_mutator/config/mutator.default.yaml` | Mutator YAML config file |
| `SCHEMA_DIR` | `schemas` | ISA schema directory |
| `DEBUG` | `0` | Master debug switch: enables all logging to `afl/isa_mutator/logs/mutator_debug.log` |

**Note**: Mutation strategy, compressed instructions, probabilities, and weights are now configured in the YAML file specified by `MUTATOR_CONFIG`, not via environment variables.

### Execution Limits
| Variable | Default | Description |
|----------|---------|-------------|
| `MAX_CYCLES` | `50000` | Maximum CPU cycles per test |
| `PC_STAGNATION_LIMIT` | `512` | Max commits at same PC |
| `MAX_PROGRAM_WORDS` | `256` | Max program size (words) |

### Golden Model (Spike)
| Variable | Default | Description |
|----------|---------|-------------|
| `GOLDEN_MODE` | `live` | Golden model mode: live, off, batch, replay |
| `STOP_ON_SPIKE_DONE` | `1` | Exit when Spike completes |
| `SPIKE_BIN` | `/opt/riscv/bin/spike` | Spike simulator path |
| `SPIKE_ISA` | `rv32im` | RISC-V ISA string |

### Toolchain
| Variable | Default | Description |
|----------|---------|-------------|
| `OBJCOPY_BIN` | `/opt/riscv/bin/riscv32-unknown-elf-objcopy` | objcopy path |
| `OBJDUMP_BIN` | `/opt/riscv/bin/riscv32-unknown-elf-objdump` | objdump path |
| `LD_BIN` | `/opt/riscv/bin/riscv32-unknown-elf-ld` | Linker path |

### Trace & Logging
| Variable | Default | Description |
|----------|---------|-------------|
| `TRACE_MODE` | `on` | Enable per-commit trace writing |
| `EXEC_BACKEND` | `verilator` | Execution backend (verilator only for now) |

**Note**: The harness automatically redirects all stdout/stderr to `logs/harness.log` to keep AFL++ stdio clean. Use `DEBUG=1` for verbose debug output to `afl/isa_mutator/logs/mutator_debug.log`.

### Exit Stub
| Variable | Default | Description |
|----------|---------|-------------|
| `APPEND_EXIT_STUB` | `1` | Append exit stub to programs |
| `TOHOST_ADDR` | `0x80001000` | MMIO address for exit signaling |

## Memory Map Configuration

Edit `tools/memory_config.mk` to change memory layout:

```makefile
RAM_BASE        := 0x80040000
RAM_SIZE_BYTES  := 262144
PROGADDR_RESET  := 0x80000000
PROGADDR_IRQ    := 0x80000100
STACK_ADDR      := 0x8007FFF0
TOHOST_ADDR     := 0x80001000
```

These values are automatically loaded by `run.sh` and passed to the harness.

## Quick Examples

### High-speed fuzzing (no golden model, no traces)
```bash
cat > fast_fuzz.env <<EOF
source fuzzer.env
export GOLDEN_MODE="off"
export TRACE_MODE="off"
export FUZZER_QUIET="1"
export MAX_CYCLES="10000"
EOF

source fast_fuzz.env
./run.sh --cores 8
```

### Deep verification (golden model + traces)
```bash
cat > deep_verify.env <<EOF
source fuzzer.env
export GOLDEN_MODE="live"
export TRACE_MODE="on"
export MAX_CYCLES="100000"
export STOP_ON_SPIKE_DONE="1"
EOF

source deep_verify.env
./run.sh --cores 2
```

### Debug mode (verbose logging)
```bash
source fuzzer.env
export DEBUG="1"
./run.sh --cores 1

# View logs in another terminal
tail -f afl/isa_mutator/logs/mutator_debug.log
```

## File Locations

After running, check these directories:
- **Crashes**: `workdir/<timestamp>/logs/crash/`
- **Harness logs**: `workdir/<timestamp>/logs/harness.log`
- **Traces**: `workdir/<timestamp>/traces/`
- **AFL++ corpus**: `workdir/<timestamp>/corpora/`
- **Spike logs**: `workdir/<timestamp>/spike.log`

## Troubleshooting

### Environment not loading
Check that `fuzzer.env` exists in the project root:
```bash
ls -l fuzzer.env
```

### Values not taking effect
Check the priority order - CLI args override everything:
```bash
# This WILL use HYBRID (CLI overrides env)
GOLDEN_MODE=off ./run.sh --golden live
```

### Missing tools
If Spike/objcopy/objdump not found, update paths in `fuzzer.env`:
```bash
export SPIKE_BIN="/custom/path/to/spike"
export OBJDUMP_BIN="/custom/path/to/objdump"
```

## See Also
- `run.sh --help` - Full command-line options
- `FUZZER_OUTPUT.md` - Output and logging system
- `CRASH_DETECTION.md` - How AFL++ detects crashes
