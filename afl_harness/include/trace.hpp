#pragma once

#include "utils.hpp"

#include <string>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>

// Minimal per-commit record used for DUT and Golden traces
struct CommitRec {
	uint32_t pc_r      = 0;
	uint32_t pc_w      = 0;
	uint32_t insn      = 0;
	uint32_t rd_addr   = 0;
	uint32_t rd_wdata  = 0;
	uint32_t mem_addr  = 0;
	uint32_t mem_rmask = 0;
	uint32_t mem_wmask = 0;
	uint32_t trap      = 0;
	// Optional extended fields (not emitted in current CSV):
	uint32_t mem_wdata = 0;
	uint32_t mem_rdata = 0;
	uint8_t  mem_is_load  = 0;
	uint8_t  mem_is_store = 0;
};

// Simple CSV trace writer; default writes to dut.trace in a directory,
// but can also open with a custom basename (e.g., golden.trace).
class TraceWriter {
public:
	TraceWriter() = default;
	explicit TraceWriter(const std::string& dir) { open(dir); }
	~TraceWriter() { if (fd_ >= 0) ::close(fd_); }

	bool open(const std::string& dir) {
		return open_with_basename(dir, "dut.trace");
	}

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

	const std::string& path() const { return path_; }

private:
	int fd_ = -1;
	std::string path_;
};
