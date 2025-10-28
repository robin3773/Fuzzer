#include "harness_config.hpp"
#include "utils.hpp"
#include "cpu_iface.hpp"
#include "crash_logger.hpp"
#include "trace.hpp"
#include "spike_process.hpp"

#include "verilated.h"
#include <csignal>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static volatile sig_atomic_t g_sig = 0;
static void sig_handler(int s) { g_sig = s; }

static void install_signal_handlers() {
  struct sigaction sa;
  std::memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sig_handler;
  sigaction(SIGSEGV, &sa, nullptr);
  sigaction(SIGILL,  &sa, nullptr);
  sigaction(SIGBUS,  &sa, nullptr);
  sigaction(SIGABRT, &sa, nullptr);
}

// Print last N lines of a text file to stderr (best-effort)
static void print_file_tail(const char* path, int max_lines = 50) {
  if (!path || !*path) return;
  int fd = ::open(path, O_RDONLY);
  if (fd < 0) return;
  struct stat st{};
  if (fstat(fd, &st) != 0) { ::close(fd); return; }
  off_t size = st.st_size;
  const size_t chunk = 4096;
  std::vector<char> buf;
  buf.reserve((size_t)std::min<off_t>(size, 64 * 1024));
  off_t pos = size;
  int lines = 0;
  while (pos > 0 && lines <= max_lines) {
    size_t toread = (size_t)std::min<off_t>(chunk, pos);
    pos -= toread;
    std::vector<char> tmp(toread);
    if (pread(fd, tmp.data(), toread, pos) != (ssize_t)toread) break;
    // scan backwards for newlines
    for (ssize_t i = (ssize_t)toread - 1; i >= 0; --i) {
      if (tmp[(size_t)i] == '\n') {
        lines++;
        if (lines > max_lines) {
          // include from next char after this newline
          size_t start = (size_t)i + 1;
          buf.insert(buf.begin(), tmp.begin() + (long)start, tmp.end());
          goto done_tail;
        }
      }
    }
    buf.insert(buf.begin(), tmp.begin(), tmp.end());
  }
done_tail:
  if (!buf.empty()) {
    fprintf(stderr, "----- spike.log (tail) -----\n");
    fwrite(buf.data(), 1, buf.size(), stderr);
    if (buf.back() != '\n') fputc('\n', stderr);
    fprintf(stderr, "----- end spike.log (tail) -----\n");
  }
  ::close(fd);
}

static void read_all_fd(int fd, std::vector<unsigned char>& out, size_t cap=1<<20) {
  out.clear();
  out.reserve(1024);
  for (;;) {
    if (out.size() >= cap) break;
    unsigned char buf[4096];
    size_t want = std::min(sizeof(buf), cap - out.size());
    ssize_t n = ::read(fd, buf, want);
    if (n <= 0) break;
    out.insert(out.end(), buf, buf + n);
  }
  if (out.empty()) { unsigned char b=0x13; out.push_back(b); } // ADDI x0,x0,0
}

extern "C" CpuIface* make_cpu();

