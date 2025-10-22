// Minimal AFL-safe single-shot harness for PicoRV32 (no C++ dtors, no STL)
#include "Vpicorv32.h"
#include "verilated.h"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

static const size_t MEM_BYTES = 64 * 1024;
static const size_t INBUF_MAX = 1 << 20; // 1 MiB cap

static uint8_t mem_area[MEM_BYTES];
static uint8_t inbuf[INBUF_MAX];
static size_t  inlen = 0;

static inline void tick(Vpicorv32 *t) { t->clk = 0; t->eval(); t->clk = 1; t->eval(); }

static inline uint32_t mem_read32(uint32_t addr) {
  uint32_t a = (addr & (MEM_BYTES - 1)) & ~0x3u;
  return (uint32_t)mem_area[a] |
         ((uint32_t)mem_area[a+1] << 8) |
         ((uint32_t)mem_area[a+2] << 16) |
         ((uint32_t)mem_area[a+3] << 24);
}

static inline void mem_write32(uint32_t addr, uint32_t data, uint8_t wstrb) {
  uint32_t a = (addr & (MEM_BYTES - 1)) & ~0x3u;
  if (wstrb & 1) mem_area[a+0] = (uint8_t)(data & 0xFF);
  if (wstrb & 2) mem_area[a+1] = (uint8_t)((data >> 8) & 0xFF);
  if (wstrb & 4) mem_area[a+2] = (uint8_t)((data >> 16) & 0xFF);
  if (wstrb & 8) mem_area[a+3] = (uint8_t)((data >> 24) & 0xFF);
}

static void read_all_fd(int fd) {
  inlen = 0;
  for (;;) {
    if (inlen >= INBUF_MAX) break;
    ssize_t n = read(fd, inbuf + inlen, INBUF_MAX - inlen);
    if (n <= 0) break;
    inlen += (size_t)n;
  }
  if (inlen == 0) { inbuf[0] = 0x13; inlen = 1; } // harmless RV32I ADDI x0,x0,0
}

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
#if VL_VER_MAJOR >= 5
  Verilated::randReset(0);
#endif

  // Prefer @@ file (argv[1]); else stdin
  if (argc > 1 && argv[1][0] != '-') {
    int fd = open(argv[1], O_RDONLY);
    if (fd >= 0) { read_all_fd(fd); close(fd); }
    else { read_all_fd(STDIN_FILENO); }
  } else {
    read_all_fd(STDIN_FILENO);
  }

  // Zero memory; copy input
  memset(mem_area, 0, MEM_BYTES);
  size_t copy_n = inlen > MEM_BYTES ? MEM_BYTES : inlen;
  memcpy(mem_area, inbuf, copy_n);

  // Max cycles (env override)
  unsigned max_cycles = 10000;
  if (const char *mc = getenv("MAX_CYCLES")) {
    unsigned v = (unsigned)strtoul(mc, 0, 0);
    if (v) max_cycles = v;
  }

  // Construct DUT
  Vpicorv32 *top = new Vpicorv32;

  // Full reset (active-low resetn)
  top->resetn    = 0;
  top->mem_valid = 0;
  top->mem_ready = 0;
  top->mem_wstrb = 0;
  for (int i = 0; i < 8; ++i) tick(top);
  top->resetn = 1;

  // Run bounded simulation
  for (unsigned cyc = 0; cyc < max_cycles && !Verilated::gotFinish(); ++cyc) {
    top->mem_ready = 0;
    if (top->mem_valid) {
      if (top->mem_wstrb) mem_write32(top->mem_addr, top->mem_wdata, top->mem_wstrb);
      else                top->mem_rdata = mem_read32(top->mem_addr);
      top->mem_ready = 1;
    }
    tick(top);
    if (top->trap) { _exit(1); }  // signal crash to AFL
  }

  _exit(0); // avoid C++ dtors touching AFL runtime
}
