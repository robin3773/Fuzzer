// ==========================================================
//  tb_picorv32_fuzz.cpp — AFL++ Fuzz Harness for PicoRV32
// ==========================================================
//  • Loads mutated input (firmware.bin) from AFL++.
//  • Drives the verilated model cycle by cycle.
//  • Detects DUT faults and reports them to AFL as crashes.
//  • Dumps waveform only when AFL_DUMP_VCD is set.
//
//  Compile via Makefile.fuzz using afl-clang-fast++.
//
// ==========================================================

#include "Vpicorv32.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <csignal>
#include <unistd.h>

// ==========================================================
// Small 4 KB Memory
// ==========================================================
static uint32_t mem[1024];

static inline uint32_t mem_read(uint32_t addr) {
    return mem[(addr & 0xFFF) >> 2];
}

static inline void mem_write(uint32_t addr, uint32_t data, uint8_t wstrb) {
    uint32_t &word = mem[(addr & 0xFFF) >> 2];
    for (int i = 0; i < 4; i++)
        if (wstrb & (1 << i))
            ((uint8_t *)&word)[i] = (data >> (8 * i)) & 0xFF;
}

// ==========================================================
// Clock Tick Helper
// ==========================================================
static inline void tick(Vpicorv32 *top, VerilatedVcdC *tfp, vluint64_t &t) {
    top->clk = 0;
    top->eval();
    if (tfp) tfp->dump(t++);
    top->clk = 1;
    top->eval();
    if (tfp) tfp->dump(t++);
}

// ==========================================================
// Crash Monitor for AFL++
// ==========================================================
static uint64_t last_activity_cycle = 0;
static const uint64_t HANG_THRESHOLD = 50000;  // cycles of inactivity
static uint32_t prev_pc = 0;

inline void crash_monitor(Vpicorv32 *top, uint64_t cycle) {
    // 1. Trap or Exception
    if (top->trap) {
        fprintf(stderr,
                "[CRASH] Trap signal asserted at cycle %" PRIu64
                "  PC=0x%08x\n",
                cycle, top->mem_addr);
        raise(SIGABRT); // AFL logs as crash
    }

    // 2. Out-of-bounds memory
    if (top->mem_valid && (top->mem_addr >= 0x1000)) {
        fprintf(stderr,
                "[CRASH] Out-of-bounds memory access 0x%08x at cycle %" PRIu64 "\n",
                top->mem_addr, cycle);
        raise(SIGSEGV);
    }

    // 3. Illegal instruction (RVFI)
    if (top->rvfi_valid && (top->rvfi_insn == 0x00000000)) {
        fprintf(stderr,
                "[CRASH] Illegal instruction 0x%08x at cycle %" PRIu64 "\n",
                top->rvfi_insn, cycle);
        raise(SIGILL);
    }

    // 4. Bus deadlock / hang
    if (top->mem_valid && !top->mem_ready) {
        if ((cycle - last_activity_cycle) > HANG_THRESHOLD) {
            fprintf(stderr,
                    "[CRASH] Bus hang detected (>%" PRIu64 " cycles)\n",
                    HANG_THRESHOLD);
            raise(SIGTERM);
        }
    } else {
        last_activity_cycle = cycle;
    }

    // 5. PC not progressing
    if (top->rvfi_valid) {
        if (top->rvfi_pc_rdata == prev_pc) {
            if ((cycle - last_activity_cycle) > HANG_THRESHOLD) {
                fprintf(stderr,
                        "[CRASH] PC stuck at 0x%08x for %" PRIu64 " cycles\n",
                        top->rvfi_pc_rdata, HANG_THRESHOLD);
                raise(SIGALRM);
            }
        } else {
            prev_pc = top->rvfi_pc_rdata;
            last_activity_cycle = cycle;
        }
    }
}

// ==========================================================
// Main Simulation
// ==========================================================
int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    Vpicorv32 *top = new Vpicorv32;
    VerilatedVcdC *tfp = nullptr;
    vluint64_t t = 0;

    // --------------------------------------------------------
    // Conditional VCD (only when AFL_DUMP_VCD is set)
    // --------------------------------------------------------
    const char *vcdpath = getenv("AFL_DUMP_VCD");
    if (vcdpath) {
        tfp = new VerilatedVcdC;
        top->trace(tfp, 99);
        tfp->open(vcdpath);
        fprintf(stderr, "[INFO] Waveform dump enabled: %s\n", vcdpath);
    }

    // --------------------------------------------------------
    // Simulation config (plusargs)
    // --------------------------------------------------------
    uint64_t print_every = 100;
    uint64_t max_cycles  = 2000000;
    if (const char *pe = Verilated::commandArgsPlusMatch("print_every=")) {
        const char *v = strchr(pe, '='); if (v) print_every = strtoull(v + 1, nullptr, 0);
    }
    if (const char *mc = Verilated::commandArgsPlusMatch("max_cycles=")) {
        const char *v = strchr(mc, '='); if (v) max_cycles = strtoull(v + 1, nullptr, 0);
    }

    // --------------------------------------------------------
    // Reset sequence
    // --------------------------------------------------------
    top->resetn = 0;
    for (int i = 0; i < 10; i++) tick(top, tfp, t);
    top->resetn = 1;

    // --------------------------------------------------------
    // Load firmware (mutated input from AFL++)
    // --------------------------------------------------------
    memset(mem, 0, sizeof(mem));
    const char *fwpath = (argc > 1) ? argv[1] : "firmware/build/firmware.bin";
    FILE *f = fopen(fwpath, "rb");
    if (!f) {
        perror("[ERR] fopen firmware");
        return 1;
    }
    size_t n = fread(mem, 1, sizeof(mem), f);
    fclose(f);

    if (n == 0) {
        fprintf(stderr, "[WARN] Empty input file %s\n", fwpath);
        return 1;
    }
    if (n > sizeof(mem)) {
        fprintf(stderr, "[WARN] Input truncated (%zu > %zu bytes)\n", n, sizeof(mem));
    }

    printf("[INFO] Loaded %zu bytes from %s\n", n, fwpath);

    // --------------------------------------------------------
    // Run Simulation
    // --------------------------------------------------------
    for (uint64_t cycle = 1; cycle < max_cycles; ++cycle) {
        top->mem_ready = 0;
        if (top->mem_valid) {
            if (top->mem_wstrb)
                mem_write(top->mem_addr, top->mem_wdata, top->mem_wstrb);
            else
                top->mem_rdata = mem_read(top->mem_addr);
            top->mem_ready = 1;
        }

        tick(top, tfp, t);

        crash_monitor(top, cycle);  // monitor faults

        if (print_every && (cycle % print_every) == 0) {
            printf("Cycle %6" PRIu64 "  PC≈0x%08x  trap=%d\n",
                   cycle, top->mem_addr, top->trap);
            fflush(stdout);
        }
    }

    printf("[INFO] Simulation completed without fatal trap.\n");

    if (tfp) {
        tfp->close();
        delete tfp;
    }
    delete top;
    return 0;
}
