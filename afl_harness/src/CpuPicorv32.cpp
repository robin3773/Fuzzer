#include "CpuIface.hpp"
#include "Vpicorv32.h"
#include "verilated.h"

#include <cstring>
#include <vector>
#include <cstdint>

static const size_t MEM_BYTES = 64 * 1024;
static uint8_t mem_area[MEM_BYTES];

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

class CpuPicoRV32 final : public CpuIface {
public:
    CpuPicoRV32() {
    #if VL_VER_MAJOR >= 5
        Verilated::randReset(0);
    #else
        Verilated::randSeed(0);
    #endif
        top_ = new Vpicorv32;
    }
    ~CpuPicoRV32() override { delete top_; }


// Clears memory and resets the CPU for 8 cycles.
    void reset() override {
      std::memset(mem_area, 0, sizeof(mem_area));
      top_->resetn    = 0;
      top_->mem_valid = 0;
      top_->mem_ready = 0;
      top_->mem_wstrb = 0;
      for (int i = 0; i < 8; ++i) tick(top_);
      top_->resetn = 1;
    }

    // Loads binary input into memory (up to 64 KB).
    void load_input(const std::vector<unsigned char>& in) override {
      size_t copy_n = in.size() > MEM_BYTES ? MEM_BYTES : in.size();
      std::memcpy(mem_area, in.data(), copy_n);
    }

    void step() override {
      top_->mem_ready = 0;
      if (top_->mem_valid) {
        if (top_->mem_wstrb) mem_write32(top_->mem_addr, top_->mem_wdata, top_->mem_wstrb);
        else                 top_->mem_rdata = mem_read32(top_->mem_addr);
        top_->mem_ready = 1;
      }
      tick(top_);
    }

    bool      got_finish()              const override { return Verilated::gotFinish();         }
    bool      trap()                    const override { return top_->rvfi_trap;                }
    bool      rvfi_valid()              const override { return top_->rvfi_valid;               }
    uint32_t  rvfi_insn()               const override { return top_->rvfi_insn;                }
    uint32_t  rvfi_pc_rdata()           const override { return top_->rvfi_pc_rdata;            }
    uint32_t  rvfi_pc_wdata()           const override { return top_->rvfi_pc_wdata;            }
    uint32_t  rvfi_rd_addr()            const override { return top_->rvfi_rd_addr;             }
    uint32_t  rvfi_rd_wdata()           const override { return top_->rvfi_rd_wdata;            }
    uint32_t  rvfi_mem_addr()           const override { return top_->rvfi_mem_addr;            }
    uint32_t  rvfi_mem_rmask()          const override { return top_->rvfi_mem_rmask;           }
    uint32_t  rvfi_mem_wmask()          const override { return top_->rvfi_mem_wmask;           }
    uint64_t  rvfi_csr_mcycle_wmask()   const override { return top_->rvfi_csr_mcycle_wmask;    }
    uint64_t  rvfi_csr_mcycle_wdata()   const override { return top_->rvfi_csr_mcycle_wdata;    }
    uint64_t  rvfi_csr_minstret_wmask() const override { return top_->rvfi_csr_minstret_wmask;  }
    uint64_t  rvfi_csr_minstret_wdata() const override { return top_->rvfi_csr_minstret_wdata;  }

  private:
    Vpicorv32* top_ = nullptr;
};

extern "C" CpuIface* make_cpu() { return new CpuPicoRV32(); }
