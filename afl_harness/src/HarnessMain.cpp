#include "HarnessConfig.hpp"
#include "Utils.hpp"
#include "CpuIface.hpp"
#include "CrashLogger.hpp"
#include "CrashDetection.hpp"
#include "Trace.hpp"
#include "GoldenModel.hpp"
#include "DifferentialChecker.hpp"
#include "DutExit.hpp"
#include "SpikeHelpers.hpp"

#include "verilated.h"
#include <hwfuzz/Debug.hpp>
#include <csignal>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

// ============================================================================
// Signal Handling
// ============================================================================

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

// ============================================================================
// Input Loading
// ============================================================================

static void read_all_fd(int fd, std::vector<unsigned char>& out, size_t cap = 1 << 20) {
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
  if (out.empty()) {
    unsigned char b = 0x13;  // ADDI x0,x0,0
    out.push_back(b);
  }
}

static std::vector<unsigned char> load_input(int argc, char** argv) {
  std::vector<unsigned char> input;
  if (argc > 1 && argv[1][0] != '-') {
    int fd = ::open(argv[1], O_RDONLY);
    if (fd >= 0) {
      read_all_fd(fd, input);
      ::close(fd);
    } else {
      read_all_fd(STDIN_FILENO, input);
    }
  } else {
    read_all_fd(STDIN_FILENO, input);
  }
  return input;
}

// ============================================================================
// Trace Setup
// ============================================================================

static TraceWriter setup_trace(const HarnessConfig& cfg) {
  TraceWriter tracer;
  const char* trace_mode_env = std::getenv("TRACE_MODE");
  
  bool trace_enabled = true;
  if (trace_mode_env && (std::string(trace_mode_env) == "off" || std::string(trace_mode_env) == "0")) {
    trace_enabled = false;
  }
  
  if (trace_enabled) {
    tracer.open(cfg.trace_dir);
  }
  
  return tracer;
}

// ============================================================================
// DUT Execution Loop
// ============================================================================

struct ExecutionState {
  unsigned cyc;
  ExitReason exit_reason;
  bool graceful_exit;
  uint32_t last_progress_pc;
  bool last_progress_valid;
  unsigned stagnation_count;
};

static void handle_signal_crash(CrashLogger& logger, CpuIface* cpu, unsigned cyc,
                                const std::vector<unsigned char>& input) {
  if (g_sig) {
    uint32_t pc = cpu->rvfi_pc_rdata();
    uint32_t insn = cpu->rvfi_insn();
    logger.writeCrash(std::string("signal_") + std::to_string(g_sig), pc, insn, cyc, input);
    _exit(126);
  }
}

static bool check_exit_conditions(CpuIface* cpu, const CommitRec& rec, const HarnessConfig& cfg,
                                   ExecutionState& state) {
  // Check for PC stagnation (infinite loop detection)
  if (cpu->rvfi_valid()) {
    if (state.last_progress_valid && rec.pc_r == state.last_progress_pc) {
      state.stagnation_count++;
    } else {
      state.last_progress_pc = rec.pc_r;
      state.last_progress_valid = true;
      state.stagnation_count = 0;
    }
  }

  // Check tohost exit
  if (cfg.use_tohost && (rec.mem_wmask & 0xF) != 0) {
    if ((rec.mem_addr & ~0x3u) == (cfg.tohost_addr & ~0x3u)) {
      state.exit_reason = ExitReason::Tohost;
      state.graceful_exit = true;
      return true;
    }
  }

  // Check trap-based exit (ECALL)
  if (rec.trap) {
    state.exit_reason = ExitReason::Ecall;
    state.graceful_exit = true;
    return true;
  }

  return false;
}

