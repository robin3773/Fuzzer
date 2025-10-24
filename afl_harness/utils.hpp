#pragma once
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace utils {

inline void ensure_dir(const std::string& path) {
  if (path.empty()) return;

  // Normalize: remove trailing slashes (but keep single leading slash)
  std::string p = path;
  while (p.size() > 1 && p.back() == '/') p.pop_back();

  // Handle absolute paths: start building from '/'
  size_t pos = 0;
  std::string cur;
  if (p[0] == '/') {
    cur = "/";
    pos = 1;
  }

  // Create each path component progressively (mkdir -p behavior)
  while (pos <= p.size()) {
    size_t slash = p.find('/', pos);
    std::string token;
    if (slash == std::string::npos) {
      token = p.substr(pos);
      pos = p.size() + 1;
    } else {
      token = p.substr(pos, slash - pos);
      pos = slash + 1;
    }
    if (token.empty()) continue;
    if (!cur.empty() && cur.back() != '/') cur += '/';
    cur += token;

    if (::mkdir(cur.c_str(), 0700) != 0) {
      if (errno == EEXIST) {
        // already exists, fine
      } else {
        fprintf(stderr, "[HARNESS/UTILS] Failed to create directory %s: %s\n",
                cur.c_str(), strerror(errno));
        return;
      }
    } else {
      printf("[HARNESS/UTILS] Created directory: %s\n", cur.c_str());
    }
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
