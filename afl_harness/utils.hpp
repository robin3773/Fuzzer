#pragma once
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace utils {

inline void ensure_dir(const std::string &path) {
  if (path.empty())
    return;

  try {
    std::filesystem::create_directories(path);
    std::cout << "[INFO] Created directory: " << path << "\n";
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "[ERROR] Failed to create directory " << path
              << ": " << e.what() << "\n";
  }
}

inline void safe_write_all(int fd, const void* buf, size_t len) {
  const unsigned char* p = (const unsigned char*)buf;
  size_t off = 0;
  while (off < len) {
    ssize_t n = ::write(fd, p + off, len - off);
    if (n < 0) {
      if (errno == EINTR) continue;
      break;
    }
    off += (size_t)n;
  }
}

inline void safe_write_all(const std::string &filepath, const void *buf, size_t len) {
  std::ofstream out(filepath, std::ios::binary | std::ios::out);
  if (!out)
    throw std::runtime_error("Failed to open file: " + filepath);

  out.write(static_cast<const char *>(buf), static_cast<std::streamsize>(len));
  if (!out)
    throw std::runtime_error("Failed to write all data to file: " + filepath);
}

inline std::string timestamp_now() {
  char buf[64];
  std::time_t t = std::time(nullptr);
  std::tm tm;
  if (localtime_r(&t, &tm)) {
    std::strftime(buf, sizeof(buf), "%Y%m%dT%H%M%S", &tm);
  } else {
    std::snprintf(buf, sizeof(buf), "%ld", (long)t);
  }
  return std::string(buf);
}

inline std::string hexdump(const std::vector<unsigned char>& data,
                           size_t bytes_per_line = 16) {
  std::ostringstream oss;
  const size_t n = data.size();
  for (size_t off = 0; off < n; off += bytes_per_line) {
    size_t row = (off + bytes_per_line <= n) ? bytes_per_line : (n - off);
    // offset
    oss << std::setw(8) << std::setfill('0') << std::hex << off << "  ";
    // hex
    for (size_t i = 0; i < bytes_per_line; ++i) {
      if (i < row) {
        oss << std::setw(2) << std::setfill('0') << std::hex
            << (unsigned)data[off + i] << " ";
      } else {
        oss << "   ";
      }
      if (i == 7) oss << " ";
    }
    oss << " |";
    // ascii
    for (size_t i = 0; i < row; ++i) {
      unsigned char c = data[off + i];
      oss << (std::isprint(c) ? (char)c : '.');
    }
    oss << "|\n";
  }
  return oss.str();
}

// Disassemble with objdump (best effort)
inline std::string disassemble(const std::vector<unsigned char>& bytes,
                               const std::string& objdump, int xlen) {
  // write temp bin
  char tmpl[] = "/tmp/afl_dasm_XXXXXX";
  int fd = ::mkstemp(tmpl);
  if (fd < 0) return {};
  ::close(fd);
  std::string bin = std::string(tmpl) + ".bin";
  FILE* f = std::fopen(bin.c_str(), "wb");
  if (!f) { ::unlink(tmpl); return {}; }
  size_t w = std::fwrite(bytes.data(), 1, bytes.size(), f);
  std::fclose(f);
  if (w != bytes.size()) { ::unlink(bin.c_str()); ::unlink(tmpl); return {}; }

  // run objdump
  std::string cmd = "'" + objdump + "' -b binary -m " +
                    std::string(xlen == 64 ? "riscv:rv64" : "riscv:rv32") +
                    " -M rvc,numeric -D -w '" + bin + "' 2>/dev/null";
  std::string out;
  FILE* pipe = ::popen(cmd.c_str(), "r");
  if (pipe) {
    char* line = nullptr; size_t cap = 0;
    while (getline(&line, &cap, pipe) != -1) out.append(line);
    if (line) free(line);
    ::pclose(pipe);
  }
  ::unlink(bin.c_str());
  ::unlink(tmpl);
  return out;
}

} // namespace utils
