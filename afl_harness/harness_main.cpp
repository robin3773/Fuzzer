#include "harness_config.hpp"
#include "utils.hpp"
#include "cpu_iface.hpp"
#include "crash_logger.hpp"

#include "verilated.h"
#include <csignal>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

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

  unsigned cyc = 0;
  for (; cyc < cfg.max_cycles && !cpu->got_finish(); ++cyc) {
    if (g_sig) {
      uint32_t pc   = cpu->rvfi_pc_rdata();
      uint32_t insn = cpu->rvfi_insn();
      logger.writeCrash(std::string("signal_") + std::to_string(g_sig),
                        pc, insn, cyc, input);
      _exit(1);
    }

    cpu->step();

    if (cpu->trap()) {
      uint32_t pc   = cpu->rvfi_pc_rdata();
      uint32_t insn = cpu->rvfi_insn();
      logger.writeCrash("trap", pc, insn, cyc, input);
      _exit(1);
    }
  }

  if (cyc >= cfg.max_cycles) {
    uint32_t pc   = cpu->rvfi_pc_rdata();
    uint32_t insn = cpu->rvfi_insn();
    logger.writeCrash("timeout", pc, insn, cyc, input);
    _exit(1);
  }

  _exit(0);
}
