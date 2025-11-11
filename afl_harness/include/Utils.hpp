/**
 * @file Utils.hpp
 * @brief General utility functions for the AFL++ fuzzing harness
 * 
 * This file provides a collection of utility functions for common operations
 * including file I/O, directory creation, timestamp generation, hexdump
 * formatting, and disassembly of binary code. All functions are inline for
 * header-only convenience.
 */

#pragma once
#include <hwfuzz/Debug.hpp>
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

/**
 * @namespace utils
 * @brief Utility functions for the fuzzing harness
 * 
 * Contains helper functions for file operations, formatting, and diagnostic
 * output. All functions are inline and header-only for easy integration.
 */
namespace utils {

/**
 * @brief Create directory and all parent directories if they don't exist
 * 
 * Recursively creates the specified directory path, similar to `mkdir -p`.
 * If the directory already exists, this is a no-op. Logs creation to the
 * harness log for debugging.
 * 
 * @param path Directory path to create (can be relative or absolute)
 * 
 * @note Silently succeeds if the directory already exists
 * @note Logs errors to harness log but does not throw exceptions
 * 
 * Example usage:
 * @code
 *   ensure_dir("/tmp/fuzzer/crashes");
 *   // Creates /tmp, /tmp/fuzzer, and /tmp/fuzzer/crashes if needed
 *   // Log output: "[INFO] Created directory: /tmp/fuzzer/crashes"
 * @endcode
 * 
 * @code
 *   ensure_dir("workdir/logs/traces");  // Relative path
 *   ensure_dir("");                      // No-op for empty path
 * @endcode
 */
inline void ensure_dir(const std::string &path) {
  if (path.empty())
    return;

  try {
    std::filesystem::create_directories(path);
    hwfuzz::debug::logInfo("Created directory: %s\n", path.c_str());
  } catch (const std::filesystem::filesystem_error &e) {
    hwfuzz::debug::logError("Failed to create directory %s: %s\n", path.c_str(), e.what());
  }
}

/**
 * @brief Write all data to a file descriptor, handling partial writes
 * 
 * Performs a complete write operation by repeatedly calling write() until
 * all bytes are written, handling EINTR and partial writes. Does not close
 * the file descriptor.
 * 
 * @param fd Open file descriptor to write to
 * @param buf Pointer to data buffer to write
 * @param len Number of bytes to write
 * 
 * @note Does not throw exceptions; silently stops on non-EINTR errors
 * @note Caller is responsible for opening and closing the file descriptor
 * 
 * Example usage:
 * @code
 *   int fd = open("output.bin", O_WRONLY | O_CREAT, 0644);
 *   std::vector<uint8_t> data = {0x13, 0x00, 0x00, 0x00};
 *   safe_write_all(fd, data.data(), data.size());
 *   close(fd);
 * @endcode
 */
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

/**
 * @brief Atomically write all data to a file, with error handling
 * 
 * Opens the specified file, writes all data in a single operation, and
 * closes the file. Throws std::runtime_error on any failure. Uses binary
 * mode to preserve exact byte values.
 * 
 * @param filepath Path to file to write (created/truncated if exists)
 * @param buf Pointer to data buffer to write
 * @param len Number of bytes to write
 * 
 * @throws std::runtime_error if file cannot be opened or write fails
 * 
 * Example usage:
 * @code
 *   std::vector<uint8_t> crash_input = {0x00, 0x00, 0x00, 0x73};  // ecall
 *   try {
 *     safe_write_all("crash_001.bin", crash_input.data(), crash_input.size());
 *   } catch (const std::runtime_error& e) {
 *     std::cerr << "Write failed: " << e.what() << std::endl;
 *   }
 * @endcode
 */
inline void safe_write_all(const std::string &filepath, const void *buf, size_t len) {
  std::ofstream out(filepath, std::ios::binary | std::ios::out);
  if (!out)
    throw std::runtime_error("Failed to open file: " + filepath);

  out.write(static_cast<const char *>(buf), static_cast<std::streamsize>(len));
  if (!out)
    throw std::runtime_error("Failed to write all data to file: " + filepath);
}

/**
 * @brief Generate ISO 8601 timestamp string for current time
 * 
 * Creates a timestamp string in the format "YYYYMMDDTHHmmss" using local
 * time. Useful for generating unique filenames for crash artifacts and logs.
 * 
 * @return Timestamp string (e.g., "20250111T143052")
 * 
 * @note Uses localtime_r() for thread-safety
 * @note Falls back to Unix epoch time if localtime fails
 * 
 * Example output:
 * - January 11, 2025 at 2:30:52 PM → "20250111T143052"
 * - Fallback on error → "1736611852" (seconds since epoch)
 * 
 * Example usage:
 * @code
 *   std::string crash_file = "crash_" + timestamp_now() + ".bin";
 *   // Creates: "crash_20250111T143052.bin"
 * @endcode
 * 
 * @code
 *   std::string log_dir = "logs/" + timestamp_now();
 *   ensure_dir(log_dir);
 *   // Creates: "logs/20250111T143052/"
 * @endcode
 */
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

/**
 * @brief Generate hexdump of binary data in traditional format
 * 
 * Creates a multi-line hexdump string similar to the output of the `hexdump`
 * or `xxd` utilities. Each line shows offset, hex bytes (space-separated),
 * and ASCII representation with printable characters shown and others as dots.
 * 
 * Format per line:
 * - 8 hex digits: byte offset
 * - 16 hex bytes: data in hex (with extra space after 8th byte)
 * - ASCII view: printable chars or '.' for non-printable
 * 
 * @param data Vector of bytes to dump
 * @param bytes_per_line Number of bytes to show per line (default: 16)
 * 
 * @return Multi-line string containing the formatted hexdump
 * 
 * Example output:
 * @code
 *   std::vector<unsigned char> data = {
 *     0x7f, 0x45, 0x4c, 0x46, 0x01, 0x01, 0x01, 0x00,
 *     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 *   };
 *   std::cout << hexdump(data);
 *   
 *   // Output:
 *   // 00000000  7f 45 4c 46 01 01 01 00  00 00 00 00 00 00 00 00 |.ELF............|
 * @endcode
 * 
 * @code
 *   // For RISC-V instructions (4 bytes per line):
 *   std::cout << hexdump(insn_stream, 4);
 *   // 00000000  13 00 00 00 |....|
 *   // 00000004  93 80 00 00 |....|
 * @endcode
 */
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

/**
 * @brief Disassemble binary code using objdump
 * 
 * Invokes the RISC-V objdump utility to disassemble a sequence of machine
 * code bytes. Creates a temporary binary file, runs objdump with appropriate
 * architecture flags, captures the output, and cleans up temporary files.
 * 
 * Disassembly options:
 * - Includes compressed (RVC) instructions
 * - Uses numeric register names (x0-x31 instead of zero, ra, sp, etc.)
 * - Shows both opcodes and mnemonics
 * 
 * @param bytes Vector of machine code bytes to disassemble
 * @param objdump Path to objdump binary (e.g., riscv32-unknown-elf-objdump)
 * @param xlen ISA width: 32 for RV32, 64 for RV64
 * 
 * @return Disassembly text output, or empty string on error
 * 
 * @note Creates temporary files in /tmp (cleaned up automatically)
 * @note Requires objdump binary to be installed and in specified path
 * @note Stderr is redirected to /dev/null to suppress warnings
 * 
 * Example output:
 * @code
 *   std::vector<unsigned char> code = {0x13, 0x00, 0x00, 0x00};
 *   std::string asm_text = disassemble(code, "/usr/bin/riscv32-objdump", 32);
 *   std::cout << asm_text;
 *   
 *   // Typical output:
 *   // /tmp/afl_dasm_Xb7K2m.bin:     file format binary
 *   //
 *   // Disassembly of section .data:
 *   //
 *   // 0000000000000000 <.data>:
 *   //    0:   00000013                nop
 * @endcode
 * 
 * Example usage in crash logging:
 * @code
 *   std::string dasm = disassemble(crash_input, cfg.objdump, cfg.xlen);
 *   if (!dasm.empty()) {
 *     crash_log << "Disassembly:\n" << dasm << "\n";
 *   } else {
 *     crash_log << "Disassembly not available\n";
 *   }
 * @endcode
 */
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
