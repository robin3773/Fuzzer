#include "Vpicorv32.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

// 4 KB on-chip memory model
static uint32_t mem[1024]; // 1024 × 4 B = 4 KB

static inline uint32_t mem_read(uint32_t addr) {
    return mem[(addr & 0xFFF) >> 2];
}
static inline void mem_write(uint32_t addr, uint32_t data, uint8_t wstrb) {
    uint32_t &word = mem[(addr & 0xFFF) >> 2];
    for (int i = 0; i < 4; ++i)
        if (wstrb & (1 << i))
            ((uint8_t *)&word)[i] = (data >> (8 * i)) & 0xFF;
}

static inline void tick(Vpicorv32 *top, VerilatedVcdC *tfp, uint64_t tickcount) {
    top->clk = 0; top->eval(); if (tfp) tfp->dump(10 * tickcount + 0);
    top->clk = 1; top->eval(); if (tfp) tfp->dump(10 * tickcount + 5);
}

// Load input (seed or fuzzed data) into memory
static void load_input(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return;
    fread(mem, 1, sizeof(mem), f);
    fclose(f);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    const char *input_file = (argc > 1) ? argv[1] : "firmware/build/firmware.bin";
    load_input(input_file);

    // Init
    Verilated::traceEverOn(true);
    Vpicorv32 *top = new Vpicorv32;

    VerilatedVcdC *tfp = nullptr;
    if (const char *dump = std::getenv("AFL_DUMP_VCD")) {
        tfp = new VerilatedVcdC;
        top->trace(tfp, 99);
        tfp->open(dump);
    }

    // Reset
    top->resetn = 0;
    for (int i = 0; i < 5; i++) tick(top, tfp, i);
    top->resetn = 1;

    uint64_t MAX_CYCLES = 10000;
    if (const char *env = std::getenv("MAX_CYCLES"))
        MAX_CYCLES = strtoull(env, nullptr, 10);

    for (uint64_t cycle = 0; cycle < MAX_CYCLES; ++cycle) {
        top->mem_ready = 0;
        if (top->mem_valid) {
            if (top->mem_wstrb)
                mem_write(top->mem_addr, top->mem_wdata, top->mem_wstrb);
            else
                top->mem_rdata = mem_read(top->mem_addr);
            top->mem_ready = 1;
        }

        tick(top, tfp, cycle);

        // Stop on trap or out-of-range access
        if (top->trap || top->mem_addr > 0xFFF) break;
    }

    if (tfp) { tfp->close(); delete tfp; }
    delete top;
    return 0;
}
