/**
 * @file CpuIface.hpp
 * @brief Abstract interface for CPU implementations under test
 * 
 * This file defines the abstract base class for interfacing with various CPU
 * implementations (Verilator simulations, FPGA instances, etc.) in a uniform way.
 * The interface is designed around the RISC-V Formal Verification Interface (RVFI)
 * standard, which provides instruction-level observability for verification.
 */

#pragma once

#include <vector>
#include <cstdint>

/**
 * @class CpuIface
 * @brief Abstract interface for CPU/DUT (Device Under Test) implementations
 * 
 * CpuIface defines the contract that all CPU implementations must satisfy to be
 * compatible with the AFL++ fuzzing harness. It provides methods for:
 * - Lifecycle management (reset, stepping)
 * - Input loading (fuzzer-generated instruction streams)
 * - State observation via RVFI (RISC-V Formal Verification Interface)
 * - Exit condition detection
 * 
 * Concrete implementations (e.g., CpuPicorv32) wrap specific CPU designs and
 * expose their internal state through this interface for differential testing
 * against a golden model (Spike).
 * 
 * @note All RVFI methods follow the RISC-V Formal standard specification
 * @see https://github.com/riscv/riscv-formal
 * 
 * Example usage:
 * @code
 *   std::unique_ptr<CpuIface> cpu = std::make_unique<CpuPicorv32>();
 *   cpu->reset();
 *   cpu->load_input(fuzzer_input);
 *   while (!cpu->got_finish() && !cpu->trap()) {
 *     cpu->step();
 *     if (cpu->rvfi_valid()) {
 *       uint32_t pc = cpu->rvfi_pc_rdata();
 *       uint32_t insn = cpu->rvfi_insn();
 *       // Compare against golden model...
 *     }
 *   }
 * @endcode
 */
class CpuIface {
public:
  /**
   * @brief Virtual destructor for proper cleanup of derived classes
   * 
   * Example:
   * @code
   *   std::unique_ptr<CpuIface> cpu = std::make_unique<CpuPicorv32>();
   *   // Automatic cleanup when unique_ptr goes out of scope.
   * @endcode
   */
  virtual ~CpuIface() = default;
  
  /**
   * @brief Reset the CPU to initial state
   * 
   * Performs a full hardware reset of the CPU, clearing all registers,
   * pipeline state, and memory. After reset, the CPU should be ready
   * to accept new input and begin execution from the reset vector.
   * 
   * @note Implementation-specific reset behavior (reset vector address,
   *       initial register values) should match the target hardware
   * 
   * Example:
   * @code
   *   cpu->reset();
   *   cpu->load_input(input);
   * @endcode
   */
  virtual void reset() = 0;
  
  /**
   * @brief Load fuzzer-generated input into CPU memory
   * 
   * Loads the instruction stream provided by the fuzzer into the CPU's
   * memory space at the configured base address. The input typically
   * consists of raw RISC-V machine code instructions.
   * 
   * @param in Vector of bytes containing the instruction stream to execute
   * 
   * @note The memory layout (base address, size limits) is typically
   *       configured through environment variables (RAM_BASE, RAM_SIZE)
   * @note Input validation (size limits) should be performed by the caller
   * 
   * Example:
   * @code
   *   std::vector<unsigned char> input = {0x13, 0x00, 0x00, 0x00}; // nop
   *   cpu->load_input(input);
   * @endcode
   */
  virtual void load_input(const std::vector<unsigned char>& in) = 0;
  
  /**
   * @brief Execute one clock cycle of the CPU
   * 
   * Advances the CPU simulation by one clock cycle, processing any
   * pending instructions, updating pipeline stages, and committing
   * architectural state changes. RVFI signals become valid during
   * this step when an instruction commits.
   * 
   * @note Call rvfi_valid() after stepping to check if new commit data is available
   * @see rvfi_valid()
   */

  virtual void step() = 0;
  
  /**
   * @brief Check if CPU has reached finish condition
   * 
   * Returns true if the CPU has encountered a clean exit condition,
   * such as executing a finish instruction or reaching a designated
   * exit stub. This indicates the program completed successfully.
   * 
   * @return true if CPU reached normal termination, false otherwise
   * 
   * @note Finish condition depends on exit stub implementation
   * @see DutExit.hpp for exit stub details
   * 
   * Example:
   * @code
   *   if (cpu->got_finish()) {
   *     hwfuzz::debug::logInfo("Program completed cleanly");
   *   }
   * @endcode
   */
  virtual bool got_finish() const = 0;
  
