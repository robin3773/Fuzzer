#pragma once
#include "harness_config.hpp"
#include "utils.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>

class CrashLogger {
public:
  explicit CrashLogger(const HarnessConfig &cfg) : cfg_(cfg) {
    utils::ensure_dir(cfg_.crash_dir);
  }

  void writeCrash(const std::string &reason,
                  uint32_t pc,
                  uint32_t insn,
                  unsigned cycle,
                  const std::vector<unsigned char> &input) const {

    const std::string base = makeBaseName(reason, cycle);
    const std::string bin_path = base + ".bin";
    const std::string log_path = base + ".log";

    writeFile(bin_path, input);

    std::string log;
    log.reserve(4096);
    log += "Reason: " + reason + "\n";
    log += "Cycle: " + std::to_string(cycle) + "\n";
    log += "PC: 0x" + hex32(pc) + "\n";
    log += "Instruction: 0x" + hex32(insn) + "\n\n";

    log += "Hexdump:\n";
    log += utils::hexdump(input);
    log += "\n";

    std::string dasm = utils::disassemble(input, cfg_.objdump, cfg_.xlen);
    if (!dasm.empty()) {
      log += "Disassembly:\n";
      log += dasm;
    }

    writeTextAtomically(log_path, log);
  }

private:
  HarnessConfig cfg_;

  static std::string hex32(uint32_t v) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%08x", v);
    return std::string(buf);
  }
  
  //
  std::string makeBaseName(const std::string &reason, unsigned cycle) const {
    std::string ts = utils::timestamp_now();
    return cfg_.crash_dir + "/crash_" + reason + "_" + ts + "_cyc" +
           std::to_string(cycle);
  }

  static void writeFile(const std::string& path, const std::vector<unsigned char>& data) {
    std::string tmp = path + ".tmp";
    int fd = ::open(tmp.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) return;
    utils::safe_write_all(fd, data.data(), data.size());
    ::close(fd);
    ::rename(tmp.c_str(), path.c_str());
  }

  static void writeTextAtomically(const std::string& path, const std::string& text) {
    std::string tmp = path + ".tmp";
    int fd = ::open(tmp.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) return;
    utils::safe_write_all(fd, text.data(), text.size());
    ::close(fd);
    ::rename(tmp.c_str(), path.c_str());
  }
};
