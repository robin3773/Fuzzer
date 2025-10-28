#include "spike_process.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <regex>
#include <sstream>
#include <vector>
#include <sys/wait.h>

std::string SpikeProcess::status_string() const {
  if (!status_valid_) return std::string("unknown");
  if (WIFEXITED(last_status_)) {
    return std::string("exited ") + std::to_string(WEXITSTATUS(last_status_));
  }
  if (WIFSIGNALED(last_status_)) {
    return std::string("signaled ") + std::to_string(WTERMSIG(last_status_));
  }
  return std::string("status ") + std::to_string(last_status_);
}

bool SpikeProcess::start(const std::string& spike_bin, const std::string& elf_path,
                         const std::string& isa,
                         const std::string& pk_bin) {
  stop();
  status_valid_ = false;
  last_status_ = 0;
  start_error_.clear();
  pending_line_.clear();
  instr_index_ = 0;
  if (!pk_bin.empty()) {
    spike_cmd_ = spike_bin + " -l --isa=" + isa + " " + pk_bin + " " + elf_path + " 2>&1";
  } else {
    spike_cmd_ = spike_bin + " -l --isa=" + isa + " " + elf_path + " 2>&1";
  }
  pipe_ = popen(spike_cmd_.c_str(), "r");
  if (!pipe_) {
    start_error_ = std::string("popen failed: ") + std::strerror(errno);
    return false;
  }
  if (!log_path_.empty()) {
    log_file_ = std::fopen(log_path_.c_str(), "w");
    if (log_file_) {
      // Line-buffer the file so it shows up while running
      setvbuf(log_file_, nullptr, _IOLBF, 0);
    }
    // If fopen failed, continue without file gracefully
  }
  // prepare regexes
  commit_re_ = std::regex("core\\s+0:\\s+0x([0-9a-fA-F]+)\\s+\\(0x([0-9a-fA-F]+)\\)");
  reg_write_re_ = std::regex("\\b(W|W0|W1)\"?\\s*x([0-9]+)\\s*[:<=-]+\\s*0x([0-9a-fA-F]+)");
  simple_reg_re_ = std::regex("\\bx([0-9]+)\\)\\s*:=\\s*0x([0-9a-fA-F]+)");
  return true;
}

void SpikeProcess::stop() {
  if (pipe_) {
    int st = pclose(pipe_);
    last_status_ = st;
    status_valid_ = true;
    pipe_ = nullptr;
  }
  if (log_file_) { std::fclose(log_file_); log_file_ = nullptr; }
}

bool SpikeProcess::next_commit(CommitRec& rec) {
  if (!pipe_) return false;
  char* line = nullptr; size_t cap = 0;
  // First, find the next commit line; pass-through any preamble lines to log raw
  for (;;) {
    std::string s;
    if (!pending_line_.empty()) {
      s.swap(pending_line_);
    } else {
      ssize_t n = getline(&line, &cap, pipe_);
      if (n <= 0) break;
      s.assign(line, (size_t)n);
    }
    // try to match commit line
    std::smatch m;
    if (std::regex_search(s, m, commit_re_)) {
      // We'll accumulate the raw lines for this instruction to a chunk and emit with separators
      std::vector<std::string> chunk;
      {
        std::ostringstream hdr;
        hdr << "----- SPIKE INSTR #" << (instr_index_ + 1)
            << " pc=0x" << std::hex << m[1].str() << " insn=0x" << m[2].str() << " -----\n";
        chunk.push_back(hdr.str());
      }
      chunk.push_back(s);
      // parse pc and insn
      rec.pc_w = (uint32_t)std::stoul(m[1].str(), nullptr, 16);
      rec.insn = (uint32_t)std::stoul(m[2].str(), nullptr, 16);
      // reset other fields; subsequent lines may contain register/mem writes
      rec.rd_addr = 0; rec.rd_wdata = 0; rec.mem_addr = 0; rec.mem_rmask = 0; rec.mem_wmask = 0; rec.trap = 0;
      rec.mem_is_load = rec.mem_is_store = 0; rec.mem_wdata = rec.mem_rdata = 0;
      // try to collect subsequent lines for register writes (non-blocking read of a few lines)
      // Note: spike prints reg writes on subsequent lines; we'll attempt to parse up to 16 lines
      for (int i = 0; i < 16; ++i) {
        ssize_t n2 = getline(&line, &cap, pipe_);
        if (n2 <= 0) break;
        std::string s2(line, (size_t)n2);
        // If next commit appears, stash it for next call and stop here
        std::smatch mnext;
        if (std::regex_search(s2, mnext, commit_re_)) {
          pending_line_ = s2;
          break;
        }
        chunk.push_back(s2);
        std::smatch mr;
        if (std::regex_search(s2, mr, simple_reg_re_)) {
          int r = std::stoi(mr[1].str());
          uint32_t v = (uint32_t)std::stoul(mr[2].str(), nullptr, 16);
          rec.rd_addr = r; rec.rd_wdata = v;
        } else if (std::regex_search(s2, mr, reg_write_re_)) {
          int r = std::stoi(mr[2].str());
          uint32_t v = (uint32_t)std::stoul(mr[3].str(), nullptr, 16);
          rec.rd_addr = r; rec.rd_wdata = v;
        } else {
          // Best-effort memory access parsing; formats vary across spike versions.
          // Try to catch common patterns like:
          //   mem [0xADDR] = 0xDATA   (store)
          //   mem [0xADDR] -> 0xDATA  (load)
          std::smatch mm;
          static const std::regex mem_store_re("\\bmem\\s*\\[?0x([0-9a-fA-F]+)\\]?\\s*(?:=|<-|:)\\s*0x([0-9a-fA-F]+)");
          static const std::regex mem_load_re ("\\bmem\\s*\\[?0x([0-9a-fA-F]+)\\]?\\s*(?:->|=>)\\s*0x([0-9a-fA-F]+)");
          if (std::regex_search(s2, mm, mem_store_re)) {
            rec.mem_addr = (uint32_t)std::stoul(mm[1].str(), nullptr, 16);
            rec.mem_wdata = (uint32_t)std::stoul(mm[2].str(), nullptr, 16);
            rec.mem_is_store = 1;
          } else if (std::regex_search(s2, mm, mem_load_re)) {
            rec.mem_addr = (uint32_t)std::stoul(mm[1].str(), nullptr, 16);
            rec.mem_rdata = (uint32_t)std::stoul(mm[2].str(), nullptr, 16);
            rec.mem_is_load = 1;
          }
        }
        // break early on blank line
        if (s2.size() <= 1) break;
      }
      // footer separator
      chunk.push_back("----- END SPIKE INSTR -----\n");
      // emit the chunk to the raw log file
      if (log_file_) {
        for (const auto& ln : chunk) {
          std::fwrite(ln.data(), 1, ln.size(), log_file_);
        }
        std::fflush(log_file_);
      }
      instr_index_++;
      if (line) free(line);
      return true;
    }
    // Not a commit: write through raw line (preamble / misc output)
    if (log_file_) { std::fwrite(s.data(), 1, s.size(), log_file_); std::fflush(log_file_); }
  }
  if (line) free(line);
  // EOF or read error: close and record status so caller can inspect
  stop();
  return false;
}
