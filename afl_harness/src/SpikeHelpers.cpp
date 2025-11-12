#include "SpikeHelpers.hpp"
#include <hwfuzz/Debug.hpp>
#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <boost/process.hpp>

namespace bp = boost::process;

namespace spike_helpers {

void print_log_tail(const char* path, int max_lines) {
  if (!path || !*path) return;
  int fd = ::open(path, O_RDONLY);
  if (fd < 0) return;
  
  struct stat st{};
  if (fstat(fd, &st) != 0) { 
    ::close(fd); 
    return; 
  }
  
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
    
    // Scan backwards for newlines
    for (ssize_t i = (ssize_t)toread - 1; i >= 0; --i) {
      if (tmp[(size_t)i] == '\n') {
        lines++;
        if (lines > max_lines) {
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
    FILE* log = hwfuzz::debug::getDebugLog();
    if (log) {
      std::fprintf(log, "----- spike.log (tail) -----\n");
      std::fwrite(buf.data(), 1, buf.size(), log);
      if (buf.back() != '\n') std::fputc('\n', log);
      std::fprintf(log, "----- end spike.log (tail) -----\n");
      std::fflush(log);
    }
  }
  ::close(fd);
}

std::string format_arg(const std::string& arg) {
  if (arg.find_first_of(" \t\"'") == std::string::npos) {
    return arg;
  }
  std::string quoted;
  quoted.reserve(arg.size() + 2);
  quoted.push_back('"');
  for (char c : arg) {
    if (c == '"' || c == '\\') quoted.push_back('\\');
    quoted.push_back(c);
  }
  quoted.push_back('"');
  return quoted;
}

std::string build_spike_elf(const std::vector<unsigned char>& input, const std::string& ld_bin, const std::string& linker_script) {
  // Create temporary binary file
  char tmpbin[] = "/tmp/dut_in_XXXXXX.bin";
  int bfd = ::mkstemps(tmpbin, 4);
  if (bfd < 0) {
    hwfuzz::debug::logError("[SPIKE] Failed to create temporary input file.\n");
    return "";
  }
  
  ::write(bfd, input.data(), input.size());
  ::close(bfd);
  
  std::string tmpbin_path(tmpbin);
  std::string tmpobj = tmpbin_path + ".o";
  std::string elfpath = tmpbin_path + ".elf";
  
  auto cleanup_tmp = [&]() {
    ::unlink(tmpobj.c_str());
    ::unlink(tmpbin_path.c_str());
  };
  
  // Get objcopy binary (default to 32-bit RISC-V with full path)
  std::string objcopy = "/opt/riscv/bin/riscv32-unknown-elf-objcopy";
  
  // Convert binary to object file
  std::vector<std::string> objcopy_args{
    "-I", "binary",
    "-O", "elf32-littleriscv",
    "-B", "riscv:rv32",
    tmpbin_path,
    tmpobj
  };
  
  std::ostringstream cmd_for_log;
  cmd_for_log << format_arg(objcopy);
  for (const auto& arg : objcopy_args) {
    cmd_for_log << ' ' << format_arg(arg);
  }
  
  std::error_code spawn_ec;
  int rc = bp::system(objcopy, bp::args(objcopy_args), bp::std_err > bp::null, spawn_ec);
  
  if (spawn_ec || rc != 0) {
    hwfuzz::debug::logError("[SPIKE] objcopy failed.\n  Command: %s\n", cmd_for_log.str().c_str());
    if (spawn_ec) {
      hwfuzz::debug::logError("[SPIKE] Spawn error: %s\n", spawn_ec.message().c_str());
    } else {
      hwfuzz::debug::logError("[SPIKE] Exit code: %d\n", rc);
    }
    cleanup_tmp();
    return "";
  }
  
  // Link object file to create ELF
  if (linker_script.empty()) {
    hwfuzz::debug::logError("[SPIKE] LINKER_SCRIPT not set; cannot build ELF.\n");
    cleanup_tmp();
    return "";
  }
  
  std::vector<std::string> ld_args{ "-T", linker_script, tmpobj, "-o", elfpath };
  
  // Add -defsym arguments from environment
  auto append_defsym = [&ld_args](const char* name) {
    if (!name) return;
    if (const char* val = std::getenv(name)) {
      if (*val) {
        ld_args.emplace_back("-defsym");
        ld_args.emplace_back(std::string(name) + "=" + val);
      }
    }
  };
  
  append_defsym("PROGADDR_RESET");
  append_defsym("PROGADDR_IRQ");
  append_defsym("RAM_BASE");
  append_defsym("RAM_SIZE");
  append_defsym("STACK_ADDR");
  append_defsym("STACKADDR");
  
  std::ostringstream ld_cmd_builder;
  ld_cmd_builder << format_arg(ld_bin);
  for (const auto& arg : ld_args) {
    ld_cmd_builder << ' ' << format_arg(arg);
  }
  
  std::error_code ld_ec;
  int ld_rc = bp::system(ld_bin, bp::args(ld_args), bp::std_err > bp::null, ld_ec);
  
  if (ld_ec || ld_rc != 0) {
    hwfuzz::debug::logError("[SPIKE] ld failed.\n  Command: %s\n", ld_cmd_builder.str().c_str());
    if (ld_ec) {
      hwfuzz::debug::logError("[SPIKE] Spawn error: %s\n", ld_ec.message().c_str());
    } else {
      hwfuzz::debug::logError("[SPIKE] Exit code: %d\n", ld_rc);
    }
    cleanup_tmp();
    return "";
  }
  
  cleanup_tmp();
  return elfpath;
}

} // namespace spike_helpers