  /**
   * @brief Check if CPU has trapped (error condition)
   * 
   * Returns true if the CPU encountered an exception or trap that
   * indicates a bug or illegal operation (e.g., illegal instruction,
   * memory access violation, misaligned access).
   * 
   * @return true if CPU trapped/faulted, false if executing normally
   * 
   * @note Traps typically indicate interesting fuzzer findings
   * 
   * Example:
   * @code
   *   if (cpu->trap()) {
   *     logger.writeCrash("trap", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cycle, input);
   *   }
   * @endcode
   */
  virtual bool trap() const = 0;

  // ========================================================================
  // RVFI (RISC-V Formal Verification Interface) Accessors
  // ========================================================================
  // These methods expose instruction-level observability following the
  // RVFI standard. They should be queried after step() when rvfi_valid()
  // returns true, indicating an instruction has committed.
  
  /**
   * @brief Check if RVFI commit data is valid this cycle
   * 
   * Returns true when an instruction has committed during the last step()
   * and the RVFI signals contain valid architectural state updates.
   * Only query other RVFI accessors when this returns true.
   * 
   * @return true if RVFI data is valid this cycle, false otherwise
   * 
   * Example:
   * @code
   *   cpu->step();
   *   if (cpu->rvfi_valid()) {
   *     uint32_t pc = cpu->rvfi_pc_rdata();
   *     // Process committed instruction...
   *   }
   * @endcode
   */
  virtual bool     rvfi_valid() const = 0;
  
  /**
   * @brief Get the committed instruction encoding
   * 
   * Returns the 32-bit machine code of the instruction that just committed.
   * For compressed (16-bit) instructions, the encoding is zero-extended.
   * 
   * @return 32-bit instruction encoding (machine code)
   * 
   * Example:
   * @code
   *   uint32_t insn = cpu->rvfi_insn();
   *   if ((insn & 0x7F) == 0x13) {
   *     // This is an I-type arithmetic instruction
   *   }
   * @endcode
   */
  virtual uint32_t rvfi_insn() const = 0;
  
  /**
   * @brief Get the PC value before instruction execution
   * 
   * Returns the program counter value at which the committed instruction
   * was fetched (the "read" PC value before execution).
   * 
   * @return PC address before instruction execution
   * 
   * Example:
   * @code
   *   uint32_t pc = cpu->rvfi_pc_rdata();
   *   dut_state.last_pc = pc;
   * @endcode
   */
  virtual uint32_t rvfi_pc_rdata() const = 0;
  
  /**
   * @brief Get the PC value after instruction execution
   * 
   * Returns the next program counter value after the instruction completes
   * (the "write" PC value after execution). For sequential instructions,
   * this is pc_rdata + instruction_length. For branches/jumps, this is
   * the target address.
   * 
   * @return PC address after instruction execution (next PC)
   * 
   * Example:
   * @code
   *   uint32_t pc_before = cpu->rvfi_pc_rdata();
   *   uint32_t pc_after = cpu->rvfi_pc_wdata();
   *   if (pc_after != pc_before + 4) {
   *     // Control flow instruction (branch/jump)
   *   }
   * @endcode
   */
  virtual uint32_t rvfi_pc_wdata() const = 0;
  
  /**
   * @brief Get the destination register address
   * 
   * Returns the architectural register number (0-31) that was written
   * by this instruction. Returns 0 if no register was written.
   * 
   * @return Destination register number (0-31), or 0 if no writeback
   * 
   * @note x0 is hardwired to zero in RISC-V; writes to x0 have no effect
   * 
   * Example:
   * @code
   *   uint32_t rd = cpu->rvfi_rd_addr();
   *   if (rd != 0) shadow_regs[rd] = cpu->rvfi_rd_wdata();
   * @endcode
   */
  virtual uint32_t rvfi_rd_addr() const = 0;
  
  /**
   * @brief Get the value written to the destination register
   * 
   * Returns the data value that was written to the destination register
   * specified by rvfi_rd_addr(). Only meaningful when rd_addr is non-zero.
   * 
   * @return 32-bit value written to destination register
   * 
   * Example:
   * @code
   *   if (cpu->rvfi_rd_addr() != 0) {
   *     uint32_t rd_val = cpu->rvfi_rd_wdata();
   *     uint32_t rd_num = cpu->rvfi_rd_addr();
   *     printf("x%u <= 0x%08x\n", rd_num, rd_val);
   *   }
   * @endcode
   */
  virtual uint32_t rvfi_rd_wdata() const = 0;
  
