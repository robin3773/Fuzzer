#pragma once
#include <vector>
#include <cstdint>

class CpuIface {
public:
  virtual ~CpuIface() = default;
  virtual void reset() = 0;
  virtual void load_input(const std::vector<unsigned char>& in) = 0;
  virtual void step() = 0;
  virtual bool got_finish() const = 0;
  virtual bool trap() const = 0;
  virtual uint32_t rvfi_insn() const = 0;
  virtual uint32_t rvfi_pc_rdata() const = 0;

  // Optional RVFI/monitoring accessors for deeper checks
  virtual bool     rvfi_valid() const = 0;
  virtual uint32_t rvfi_pc_wdata() const = 0;
  virtual uint32_t rvfi_rd_addr() const = 0;
  virtual uint32_t rvfi_rd_wdata() const = 0;
  virtual uint32_t rvfi_mem_addr() const = 0;
  virtual uint32_t rvfi_mem_rmask() const = 0;
  virtual uint32_t rvfi_mem_wmask() const = 0;
};