namespace {

// Return true if crash was detected and logged
bool check_x0_write(const CpuIface* cpu, const CrashLogger& logger, unsigned cyc,
                    const std::vector<unsigned char>& input) {
  if (!cpu->rvfi_valid()) return false;
  uint32_t rd = cpu->rvfi_rd_addr();
  uint32_t w  = cpu->rvfi_rd_wdata();
  if (rd == 0 && w != 0) {
    logger.writeCrash("x0_write", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  return false;
}

bool check_pc_misaligned(const CpuIface* cpu, const CrashLogger& logger, unsigned cyc,
                         const std::vector<unsigned char>& input) {
  if (!cpu->rvfi_valid()) return false;
  uint32_t pcw = cpu->rvfi_pc_wdata();
  if ((pcw & 0x1) != 0) {
    logger.writeCrash("pc_misaligned", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  return false;
}

bool check_mem_align_load(const CpuIface* cpu, const CrashLogger& logger, unsigned cyc,
                          const std::vector<unsigned char>& input) {
  if (!cpu->rvfi_valid()) return false;
  uint32_t addr = cpu->rvfi_mem_addr();
  uint32_t mask = cpu->rvfi_mem_rmask() & 0xFu;
  if (!mask) return false;

  uint32_t off = addr & 0x3u;
  uint32_t contiguous = 0;
  for (uint32_t i = off; i < 4 && (mask & (1u << i)); ++i) contiguous++;
  bool is_contig = (mask == (0x1u << off)) ||
                   (contiguous == 2 && (mask == (0x3u << off))) ||
                   (contiguous == 4 && mask == 0xFu);
  if (!is_contig) {
    logger.writeCrash("mem_mask_irregular_load", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  if (contiguous >= 2 && (addr & 0x1)) {
    logger.writeCrash("mem_unaligned_load", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  if (contiguous >= 4 && (addr & 0x3)) {
    logger.writeCrash("mem_unaligned_load", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  return false;
}

bool check_mem_align_store(const CpuIface* cpu, const CrashLogger& logger, unsigned cyc,
                           const std::vector<unsigned char>& input) {
  if (!cpu->rvfi_valid()) return false;
  uint32_t addr = cpu->rvfi_mem_addr();
  uint32_t mask = cpu->rvfi_mem_wmask() & 0xFu;
  if (!mask) return false;

  uint32_t off = addr & 0x3u;
  uint32_t contiguous = 0;
  for (uint32_t i = off; i < 4 && (mask & (1u << i)); ++i) contiguous++;
  bool is_contig = (mask == (0x1u << off)) ||
                   (contiguous == 2 && (mask == (0x3u << off))) ||
                   (contiguous == 4 && mask == 0xFu);
  if (!is_contig) {
    logger.writeCrash("mem_mask_irregular_store", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  if (contiguous >= 2 && (addr & 0x1)) {
    logger.writeCrash("mem_unaligned_store", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  if (contiguous >= 4 && (addr & 0x3)) {
    logger.writeCrash("mem_unaligned_store", cpu->rvfi_pc_rdata(), cpu->rvfi_insn(), cyc, input);
    return true;
  }
  return false;
}

} // namespace

int main(int argc, char** argv) {
  HarnessConfig cfg;
  cfg.load_from_env();
  utils::ensure_dir(cfg.crash_dir);
  install_signal_handlers();

  Verilated::commandArgs(argc, argv);
#if VL_VER_MAJOR >= 5
  Verilated::randReset(0);
#else
  Verilated::randSeed(0);
#endif

  std::vector<unsigned char> input;
  if (argc > 1 && argv[1][0] != '-') {
    int fd = ::open(argv[1], O_RDONLY);
    if (fd >= 0) { read_all_fd(fd, input); ::close(fd); }
    else { read_all_fd(STDIN_FILENO, input); }
  } else {
    read_all_fd(STDIN_FILENO, input);
  }

  CpuIface* cpu = make_cpu();
  cpu->reset();
  cpu->load_input(input);

  CrashLogger logger(cfg);

  // Optional per-commit trace (enabled when TRACE_DIR env is set)
  TraceWriter tracer;
  TraceWriter golden_tracer;
  const char* trace_dir = std::getenv("TRACE_DIR");
  const char* trace_mode_env = std::getenv("TRACE_MODE");
  bool trace_enabled = true;
  if (trace_mode_env && (std::string(trace_mode_env) == "off" || std::string(trace_mode_env) == "0")) {
    trace_enabled = false;
  }
  if (trace_enabled && trace_dir && *trace_dir) {
    tracer.open(trace_dir);
  }

  // Golden model (Spike) integration (optional)
  SpikeProcess spike;
  bool use_golden = false;
  std::string spike_bin;
  std::string spike_isa = "rv32imc";
  std::string pk_bin;
  const char* spike_log_env = std::getenv("SPIKE_LOG_FILE");
  const char* golden_env_old = std::getenv("ENABLE_GOLDEN");
  const char* golden_mode_env = std::getenv("GOLDEN_MODE");
  const char* spike_env = std::getenv("SPIKE_BIN");
  const char* spike_isa_env = std::getenv("SPIKE_ISA");
  const char* pk_env = std::getenv("PK_BIN");
  std::string golden_mode = "live"; // default: keep per-instruction diff
  if (golden_mode_env && *golden_mode_env) golden_mode = std::string(golden_mode_env);
  // Back-compat: ENABLE_GOLDEN=spike implies GOLDEN_MODE=live
  if (golden_env_old && std::string(golden_env_old) == "spike") golden_mode = "live";
  if (golden_mode == "live" && spike_env && *spike_env) {
    spike_bin = std::string(spike_env);
    use_golden = true;
  }
  if (spike_isa_env && *spike_isa_env) spike_isa = std::string(spike_isa_env);
  if (pk_env && *pk_env) pk_bin = std::string(pk_env);
  if (golden_mode == "off" || golden_mode == "none" || golden_mode == "0") {
    use_golden = false;
  } else if (golden_mode == "batch" || golden_mode == "replay") {
    // Not implemented yet in-harness; will be handled by external tools.
    use_golden = false;
    printf("[HARNESS/GOLDEN] GOLDEN_MODE=%s requested; external replay/tools should be used.\n", golden_mode.c_str());
  }

  // Backend selection (future: fpga). Currently only 'verilator' is supported.
  const char* exec_backend_env = std::getenv("EXEC_BACKEND");
  std::string exec_backend = exec_backend_env && *exec_backend_env ? std::string(exec_backend_env) : std::string("verilator");
  if (exec_backend != "verilator") {
    printf("[HARNESS] EXEC_BACKEND=%s not supported in this build, defaulting to verilator.\n", exec_backend.c_str());
  }

  std::string tmp_elf;
  bool golden_ready = false;
  if (use_golden) {
    if (spike_log_env && *spike_log_env) spike.set_log_path(spike_log_env);
    // write input blob to a temp file and try to build a minimal ELF with objcopy
    char tmpbin[] = "/tmp/dut_in_XXXXXX.bin";
    int bfd = ::mkstemps(tmpbin, 4); // leaves suffix .bin
    if (bfd >= 0) {
      ::write(bfd, input.data(), input.size());
      ::close(bfd);
      // Prefer 32-bit objcopy; allow override via OBJCOPY_BIN; fallback to 64-bit if needed
      std::string objcopy_default32 = "riscv32-unknown-elf-objcopy";
      std::string objcopy_default64 = "riscv64-unknown-elf-objcopy";
      std::string objcopy = objcopy_default32;
      bool objcopy_forced = false;
      if (const char* oc = std::getenv("OBJCOPY_BIN")) { if (*oc) { objcopy = std::string(oc); objcopy_forced = true; } }
      std::string elfpath = std::string(tmpbin) + ".elf";
      auto build_cmd = [&](const std::string& oc){ return oc + " -I binary -O elf32-littleriscv -B riscv:rv32 --set-start 0x0 " + std::string(tmpbin) + " " + elfpath + " 2>/dev/null"; };
      std::string cmd = build_cmd(objcopy);
      int rc = ::system(cmd.c_str());
      if (rc == 0) {
        tmp_elf = elfpath;
        if (spike.start(spike_bin, tmp_elf, spike_isa, pk_bin)) {
          golden_ready = true;
        } else {
          fprintf(stderr,
                  "[HARNESS/GOLDEN] Failed to start Spike.\n  Command: %s\n  ELF: %s\n",
                  spike.command().c_str(), tmp_elf.c_str());
          if (spike_log_env && *spike_log_env) {
            fprintf(stderr, "  See Spike log: %s\n", spike_log_env);
            print_file_tail(spike_log_env, 60);
          }
        }
      } else {
        // try fallback to 64-bit objcopy even if forced 32-bit failed
        objcopy = objcopy_default64;
        cmd = build_cmd(objcopy);
        rc = ::system(cmd.c_str());
        if (rc == 0) {
          tmp_elf = elfpath;
          if (spike.start(spike_bin, tmp_elf, spike_isa, pk_bin)) {
            golden_ready = true;
          } else {
            fprintf(stderr,
                    "[HARNESS/GOLDEN] Spike failed to start with ELF after objcopy64.\n  Command: %s\n  ELF: %s\n",
                    spike.command().c_str(), tmp_elf.c_str());
            if (spike_log_env && *spike_log_env) {
              fprintf(stderr, "  See Spike log: %s\n", spike_log_env);
              print_file_tail(spike_log_env, 60);
            }
          }
        } else {
          printf("[HARNESS/GOLDEN] objcopy failed (forced or default). Tried 32 and 64. Disabling Spike golden.\n");
        }
      }
    }
    // If tracing is enabled, also prepare a golden.trace
    if (golden_ready && trace_enabled && trace_dir && *trace_dir) {
      golden_tracer.open_with_basename(trace_dir, "golden.trace");
    }
  }

  // Shadow regfiles for broader comparison (x0..x31). Enforce x0==0 invariant.
  uint32_t dut_regs[32] = {0};
  uint32_t gold_regs[32] = {0};
  // Shadow CSRs (best-effort): we track mcycle/minstret. For golden, assume +1 per commit.
  uint64_t dut_mcycle = 0, gold_mcycle = 0;
  uint64_t dut_minstret = 0, gold_minstret = 0;

  unsigned cyc = 0;
  for (; cyc < cfg.max_cycles && !cpu->got_finish(); ++cyc) {
    if (g_sig) {
      uint32_t pc   = cpu->rvfi_pc_rdata();
      uint32_t insn = cpu->rvfi_insn();
      logger.writeCrash(std::string("signal_") + std::to_string(g_sig),
                        pc, insn, cyc, input);
      _exit(126);
    }

    cpu->step();

    // On commit: record a trace row (if enabled) and optionally compare to golden
    if (cpu->rvfi_valid()) {
  CommitRec rec;
      rec.pc_r      = cpu->rvfi_pc_rdata();
      rec.pc_w      = cpu->rvfi_pc_wdata();
      rec.insn      = cpu->rvfi_insn();
      rec.rd_addr   = cpu->rvfi_rd_addr();
      rec.rd_wdata  = cpu->rvfi_rd_wdata();
      rec.mem_addr  = cpu->rvfi_mem_addr();
      rec.mem_rmask = cpu->rvfi_mem_rmask();
      rec.mem_wmask = cpu->rvfi_mem_wmask();
      rec.trap      = cpu->trap() ? 1u : 0u;
      tracer.write(rec);

      // Update DUT shadow regfile
      if (rec.rd_addr != 0) dut_regs[rec.rd_addr] = rec.rd_wdata;
      dut_regs[0] = 0;

  // Update DUT shadow CSRs from RVFI (apply mask)
  uint64_t msk, dat;
  msk = cpu->rvfi_csr_mcycle_wmask();
  dat = cpu->rvfi_csr_mcycle_wdata();
  if (msk) dut_mcycle = (dut_mcycle & ~msk) | (dat & msk);
  msk = cpu->rvfi_csr_minstret_wmask();
  dat = cpu->rvfi_csr_minstret_wdata();
  if (msk) dut_minstret = (dut_minstret & ~msk) | (dat & msk);

      if (golden_ready) {
        CommitRec g;
        if (spike.next_commit(g)) {
          // persist golden trace (optional)
          if (trace_enabled && trace_dir && *trace_dir) {
            golden_tracer.write(g);
          }
          // Early direct field checks for quicker diffs
          if (rec.pc_w != g.pc_w) {
            std::ostringstream oss;
            oss << "Golden vs DUT mismatch: pc_mismatch\n";
            oss << "DUT: pc=0x" << std::hex << rec.pc_w << "\n";
            oss << "GOLD: pc=0x" << std::hex << g.pc_w << "\n";
            logger.writeCrashDetailed("golden_divergence_pc", rec.pc_r, rec.insn, cyc, input, oss.str());
            _exit(123);
          }

          // Update Golden shadow regfile
          if (g.rd_addr != 0) gold_regs[g.rd_addr] = g.rd_wdata;
          gold_regs[0] = 0;

          // Golden CSR model: increment counters per commit (assumption for single-issue)
          gold_minstret += 1;
          gold_mcycle   += 1;

          // Full regfile comparison after applying this commit
          int first_diff = -1;
          for (int i = 0; i < 32; ++i) {
            if (dut_regs[i] != gold_regs[i]) { first_diff = i; break; }
          }
          if (first_diff != -1) {
            std::ostringstream oss;
            oss << "Golden vs DUT mismatch: regfile_mismatch at x" << first_diff << "\n";
            oss << std::hex;
            oss << "PC: dut=0x" << rec.pc_w << " gold=0x" << g.pc_w << "\n";
            oss << "RD this step: dut x" << std::dec << (int)rec.rd_addr << "=0x" << std::hex << rec.rd_wdata
                << ", gold x" << std::dec << (int)g.rd_addr << "=0x" << std::hex << g.rd_wdata << "\n";
            // Dump a few differing registers for context
            oss << "Diffs: ";
            int shown = 0;
            for (int i = 0; i < 32 && shown < 8; ++i) {
              if (dut_regs[i] != gold_regs[i]) {
                oss << "x" << std::dec << i << "=dut:0x" << std::hex << dut_regs[i]
                    << ",gold:0x" << std::hex << gold_regs[i] << "; ";
                ++shown;
              }
            }
            oss << "\nRepro: run harness binary with same input file.";
            logger.writeCrashDetailed("golden_divergence_regfile", rec.pc_r, rec.insn, cyc, input, oss.str());
            _exit(123);
          }

          // Memory effect comparison (best-effort): compare presence and address
          bool dut_store = (rec.mem_wmask & 0xF) != 0;
          bool dut_load  = (rec.mem_rmask & 0xF) != 0;
          if ((dut_store != (g.mem_is_store != 0)) || (dut_load != (g.mem_is_load != 0))) {
            std::ostringstream oss;
            oss << "Golden vs DUT mismatch: mem_kind\n";
            oss << "DUT: load=" << (dut_load?"1":"0") << " store=" << (dut_store?"1":"0") << " addr=0x" << std::hex << rec.mem_addr << "\n";
            oss << "GOLD: load=" << (g.mem_is_load?"1":"0") << " store=" << (g.mem_is_store?"1":"0") << " addr=0x" << std::hex << g.mem_addr << "\n";
            logger.writeCrashDetailed("golden_divergence_mem_kind", rec.pc_r, rec.insn, cyc, input, oss.str());
            _exit(123);
          }
          if (dut_store && g.mem_is_store && (rec.mem_addr != g.mem_addr)) {
            std::ostringstream oss;
            oss << "Golden vs DUT mismatch: mem_store_addr\n";
            oss << "DUT: addr=0x" << std::hex << rec.mem_addr << " wmask=0x" << std::hex << rec.mem_wmask << "\n";
            oss << "GOLD: addr=0x" << std::hex << g.mem_addr << " data=0x" << std::hex << g.mem_wdata << "\n";
            logger.writeCrashDetailed("golden_divergence_mem_store_addr", rec.pc_r, rec.insn, cyc, input, oss.str());
            _exit(123);
          }
          if (dut_load && g.mem_is_load && (rec.mem_addr != g.mem_addr)) {
            std::ostringstream oss;
            oss << "Golden vs DUT mismatch: mem_load_addr\n";
            oss << "DUT: addr=0x" << std::hex << rec.mem_addr << " rmask=0x" << std::hex << rec.mem_rmask << "\n";
            oss << "GOLD: addr=0x" << std::hex << g.mem_addr << " data=0x" << std::hex << g.mem_rdata << "\n";
            logger.writeCrashDetailed("golden_divergence_mem_load_addr", rec.pc_r, rec.insn, cyc, input, oss.str());
            _exit(123);
          }

          // CSR comparison: mcycle/minstret shadow vs golden model
          if (dut_minstret != gold_minstret) {
            std::ostringstream oss;
            oss << "Golden vs DUT mismatch: csr_minstret\n";
            oss << "DUT: minstret=" << std::dec << dut_minstret << " GOLD: " << gold_minstret << "\n";
            logger.writeCrashDetailed("golden_divergence_csr_minstret", rec.pc_r, rec.insn, cyc, input, oss.str());
            _exit(123);
          }
          if (dut_mcycle != gold_mcycle) {
            std::ostringstream oss;
            oss << "Golden vs DUT mismatch: csr_mcycle\n";
            oss << "DUT: mcycle=" << std::dec << dut_mcycle << " GOLD: " << gold_mcycle << "\n";
            logger.writeCrashDetailed("golden_divergence_csr_mcycle", rec.pc_r, rec.insn, cyc, input, oss.str());
            _exit(123);
          }
        } else {
          // Spike ended or errored: report and disable golden checks
          golden_ready = false;
          fprintf(stderr,
                  "[HARNESS/GOLDEN] Spike stopped producing commits; disabling golden checks.\n  Command: %s\n  ELF: %s\n",
                  spike.command().c_str(), tmp_elf.empty()?"<unknown>":tmp_elf.c_str());
          if (spike_log_env && *spike_log_env) {
            fprintf(stderr, "  See Spike log: %s\n", spike_log_env);
            print_file_tail(spike_log_env, 60);
          }
        }
      }
    }

    // Perform retire-time checks via dedicated functions
    if (check_x0_write(cpu, logger, cyc, input)) _exit(1);
    if (check_pc_misaligned(cpu, logger, cyc, input)) _exit(1);
    if (check_mem_align_store(cpu, logger, cyc, input)) _exit(1);
    if (check_mem_align_load(cpu, logger, cyc, input)) _exit(1);

    if (cpu->trap()) {
      uint32_t pc   = cpu->rvfi_pc_rdata();
      uint32_t insn = cpu->rvfi_insn();
      logger.writeCrash("trap", pc, insn, cyc, input);
      _exit(124);
    }
  }

  if (cyc >= cfg.max_cycles) {
    uint32_t pc   = cpu->rvfi_pc_rdata();
    uint32_t insn = cpu->rvfi_insn();
    logger.writeCrash("timeout", pc, insn, cyc, input);
    _exit(125);
  }

  _exit(0);
}
