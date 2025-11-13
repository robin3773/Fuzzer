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
#include "Feedback.hpp"
#include "VerilatorCoverage.hpp"

#include "verilated.h"
#include <hwfuzz/Debug.hpp>
#include <algorithm>
#include <csignal>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
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
  if (cfg.trace_enabled) {
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
  bool bootloader_skipped;
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
                               Feedback& feedback, fuzz::VerilatorCoverage& coverage,
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
      rec.mem_wdata = cpu->rvfi_mem_wdata();
      rec.mem_rdata = cpu->rvfi_mem_rdata();
      rec.trap = cpu->trap() ? 1u : 0u;
      
      tracer.write(rec);

      // Report coverage feedback to AFL++
      feedback.report_instruction(rec.pc_r, rec.insn);
      if (rec.mem_rmask || rec.mem_wmask) {
        feedback.report_memory_access(rec.mem_addr, rec.mem_wmask != 0);
      }
      if (rec.rd_addr != 0) {
        feedback.report_register_write(rec.rd_addr, rec.rd_wdata);
      }

      // Check PC stagnation (hang detection)
      if (crash_detection::check_pc_stagnation(cpu, logger, state.cyc, input,
                                               cfg.pc_stagnation_limit, state.last_progress_pc,
                                               state.last_progress_valid, state.stagnation_count)) {
        // PC stagnation is a hang - abort so AFL++ can detect it
        golden.stop();
        hwfuzz::debug::logWarn("[HANG] PC stagnation detected - aborting\n");
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
        // Skip Spike bootloader commits (PC < 0x80000000) before first comparison
        if (!state.bootloader_skipped) {
          CommitRec skip_rec;
          while (golden.next_commit(skip_rec)) {
            if (skip_rec.pc_w >= 0x80000000) {
              // Reached DUT entry point - use this commit for comparison
              diff_checker.update_golden_state(skip_rec);
              diff_checker.check_divergence(rec, skip_rec, logger, state.cyc, input);
              state.bootloader_skipped = true;
              break;
            }
            // Discard bootloader commit, continue skipping
          }
          if (!state.bootloader_skipped) {
            // Spike stopped before reaching entry point
            state.exit_reason = ExitReason::SpikeDone;
            state.graceful_exit = true;
            break;
          }
        } else {
          // Normal operation after bootloader skipped
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

  // One-time initialization (outside AFL++ loop)
  CrashLogger logger(cfg);
  // Note: Trace file is NOT initialized here - it's per-test-case now
  
  // Setup feedback for AFL++ (shared across iterations)
  Feedback feedback;
  feedback.initialize();
  
  // Setup Verilator coverage tracking (shared across iterations)
  fuzz::VerilatorCoverage coverage;
  std::string coverage_file = cfg.trace_dir + "/coverage.dat";
  coverage.initialize(coverage_file);

  auto execute_test_case = [&](const std::vector<unsigned char>& input) {
    // Initialize per-test-case trace writer
    // This ensures each test case gets a fresh trace file instead of accumulating
    TraceWriter tracer;
    if (cfg.trace_enabled) {
      tracer.open(cfg.trace_dir);
    }
    
    // Initialize DUT (fresh instance per iteration)
    CpuIface* cpu = make_cpu();
    cpu->reset();
    cpu->load_input(input);

    // Setup golden model (Spike) for this iteration
    GoldenModel golden;
    golden.initialize(input, cfg);

    // Setup differential checker for this iteration
    DifferentialChecker diff_checker;

    // Run execution
    ExecutionState state = {};
    state.exit_reason = ExitReason::None;
    state.graceful_exit = false;
    state.stagnation_count = 0;
    state.last_progress_valid = false;

    run_execution_loop(cpu, cfg, input, logger, tracer, golden, diff_checker, feedback, coverage, state);

    // Write coverage data and report to AFL++
    coverage.write_and_reset();
    coverage.report_to_afl(feedback);

    // Cleanup golden model
    golden.stop();

    // Handle termination
    if (state.graceful_exit) {
      hwfuzz::debug::logInfo("[HARNESS] Graceful termination after %u cycles (reason=%s).\n",
                             state.cyc, exit_reason_text(state.exit_reason));
      delete cpu;
      return;
    }

    // Check for timeout (hang detection)
    if (crash_detection::check_timeout(state.cyc, cfg.max_cycles, cpu, logger, input)) {
      // Timeout is a hang - abort so AFL++ can detect it
      hwfuzz::debug::logWarn("[HANG] Timeout detected - aborting\n");
      delete cpu;
      std::abort();
    }

    delete cpu;
  };

  std::string golden_mode_lower = cfg.golden_mode;
  std::transform(golden_mode_lower.begin(), golden_mode_lower.end(), golden_mode_lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  bool use_persistent = (golden_mode_lower != "live");

  if (use_persistent) {
    // AFL++ persistent mode - fork server starts here
    // This dramatically improves fuzzing speed by avoiding process startup overhead
    __AFL_INIT();

    hwfuzz::debug::logInfo("[HARNESS] Starting AFL++ persistent mode loop\n");

    // Per-iteration fuzzing loop (AFL++ will restart from here)
    while (__AFL_LOOP(10000)) {
      hwfuzz::debug::logInfo("[HARNESS] Loop iteration started\n");

      // Load input for this iteration (must reopen file each time for AFL++ @@)
      std::vector<unsigned char> input = load_input(argc, argv);

      hwfuzz::debug::logInfo("[HARNESS] Loaded %zu bytes of input\n", input.size());

      // Skip empty inputs
      if (input.empty()) {
        hwfuzz::debug::logWarn("[HARNESS] Empty input, skipping iteration\n");
        continue;
      }

      execute_test_case(input);
    }

    // Exit after AFL++ persistent loop completes
    _exit(0);
  }

  // GOLDEN_MODE=live → no persistent loop, just single-run model
  __AFL_INIT();
  hwfuzz::debug::logInfo("[HARNESS] Non-persistent execution (GOLDEN_MODE=live)\n");
  hwfuzz::debug::logInfo("[HARNESS] Loop iteration started\n");
  std::vector<unsigned char> input = load_input(argc, argv);
  hwfuzz::debug::logInfo("[HARNESS] Loaded %zu bytes of input\n", input.size());
  if (input.empty()) {
    hwfuzz::debug::logWarn("[HARNESS] Empty input, exiting\n");
    return 0;
  }
  execute_test_case(input);
  return 0;
}
