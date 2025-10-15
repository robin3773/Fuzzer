#include "Vpicorv32.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>

// ------------------------------------------------------------
// Small 4 KB memory
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// Clock tick helper
// ------------------------------------------------------------
static inline void tick(Vpicorv32 *top, VerilatedVcdC *tfp, vluint64_t &t) {
    top->clk = 0;
    top->eval();
    if (tfp) tfp->dump(t++);
    top->clk = 1;
    top->eval();
    if (tfp) tfp->dump(t++);
}

// ------------------------------------------------------------
// Main simulation
// ------------------------------------------------------------
int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    Vpicorv32 *top = new Vpicorv32;
    VerilatedVcdC *tfp = new VerilatedVcdC;
    vluint64_t t = 0;

    top->trace(tfp, 99);
    tfp->open("./traces/waveform.vcd");

    // plusargs
    uint64_t print_every = 100;
    uint64_t max_cycles  = 2000000; // 2 million
    if (const char *pe = Verilated::commandArgsPlusMatch("print_every=")) {
        const char *v = strchr(pe, '='); if (v) print_every = strtoull(v + 1, nullptr, 0);
    }
    if (const char *mc = Verilated::commandArgsPlusMatch("max_cycles=")) {
        const char *v = strchr(mc, '='); if (v) max_cycles = strtoull(v + 1, nullptr, 0);
    }

    // --------------------------------------------------------
    // Reset
    // --------------------------------------------------------
    top->resetn = 0;
    for (int i = 0; i < 10; i++) tick(top, tfp, t);
    top->resetn = 1;

    // --------------------------------------------------------
    // Load firmware
    // --------------------------------------------------------
    memset(mem, 0, sizeof(mem));
    const char *fwpath = (argc > 1) ? argv[1] : "firmware/build/firmware.bin";
    FILE *f = fopen(fwpath, "rb");
    if (!f) {
        perror("[ERR] fopen firmware");
        return 1;
    }
    size_t n = fread(mem, 1, sizeof(mem), f);
    if (n == 0 && ferror(f)) {
        perror("[ERR] fread firmware");
        fclose(f);
        return 1;
    }
    fclose(f);
    printf("[INFO] Loaded %zu bytes (%.1f KiB) from %s\n", n, n / 1024.0, fwpath);
    printf("[INFO] First 4 words of firmware memory:\n");
    for (int i = 0; i < 4; i++)
        printf("  mem[%d] = 0x%08x\n", i, mem[i]);


    // --------------------------------------------------------
    // Run simulation
    // --------------------------------------------------------
    printf("Cycle %3llu  PC≈0x%08x  trap=%d\n", (unsigned long long)0, top->mem_addr, top->trap);

    for (uint64_t cycle = 1; cycle < max_cycles && !top->trap; ++cycle) {
        top->mem_ready = 0;
        if (top->mem_valid) {
            if (top->mem_wstrb)
                mem_write(top->mem_addr, top->mem_wdata, top->mem_wstrb);
            else
                top->mem_rdata = mem_read(top->mem_addr);
            top->mem_ready = 1;
        }

        tick(top, tfp, t);

        if (print_every && (cycle % print_every) == 0) {
            printf("Cycle %3llu  PC≈0x%08x  trap=%d  mem_v=%d mem_rdy=%d\n",
                   (unsigned long long)cycle,
                   top->mem_addr, top->trap,
                   (int)top->mem_valid, (int)top->mem_ready);
            fflush(stdout);
        }
    }

    printf("Simulation ended (trap=%d)\n", top->trap);
    tfp->close();
    delete tfp;
    delete top;
    return 0;
}