static void run_execution_loop(CpuIface* cpu, const HarnessConfig& cfg,
                               const std::vector<unsigned char>& input,
                               CrashLogger& logger, TraceWriter& tracer,
                               GoldenModel& golden, DifferentialChecker& diff_checker,
                               ExecutionState& state) {
  for (; state.cyc < cfg.max_cycles && !cpu->got_finish(); ++state.cyc) {
    handle_signal_crash(logger, cpu, state.cyc, input);

    cpu->step();

    if (cpu->got_finish()) {
      state.exit_reason = ExitReason::Finish;
      state.graceful_exit = true;
      break;
    }

    // Process committed instruction
    if (cpu->rvfi_valid()) {
      CommitRec rec;
      rec.pc_r = cpu->rvfi_pc_rdata();
      rec.pc_w = cpu->rvfi_pc_wdata();
      rec.insn = cpu->rvfi_insn();
      rec.rd_addr = cpu->rvfi_rd_addr();
      rec.rd_wdata = cpu->rvfi_rd_wdata();
      rec.mem_addr = cpu->rvfi_mem_addr();
      rec.mem_rmask = cpu->rvfi_mem_rmask();
      rec.mem_wmask = cpu->rvfi_mem_wmask();
      rec.trap = cpu->trap() ? 1u : 0u;
      
      tracer.write(rec);

      // Check PC stagnation
      if (crash_detection::check_pc_stagnation(cpu, logger, state.cyc, input,
                                               cfg.pc_stagnation_limit, state.last_progress_pc,
                                               state.last_progress_valid, state.stagnation_count)) {
        std::abort();
      }

      // Check exit conditions
      if (check_exit_conditions(cpu, rec, cfg, state)) {
        break;
      }

      // Update DUT state
      diff_checker.update_dut_state(rec);
      diff_checker.update_dut_csrs(cpu);

      // Golden model differential checking
      if (golden.is_ready()) {
        CommitRec gold_rec;
        if (golden.next_commit(gold_rec)) {
          diff_checker.update_golden_state(gold_rec);
          diff_checker.check_divergence(rec, gold_rec, logger, state.cyc, input);
        } else if (cfg.stop_on_spike_done && golden.spike().has_status() &&
                   golden.spike().exited() && golden.spike().exit_code() == 0) {
          state.exit_reason = ExitReason::SpikeDone;
          state.graceful_exit = true;
          break;
        }
      }
    }

    // Perform retire-time crash checks
    if (crash_detection::check_x0_write(cpu, logger, state.cyc, input)) std::abort();
    if (crash_detection::check_pc_misaligned(cpu, logger, state.cyc, input)) std::abort();
    if (crash_detection::check_mem_align_store(cpu, logger, state.cyc, input)) std::abort();
    if (crash_detection::check_mem_align_load(cpu, logger, state.cyc, input)) std::abort();
    if (crash_detection::check_trap(cpu, logger, state.cyc, input)) std::abort();
  }
}

// ============================================================================
// Main Entry Point
// ============================================================================

extern "C" CpuIface* make_cpu();

int main(int argc, char** argv) {
  // Setup
  install_signal_handlers();
  
  HarnessConfig cfg;
  cfg.loadconfig();
  utils::ensure_dir(cfg.crash_dir);

  Verilated::commandArgs(argc, argv);
#if VL_VER_MAJOR >= 5
  Verilated::randReset(0);
#else
  Verilated::randSeed(0);
#endif

  // Load input
  std::vector<unsigned char> input = load_input(argc, argv);

  // Initialize DUT
  CpuIface* cpu = make_cpu();
  cpu->reset();
  cpu->load_input(input);

  // Setup logging and tracing
  CrashLogger logger(cfg);
  TraceWriter tracer = setup_trace(cfg);

  // Setup golden model (Spike)
  GoldenModel golden;
  golden.initialize(input, cfg.trace_dir.c_str());

  // Setup differential checker
  DifferentialChecker diff_checker;

  // Run execution
  ExecutionState state = {};
  state.exit_reason = ExitReason::None;
  state.graceful_exit = false;
  state.stagnation_count = 0;
  state.last_progress_valid = false;

  run_execution_loop(cpu, cfg, input, logger, tracer, golden, diff_checker, state);

  // Handle termination
  if (state.graceful_exit) {
    golden.stop();
    hwfuzz::debug::logInfo("[HARNESS] Graceful termination after %u cycles (reason=%s).\n",
                           state.cyc, exit_reason_text(state.exit_reason));
    _exit(0);
  }

  // Check for timeout
  if (crash_detection::check_timeout(state.cyc, cfg.max_cycles, cpu, logger, input)) {
    std::abort();
  }

  _exit(0);
}