  /**
   * @brief Get the memory address accessed by this instruction
   * 
   * Returns the memory address that was accessed if this instruction
   * performed a load or store operation. Returns 0 if no memory access.
   * 
   * @return Memory address accessed, or 0 if no memory operation
   * 
   * @see rvfi_mem_rmask(), rvfi_mem_wmask()
   * 
   * Example:
   * @code
   *   if (cpu->rvfi_mem_wmask()) {
   *     crash_mem_addr = cpu->rvfi_mem_addr();
   *   }
   * @endcode
   */
  virtual uint32_t rvfi_mem_addr() const = 0;
  
  /**
   * @brief Get the memory read mask
   * 
   * Returns a byte-level mask indicating which bytes were read from memory.
   * Each bit corresponds to one byte at mem_addr. For example, 0x0F indicates
   * a 4-byte (word) read, 0x03 indicates a 2-byte (halfword) read.
   * 
   * @return Byte-level read mask (0 if no read occurred)
   * 
   * Example:
   * @code
   *   uint32_t rmask = cpu->rvfi_mem_rmask();
   *   if (rmask == 0x01) {
   *     // Byte read (LB/LBU)
   *   } else if (rmask == 0x03) {
   *     // Halfword read (LH/LHU)
   *   } else if (rmask == 0x0F) {
   *     // Word read (LW)
   *   }
   * @endcode
   */
  virtual uint32_t rvfi_mem_rmask() const = 0;
  
  /**
   * @brief Get the memory write mask
   * 
   * Returns a byte-level mask indicating which bytes were written to memory.
   * Each bit corresponds to one byte at mem_addr. For example, 0x0F indicates
   * a 4-byte (word) write, 0x01 indicates a 1-byte write.
   * 
   * @return Byte-level write mask (0 if no write occurred)
   * 
   * @see rvfi_mem_rmask() for mask interpretation
   * 
   * Example:
   * @code
   *   if (cpu->rvfi_mem_wmask() == 0xF) {
   *     // Full word store
   *   }
   * @endcode
   */
  virtual uint32_t rvfi_mem_wmask() const = 0;
  
  // ========================================================================
  // Optional RVFI CSR (Control and Status Register) Tracking
  // ========================================================================
  // The following methods expose changes to performance counters (mcycle,
  // minstret). Default implementations return 0, indicating no CSR tracking.
  // Override in derived classes if CSR observability is available.
  
  /**
   * @brief Get the mcycle CSR write mask (optional)
   * 
   * Returns a 64-bit mask indicating if the mcycle counter was updated.
   * Non-zero indicates mcycle was written; zero indicates no update.
   * 
   * @return Write mask for mcycle CSR (default: 0 = not tracked)
   * 
   * Example:
   * @code
   *   if (cpu->rvfi_csr_mcycle_wmask()) {
   *     last_mcycle = cpu->rvfi_csr_mcycle_wdata();
   *   }
   * @endcode
   */
  virtual uint64_t rvfi_csr_mcycle_wmask() const { return 0; }
  
  /**
   * @brief Get the new value of mcycle CSR (optional)
   * 
   * Returns the updated value of the mcycle performance counter if it
   * was written this cycle. Only meaningful if wmask is non-zero.
   * 
   * @return New mcycle counter value (default: 0 = not tracked)
   * 
   * Example:
   * @code
   *   uint64_t mcycle = cpu->rvfi_csr_mcycle_wdata();
   * @endcode
   */
  virtual uint64_t rvfi_csr_mcycle_wdata() const { return 0; }
  
  /**
   * @brief Get the minstret CSR write mask (optional)
   * 
   * Returns a 64-bit mask indicating if the minstret counter was updated.
   * Non-zero indicates minstret was written; zero indicates no update.
   * 
   * @return Write mask for minstret CSR (default: 0 = not tracked)
   * 
   * Example:
   * @code
   *   if (cpu->rvfi_csr_minstret_wmask()) {
   *     shadow_minstret = cpu->rvfi_csr_minstret_wdata();
   *   }
   * @endcode
   */
  virtual uint64_t rvfi_csr_minstret_wmask() const { return 0; }
  
  /**
   * @brief Get the new value of minstret CSR (optional)
   * 
   * Returns the updated value of the minstret (instructions retired)
   * performance counter if it was written this cycle.
   * 
   * @return New minstret counter value (default: 0 = not tracked)
   * 
   * Example:
   * @code
   *   uint64_t retired = cpu->rvfi_csr_minstret_wdata();
   * @endcode
   */
  virtual uint64_t rvfi_csr_minstret_wdata() const { return 0; }
};
