#pragma once

//
// SpikeProcess — minimal wrapper around the Spike simulator (-l) that:
//
// 1) Launches Spike as a subprocess (using popen) with logging enabled (-l),
//    optionally via a Proxy Kernel (pk), and with a specific ISA string.
// 2) Streams Spike's raw textual output and writes it to a single log file.
//    The log is kept as raw as possible, but grouped per instruction between
//    simple START/END markers for readability during triage.
// 3) Parses Spike's "commit" lines to extract per-instruction information and
//    converts them to a CommitRec for golden comparison against the DUT.
//
// This class focuses on being robust and simple: no fancy decoding beyond what
// is needed to (best-effort) pick up PC, rd writeback and obvious memory ops.
// The raw log remains your source of truth for debugging.
//

#include "Trace.hpp"
#include <boost/process.hpp>
#include <cstdio>
#include <memory>
#include <optional>
#include <regex>
#include <string>

// Simple Spike process wrapper that runs `spike -l --isa=<isa> <elf>` and
// parses commit lines to produce CommitRec records. This is a best-effort
// parser and may need adjustment for your spike version and log format.
class SpikeProcess {
public:
  // Construct an empty SpikeProcess. No external resources are acquired
  // until start() is called.
  SpikeProcess() = default;
  // Destructor closes the Spike process and the log file if they are open.
  ~SpikeProcess() { stop(); }

  // set_log_path
  //   Configure where to write Spike's raw output. If non-empty, a file will
  //   be opened on start() in append mode (never truncates), and set to
  //   line-buffering so entries show up live. If the file cannot be opened,
  //   the run continues without a file.
  void set_log_path(const std::string& p) { log_path_ = p; }

  // Expose command and log file path for diagnostics
  //   command(): returns the exact shell command used to start Spike. Useful
  //              for manual reproduction.
  //   log_path(): returns the path previously set by set_log_path().
  const std::string& command() const { return spike_cmd_; }
  const std::string& log_path() const { return log_path_; }
  bool saw_fatal_trap() const { return fatal_trap_seen_; }
  const std::string& fatal_trap_summary() const { return fatal_trap_summary_; }

  // Process status helpers (valid after spike stops or start() failure)
  //   has_status(): true when the last run produced a wait status (after stop)
  //   raw_status(): raw integer status from wait (platform-specific)
  //   exited()    : true if the process exited normally
  //   exit_code() : valid only if exited()==true; otherwise -1
  //   signaled()  : true if the process was terminated by a signal
  //   term_signal(): valid only if signaled()==true; otherwise -1
  //   last_error(): a short string describing a start() failure (e.g., spawn)
  //   status_string(): human-readable one-line summary of the last status
  bool has_status() const { return status_valid_; }
  int raw_status() const { return last_status_; }
  bool exited() const { return status_valid_ && WIFEXITED(last_status_); }
  int exit_code() const { return exited() ? WEXITSTATUS(last_status_) : -1; }
  bool signaled() const { return status_valid_ && WIFSIGNALED(last_status_); }
  int term_signal() const { return signaled() ? WTERMSIG(last_status_) : -1; }
  const std::string& last_error() const { return start_error_; }
  std::string status_string() const;

  // Start spike; elf_path must be readable. isa is like "rv32imc".
  //   spike_bin: absolute path to Spike executable (e.g., /opt/riscv/bin/spike)
  //   elf_path : path to ELF to run (DUT input wrapped into an ELF container)
  //   isa      : ISA string for Spike (e.g., "rv32imc")
  //   pk_bin   : optional path to Proxy Kernel; if provided, Spike runs as:
  //               spike -l --isa=<isa> <pk_bin> <elf_path>
  // Behavior:
  //   - Terminates any previously running Spike instance.
  //   - Builds the exact shell command string and launches it via boost::process.
  //   - Redirects stderr to stdout so the raw log captures everything.
  //   - Opens log file if configured and sets it to line-buffering.
  //   - Prepares regular expressions used by next_commit().
  // Returns:
  //   true on success (process running), false on failure (with last_error()).
  bool start(const std::string& spike_bin, const std::string& elf_path,
             const std::string& isa = "rv32imc",
             const std::string& pk_bin = std::string());

  // stop
  //   Waits for the Spike subprocess if running, records its wait status
  //   (available via status helpers), releases the output stream, and
  //   closes the raw log file if open.
  void stop();

  // Blocking: read until next commit found; returns true and fills rec.
  //   next_commit(rec):
  //     - Reads lines from Spike's output until a "commit" line is found.
  //     - Parses that line to extract PC and instruction value.
  //     - Reads a short window of subsequent lines to capture any register
  //       writes or obvious memory accesses, without consuming the next commit.
  //     - Writes a grouped chunk of all raw lines for this instruction to the
  //       log file, enclosed by simple START/END markers with the instruction
  //       index and decoded PC/insn for quick scanning.
  //     - Populates the provided CommitRec (pc, insn, rd writeback, basic mem
  //       info when available). Fields are best-effort and may be zero if not
  //       present in the log output.
  //   Returns:
  //     true if a commit was parsed into rec; false on EOF or error (Spike
  //     likely terminated). After false, stop() is called internally.
  bool next_commit(CommitRec& rec);

private:
  // The exact command used to launch Spike (for reproduction).
  std::string spike_cmd_;
  // Child process handle and merged stdout/stderr stream (boost::process).
  std::optional<boost::process::child> child_;
  std::unique_ptr<boost::process::ipstream> child_stream_;
  // Path to the raw log file (if set) and its FILE* handle.
  std::string log_path_;
  FILE* log_file_ = nullptr;
  bool fatal_trap_seen_ = false;
  std::string fatal_trap_summary_;
  // Regexes to detect commit lines and register writes in Spike logs.
  std::regex commit_re_;
  std::regex reg_write_re_;
  std::regex simple_reg_re_;
  // Last observed wait status from wait(), and flags for status validity.
  int last_status_ = 0;
  bool status_valid_ = false;
  // Short description of a startup failure (e.g., popen error message).
  std::string start_error_;
  // Line buffered between calls when we've already seen the next commit line.
  std::string pending_line_;
  // Monotonic instruction counter used in the per-instruction log headers.
  size_t instr_index_ = 0;
};
