/**
 * @file Trace.hpp
 * @brief Instruction-level trace recording for differential testing
 * 
 * This file provides facilities for recording per-instruction commit records
 * during CPU execution. Traces are written in CSV format for easy analysis,
 * comparison, and post-mortem debugging. Both DUT and golden model traces
 * use the same CommitRec format for straightforward differential analysis.
 */

#pragma once

#include "Utils.hpp"

#include <string>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>

/**
 * @struct CommitRec
 * @brief Per-instruction commit record for trace comparison
 * 
 * CommitRec captures the architectural state changes caused by a single
 * committed instruction. This structure is used for both DUT (Device Under
 * Test) and golden model traces, enabling cycle-accurate differential testing.
 * 
 * The record follows RVFI (RISC-V Formal Verification Interface) conventions
 * for observing instruction execution, tracking PC changes, register writes,
 * memory operations, and exception conditions.
 * 
 * Example usage:
 * @code
 *   CommitRec rec;
 *   rec.pc_r = 0x80000000;      // Instruction fetch address
 *   rec.pc_w = 0x80000004;      // Next PC (sequential)
 *   rec.insn = 0x00000013;      // nop instruction
 *   rec.rd_addr = 0;            // No destination register
 *   rec.trap = 0;               // No exception
 *   trace_writer.write(rec);
 * @endcode
 */
struct CommitRec {
	/**
	 * @brief Program counter before instruction execution (fetch address)
	 * 
	 * The PC value at which the instruction was fetched. For sequential
	 * instructions, pc_r + 4 (or + 2 for compressed) equals pc_w.
	 * 
	 * Example: 0x80000000 → instruction fetched from this address
	 */
	uint32_t pc_r      = 0;
	
	/**
	 * @brief Program counter after instruction execution (next PC)
	 * 
	 * The PC value after instruction completion. For branches and jumps,
	 * this is the target address. For sequential instructions, this is
	 * pc_r + instruction_length.
	 * 
	 * Example: 0x80000004 → next instruction will execute from here
	 */
	uint32_t pc_w      = 0;
	
	/**
	 * @brief Instruction encoding (machine code)
	 * 
	 * The 32-bit encoding of the executed instruction. For compressed
	 * (16-bit) instructions, the value is zero-extended.
	 * 
	 * Example: 0x00000013 → addi x0, x0, 0 (nop)
	 */
	uint32_t insn      = 0;
	
	/**
	 * @brief Destination register address (0-31)
	 * 
	 * The architectural register number that was written by this instruction.
	 * Zero indicates no register was written (or write to x0, which is hardwired
	 * to zero and has no effect).
	 * 
	 * Example: 5 → register x5 (t0) was written
	 */
	uint32_t rd_addr   = 0;
	
	/**
	 * @brief Value written to destination register
	 * 
	 * The data value that was written to the register specified by rd_addr.
	 * Only meaningful when rd_addr is non-zero.
	 * 
	 * Example: 0x12345678 → this value was written to the destination register
	 */
	uint32_t rd_wdata  = 0;
	
	/**
	 * @brief Memory address accessed by this instruction
	 * 
	 * The memory address that was accessed if this instruction performed
	 * a load or store. Zero if no memory operation occurred.
	 * 
	 * Example: 0x80001000 → memory access at this address
	 */
	uint32_t mem_addr  = 0;
	
	/**
	 * @brief Memory read mask (byte-level)
	 * 
	 * Bitmask indicating which bytes were read from memory. Each bit
	 * corresponds to one byte at mem_addr.
	 * - 0x0: No read
	 * - 0x1: Byte read (LB/LBU)
	 * - 0x3: Halfword read (LH/LHU)
	 * - 0xF: Word read (LW)
	 * 
	 * Example: 0x3 → two bytes were read (halfword access)
	 */
	uint32_t mem_rmask = 0;
	
	/**
	 * @brief Memory write mask (byte-level)
	 * 
	 * Bitmask indicating which bytes were written to memory. Each bit
	 * corresponds to one byte at mem_addr.
	 * - 0x0: No write
	 * - 0x1: Byte write (SB)
	 * - 0x3: Halfword write (SH)
	 * - 0xF: Word write (SW)
	 * 
	 * Example: 0xF → four bytes were written (word access)
	 */
	uint32_t mem_wmask = 0;
	
	/**
	 * @brief Trap/exception indicator
	 * 
	 * Non-zero if the instruction triggered a trap or exception.
	 * Zero indicates normal execution without faults.
	 * 
	 * Example: 1 → trap occurred (illegal instruction, misaligned access, etc.)
	 */
	uint32_t trap      = 0;
	
	// ========================================================================
	// Optional Extended Fields
	// ========================================================================
	// These fields provide additional detail but are not currently written
	// to CSV traces. They can be used for internal bookkeeping and enhanced
	// debugging when needed.
	
	/**
	 * @brief Data written to memory (if store occurred)
	 * 
	 * The actual data value that was written to memory. Only valid when
	 * mem_wmask is non-zero.
	 * 
	 * @note Not currently emitted in CSV output
	 */
	uint32_t mem_wdata = 0;
	
	/**
	 * @brief Data read from memory (if load occurred)
	 * 
	 * The actual data value that was read from memory. Only valid when
	 * mem_rmask is non-zero.
	 * 
	 * @note Not currently emitted in CSV output
	 */
	uint32_t mem_rdata = 0;
	
	/**
	 * @brief Flag indicating this was a load instruction
	 * 
	 * Set to 1 if the instruction performed a memory read (LB, LH, LW, etc.).
	 * 
	 * @note Not currently emitted in CSV output
	 */
	uint8_t  mem_is_load  = 0;
	
	/**
	 * @brief Flag indicating this was a store instruction
	 * 
	 * Set to 1 if the instruction performed a memory write (SB, SH, SW, etc.).
	 * 
	 * @note Not currently emitted in CSV output
	 */
	uint8_t  mem_is_store = 0;
};

/**
 * @class TraceWriter
 * @brief CSV trace file writer for instruction commit records
 * 
 * TraceWriter provides a simple interface for recording instruction-level
 * traces to CSV files. Each commit record is written as a single CSV line
 * with fields for PC, instruction encoding, register writes, and memory
 * operations.
 * 
 * The CSV format is designed for:
 * - Easy parsing by analysis scripts (Python, awk, etc.)
 * - Human readability for quick inspection
 * - Differential comparison between DUT and golden model traces
 * - Post-mortem debugging of divergences
 * 
 * CSV columns (in order):
 * 1. pc_r: Fetch PC (hex)
 * 2. pc_w: Next PC (hex)
 * 3. insn: Instruction encoding (hex)
 * 4. rd_addr: Destination register (decimal)
 * 5. rd_wdata: Register write value (hex)
 * 6. mem_addr: Memory address (hex)
 * 7. mem_rmask: Read mask (hex)
 * 8. mem_wmask: Write mask (hex)
 * 9. trap: Exception flag (decimal)
 * 
 * Example CSV output:
 * @code
 *   #pc_r,pc_w,insn,rd_addr,rd_wdata,mem_addr,mem_rmask,mem_wmask,trap
 *   0x80000000,0x80000004,0x00000013,0,0x00000000,0x00000000,0x0,0x0,0
 *   0x80000004,0x80000008,0x00108093,1,0x00000001,0x00000000,0x0,0x0,0
 *   0x80000008,0x8000000c,0x00208113,2,0x00000002,0x00000000,0x0,0x0,0
 * @endcode
 * 
 * Example usage:
 * @code
 *   TraceWriter dut_trace("workdir/traces");
 *   TraceWriter golden_trace;
 *   golden_trace.open_with_basename("workdir/traces", "golden.trace");
 *   
 *   while (cpu->rvfi_valid()) {
 *     CommitRec rec;
 *     rec.pc_r = cpu->rvfi_pc_rdata();
 *     rec.insn = cpu->rvfi_insn();
 *     dut_trace.write(rec);
 *   }
 * @endcode
 */
class TraceWriter {
public:
	/**
	 * @brief Default constructor (no file opened)
	 * 
	 * Creates an inactive TraceWriter. Call open() to start writing.
	 * 
	 * Example:
	 * @code
	 *   TraceWriter trace;
	 *   trace.open("workdir/traces");
	 * @endcode
	 */
	TraceWriter() = default;
	
	/**
	 * @brief Construct and open trace file with default name
	 * 
	 * Creates the trace directory if needed and opens "dut.trace" for writing.
	 * 
	 * @param dir Directory path for trace file
	 * 
	 * Example:
	 * @code
	 *   TraceWriter trace("workdir/traces");
	 *   // Creates workdir/traces/dut.trace
	 * @endcode
	 */
	explicit TraceWriter(const std::string& dir) { open(dir); }
	
	/**
	 * @brief Destructor closes file descriptor if open
	 * 
	 * Example:
	 * @code
	 *   {
	 *     TraceWriter trace("workdir/traces");
	 *     trace.write(rec);
	 *   } // File descriptor closed automatically.
	 * @endcode
	 */
	~TraceWriter() { if (fd_ >= 0) ::close(fd_); }

	/**
	 * @brief Open trace file with default name "dut.trace"
	 * 
	 * Creates the directory if it doesn't exist, then opens/creates the
	 * trace file with CSV header. Any existing file is truncated.
	 * 
	 * @param dir Directory path for trace file
	 * @return true if file opened successfully, false on error
	 * 
	 * Example:
	 * @code
	 *   TraceWriter trace;
	 *   if (!trace.open("workdir/traces")) {
	 *     std::cerr << "Failed to open trace file\n";
	 *   }
	 * @endcode
	 */
	bool open(const std::string& dir) {
		return open_with_basename(dir, "dut.trace");
	}

	/**
	 * @brief Open trace file with custom basename
	 * 
	 * Allows specifying a custom filename (e.g., "golden.trace", "ref.trace")
	 * instead of the default "dut.trace". Useful for creating separate traces
	 * for DUT and golden model.
	 * 
	 * @param dir Directory path for trace file
	 * @param base Filename to create in the directory
	 * @return true if file opened successfully, false on error
	 * 
	 * Example:
	 * @code
	 *   TraceWriter golden_trace;
	 *   golden_trace.open_with_basename("workdir/traces", "golden.trace");
	 *   // Creates: workdir/traces/golden.trace
	 * @endcode
	 */
	bool open_with_basename(const std::string& dir, const std::string& base) {
		if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
		utils::ensure_dir(dir);
		path_ = dir + "/" + base;
		int fd = ::open(path_.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
		if (fd < 0) return false;
		const char* hdr = "#pc_r,pc_w,insn,rd_addr,rd_wdata,mem_addr,mem_rmask,mem_wmask,trap\n";
		if (::write(fd, hdr, (ssize_t)std::strlen(hdr)) < 0) { ::close(fd); return false; }
		fd_ = fd;
		return true;
	}

	/**
	 * @brief Write a commit record to the trace file
	 * 
	 * Formats the record as a CSV line and writes it to the file. If the
	 * file is not open, this is a no-op. Writing is unbuffered to ensure
	 * traces are preserved even if the fuzzer crashes.
	 * 
	 * @param r CommitRec containing instruction execution details
	 * 
	 * CSV format produced:
	 * - Hex values: 0x prefix with 8 hex digits (uppercase X not used)
	 * - Decimal values: No prefix
	 * - Fields separated by commas
	 * - Line terminated with newline
	 * 
	 * Example:
	 * @code
	 *   CommitRec rec;
	 *   rec.pc_r = 0x80000000;
	 *   rec.pc_w = 0x80000004;
	 *   rec.insn = 0x00000013;  // nop
	 *   rec.rd_addr = 0;
	 *   trace.write(rec);
	 *   // Writes: "0x80000000,0x80000004,0x00000013,0,0x00000000,0x00000000,0x0,0x0,0\n"
	 * @endcode
	 */
	void write(const CommitRec& r) {
		if (fd_ < 0) return;
		char line[256];
		// Use hex for wide fields; rd_addr/trap in decimal for readability
		int n = std::snprintf(line, sizeof(line),
										"0x%08x,0x%08x,0x%08x,%u,0x%08x,0x%08x,0x%x,0x%x,%u\n",
										r.pc_r, r.pc_w, r.insn,
										(unsigned)r.rd_addr, r.rd_wdata,
										r.mem_addr, r.mem_rmask, r.mem_wmask,
										(unsigned)r.trap);
		if (n > 0) {
			utils::safe_write_all(fd_, line, (size_t)n);
		}
	}

	/**
	 * @brief Get the full path of the open trace file
	 * 
	 * Returns the absolute path to the trace file that was opened.
	 * Useful for logging and reporting where traces are stored.
	 * 
	 * @return Full filesystem path to trace file (empty if not open)
	 * 
	 * Example:
	 * @code
	 *   TraceWriter trace("workdir/traces");
	 *   std::cout << "Trace file: " << trace.path() << std::endl;
	 *   // Output: "Trace file: workdir/traces/dut.trace"
	 * @endcode
	 */
	const std::string& path() const { return path_; }

private:
	/**
	 * @brief File descriptor for open trace file (-1 if closed)
	 */
	int fd_ = -1;
	
	/**
	 * @brief Full path to the trace file
	 */
	std::string path_;
};
