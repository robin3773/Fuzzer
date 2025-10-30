// ==========================================================
// mutator_selftest.cpp — RV32/RV64 Mutator Standalone Tester
// (classic AFL custom mutator API: afl_custom_mutator / _havoc_mutation)
// ==========================================================

#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glob.h>
#include <sys/types.h>

// ---------- AFL custom mutator (classic) signatures ----------
using afl_init_fn   = int    (*)(void *afl);
using afl_deinit_fn = void   (*)();
using afl_mut_fn    = size_t (*)(void *afl,
                                 unsigned char *buf, size_t buf_size,
                                 unsigned char **out_buf, size_t max_size);

// ---------- ANSI colors ----------
static bool g_color = true;
static inline const char* C(const char* s) { return g_color ? s : ""; }
static constexpr const char* C_RESET = "\033[0m";
static constexpr const char* C_GREEN = "\033[1;32m";
static constexpr const char* C_CYAN  = "\033[1;36m";
static constexpr const char* C_YEL   = "\033[1;33m";
static constexpr const char* C_MIDB  = "\033[1;34m";
static constexpr const char* C_RED   = "\033[1;31m";

// ---------- Helpers ----------
static inline bool is_space(unsigned char c) { return std::isspace(c) != 0; }

struct DisasmCols {
  std::string prefix;   // includes offset + ':' and original indentation
  std::string bytes;    // encoded word / .insn token
  std::string mnemonic; // assembly mnemonic
  std::string operands; // remaining operand string (may include comments)
  std::string raw_rest; // fallback when mnemonic parsing fails
};

static DisasmCols split_disasm_line(const std::string& line) {
  DisasmCols cols;
  if (line.empty()) {
    cols.prefix = line;
    return cols;
  }

  size_t colon = line.find(':');
  if (colon == std::string::npos) {
    cols.prefix = line;
    return cols;
  }

  size_t prefix_end = colon + 1;
  cols.prefix = line.substr(0, prefix_end);

  size_t pos = prefix_end;
  while (pos < line.size() && is_space(static_cast<unsigned char>(line[pos])))
    ++pos;

  size_t bytes_begin = pos;
  while (pos < line.size() && !is_space(static_cast<unsigned char>(line[pos])))
    ++pos;
  cols.bytes = line.substr(bytes_begin, pos - bytes_begin);

  while (pos < line.size() && is_space(static_cast<unsigned char>(line[pos])))
    ++pos;

  if (pos < line.size()) {
    cols.raw_rest = line.substr(pos);

    size_t m_begin = pos;
    while (pos < line.size() && !is_space(static_cast<unsigned char>(line[pos])))
      ++pos;
    cols.mnemonic = line.substr(m_begin, pos - m_begin);

    while (pos < line.size() && is_space(static_cast<unsigned char>(line[pos])))
      ++pos;

    if (pos < line.size())
      cols.operands = line.substr(pos);
  }
  return cols;
}

static std::string format_disasm_line(const std::string& line) {
  auto cols = split_disasm_line(line);
  // If no colon or nothing beyond prefix, return original line unchanged.
  if (cols.prefix.empty() || cols.prefix == line)
    return line;

  std::ostringstream oss;
  oss << cols.prefix;
  if (!cols.bytes.empty()) {
    oss << ' ' << std::left << std::setw(12) << cols.bytes;
  } else {
    oss << ' ' << std::setw(12) << ' ';
  }
  if (!cols.mnemonic.empty()) {
    oss << ' ' << std::left << std::setw(8) << cols.mnemonic;
    if (!cols.operands.empty())
      oss << ' ' << cols.operands;
  } else if (!cols.raw_rest.empty()) {
    oss << ' ' << cols.raw_rest;
  }

  return oss.str();
}

static bool file_exists(const std::string& p) {
  struct stat st; return ::stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}
static std::string env_or(const char* key, const char* defv) {
  const char* v = std::getenv(key);
  return (v && *v) ? std::string(v) : std::string(defv);
}
// Read the selected mutator strategy (default HYBRID) and normalize it
static std::string get_strategy() {
  std::string s = env_or("RV32_STRATEGY", "HYBRID");
  std::string u = s;
  std::transform(u.begin(), u.end(), u.begin(),
                 [](unsigned char c){ return std::toupper(c); });
  return u;
}

static bool read_file(const std::string& path, std::vector<unsigned char>& out) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return false;
  std::fseek(f, 0, SEEK_END);
  long n = std::ftell(f);
  if (n < 0) { std::fclose(f); return false; }
  std::fseek(f, 0, SEEK_SET);
  out.resize((size_t)n);
  size_t r = std::fread(out.data(), 1, out.size(), f);
  std::fclose(f);
  return r == out.size();
}
static bool write_file_temp(const std::string& path, const std::vector<unsigned char>& buf) {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  size_t w = std::fwrite(buf.data(), 1, buf.size(), f);
  std::fclose(f);
  return w == buf.size();
}

// Run objdump and collect lines like: "00000000: 00000513    addi a0,zero,0"
static std::vector<std::string> disasm_with_objdump(const std::string& objdump_path,
                                                    const std::vector<unsigned char>& bytes,
                                                    bool rv64 = false) {
  char tmpTemplate[] = "/tmp/mut_selftest_XXXXXX";
  int fd = mkstemp(tmpTemplate);
  if (fd < 0) {
    std::perror("mkstemp");
    return {};
  }
  std::string binpath = std::string(tmpTemplate) + ".bin";
  ::close(fd);
  if (!write_file_temp(binpath, bytes)) {
    std::fprintf(stderr, "[!] Failed to write temp binary: %s\n", binpath.c_str());
    ::unlink(tmpTemplate);
    return {};
  }

  std::ostringstream cmd;
  cmd << "'" << objdump_path << "'"
      << " -b binary"
      << " -m " << (rv64 ? "riscv:rv64" : "riscv:rv32")
      << " -M rvc,numeric"
      << " -D -w "
      << "'" << binpath << "' 2>/dev/null";

  std::vector<std::string> out_lines;
  FILE* pipe = popen(cmd.str().c_str(), "r");
  if (!pipe) {
    std::fprintf(stderr, "[!] popen failed for objdump (cmd: %s)\n", cmd.str().c_str());
    ::unlink(binpath.c_str());
    ::unlink(tmpTemplate);
    return {};
  }
  char* line = nullptr; size_t cap = 0;
  while (getline(&line, &cap, pipe) != -1) {
    std::string s(line);
    if (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    size_t i = s.find_first_not_of(' ');
    if (i != std::string::npos && i + 9 < s.size() && std::isxdigit((unsigned char)s[i])) {
      size_t colon = s.find(':', i);
      if (colon != std::string::npos) out_lines.push_back(s);
    }
  }
  if (line) free(line);
  pclose(pipe);
  ::unlink(binpath.c_str());
  ::unlink(tmpTemplate);
  return out_lines;
}

static void print_side_by_side(const std::string& left,
                               const std::string& right,
                               size_t pad = 64) {
  std::string L_fmt = format_disasm_line(left);
  std::string R_fmt = format_disasm_line(right);
  bool changed = (left != right);

  if (L_fmt.size() < pad)
    L_fmt.append(pad - L_fmt.size(), ' ');

  if (changed) {
    std::cout << C(C_RED) << L_fmt << C(C_RESET) << ' '
              << C(C_RED) << "→ " << C(C_GREEN) << R_fmt << C(C_RESET)
              << '\n';
  } else {
    std::cout << L_fmt << "   " << R_fmt << '\n';
  }
}

static void show_diff_disasm(const std::vector<unsigned char>& before,
                             const std::vector<unsigned char>& after,
                             const std::string& objdump_path,
                             bool rv64,
                             int step_idx, int step_total,
                             size_t pad,
                             const std::string& strategy) {
  std::cout << C(C_CYAN)
            << "\n──────────────────────────────────────────────────────────────────────────────\n"
            << " Step " << step_idx << "/" << step_total
            << " — Strategy: " << strategy
            << " — Disassembly (system objdump) — BEFORE  vs  AFTER\n"
            << "    Format:  PC: HEX   ASM\n"
            << "──────────────────────────────────────────────────────────────────────────────" << C(C_RESET) << "\n";

  auto L = disasm_with_objdump(objdump_path, before, rv64);
  auto R = disasm_with_objdump(objdump_path, after, rv64);

  size_t n = std::max(L.size(), R.size());
  for (size_t i = 0; i < n; ++i) {
    std::string l = (i < L.size()) ? L[i] : "";
    std::string r = (i < R.size()) ? R[i] : "";
    print_side_by_side(l, r, pad);
  }

  std::cout << C(C_CYAN)
            << "──────────────────────────────────────────────────────────────────────────────\n"
            << C(C_RESET);
}

static void dump_hex(const char* tag, const std::vector<unsigned char>& buf, size_t max_bytes = 64) {
  std::cout << C(C_CYAN) << "\n[" << tag << "] " << buf.size() << " bytes" << C(C_RESET) << "\n";
  size_t n = std::min(buf.size(), max_bytes);
  for (size_t i = 0; i < n; ++i) {
    if ((i % 16) == 0) std::cout << std::setw(6) << std::setfill(' ') << i << ": ";
    std::cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned)buf[i] << " ";
    if ((i % 16) == 15) std::cout << "\n";
  }
  if (n < buf.size()) std::cout << "... (" << (buf.size() - n) << " more bytes)\n";
  std::cout << std::dec;
}

static bool glob_first_bin(const std::string& pattern, std::string& out_path) {
  glob_t g;
  memset(&g, 0, sizeof(g));
  int rc = ::glob(pattern.c_str(), GLOB_TILDE, nullptr, &g);
  if (rc != 0 || g.gl_pathc == 0) { globfree(&g); return false; }
  out_path = std::string(g.gl_pathv[0]);
  globfree(&g);
  return true;
}
static bool find_seed_bin(std::string& out_path) {
  const char* patterns[] = {
    "../../seeds/*.bin",
    "../seeds/*.bin",
    "seeds/*.bin"
  };
  for (const char* p : patterns) {
    if (glob_first_bin(p, out_path)) return true;
  }
  return false;
}

// ---------- CLI ----------
struct Args {
  std::string lib = "../libisa_mutator.so";
  std::string in  = "";
  unsigned int seed = 12345;
  int repeat = 1;
  std::string objdump = env_or("OBJDUMP", "riscv32-unknown-elf-objdump");
  bool rv64 = false;
  size_t pad = 64;
  std::string config;
};
static void usage(const char* prog) {
  std::cout <<
    "Usage: " << prog << " [--lib path.so] [--config file.yaml] [--in input.bin] [--seed N] [--repeat N] [--objdump PATH] [--width W]\n"
    "Env:   NO_COLOR=1 disables colors\n"
    "       OBJDUMP=/path/to/riscv32-unknown-elf-objdump (default)\n"
    "       XLEN=32|64 (affects disassembly only)\n";
}
static Args parse_args(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    std::string s(argv[i]);
    if (s == "--lib" && i+1 < argc) a.lib = argv[++i];
    else if (s == "--config" && i+1 < argc) a.config = argv[++i];
    else if (s == "--in" && i+1 < argc) a.in = argv[++i];
    else if (s == "--seed" && i+1 < argc) a.seed = (unsigned)std::strtoul(argv[++i], nullptr, 0);
    else if (s == "--repeat" && i+1 < argc) a.repeat = std::max(1, (int)std::strtol(argv[++i], nullptr, 0));
    else if (s == "--objdump" && i+1 < argc) a.objdump = argv[++i];
    else if (s == "--width" && i+1 < argc) a.pad = std::max(32, (int)std::strtol(argv[++i], nullptr, 0));
    else if (s == "--help" || s == "-h") { usage(argv[0]); std::exit(0); }
    else if (s == "--debug") { setenv("DEBUG_MUTATOR", "1", 1); }
    else if (s == "--debug-log" && i+1 < argc) { setenv("DEBUG_LOG", argv[++i], 1); }
    else { std::cerr << "[!] Unknown arg: " << s << "\n"; usage(argv[0]); std::exit(1); }
  }
  std::string xlen = env_or("XLEN", "32");
  a.rv64 = (xlen == "64");
  return a;
}

// ---------- Mutate once via classic API ----------
static std::vector<unsigned char>
mutate_once(afl_mut_fn afl_custom_mutator,
            const std::vector<unsigned char>& before,
            unsigned int seed) {
  if (!afl_custom_mutator) return before;

  // We pass nullptr for afl_state*, same as many simple custom mutators.
  unsigned char* out_ptr = nullptr;

  // Use before.size() as max_size cap; mutator may allocate internally anyway.
  size_t n = afl_custom_mutator(nullptr,
                                const_cast<unsigned char*>(before.data()),
                                before.size(),
                                &out_ptr,
                                before.size());

  if (!out_ptr || n == 0) return before;

  std::vector<unsigned char> out(out_ptr, out_ptr + n);
  // The mutator uses malloc; free here is fine for the test harness.
  std::free(out_ptr);
  return out;
}

int main(int argc, char** argv) {
  if (std::getenv("NO_COLOR")) g_color = false;
  Args A = parse_args(argc, argv);
  std::string strategy = get_strategy();

  std::cout << C(C_CYAN) << "[*] Mutator self-test using system disassembler\n" << C(C_RESET);
  std::cout << "    lib:     " << A.lib << "\n"
            << "    objdump: " << A.objdump << "\n"
            << "    seed:    " << A.seed << "\n"
            << "    repeat:  " << A.repeat << "\n"
            << "    strategy:" << strategy << "\n"
            << "    XLEN:    " << (A.rv64 ? "64" : "32") << "\n";

  if (!file_exists(A.lib)) {
    std::cerr << C(C_YEL) << "[!] Library not found: " << A.lib << C(C_RESET) << "\n";
    return 1;
  }

  void* handle = dlopen(A.lib.c_str(), RTLD_NOW);
  if (!handle) {
    std::cerr << "[!] dlopen failed: " << dlerror() << "\n";
    return 1;
  }

  auto afl_custom_init_fn   = (afl_init_fn )dlsym(handle, "afl_custom_init");
  auto afl_custom_mutator   = (afl_mut_fn  )dlsym(handle, "afl_custom_mutator");
  auto afl_custom_havoc_fn  = (afl_mut_fn  )dlsym(handle, "afl_custom_havoc_mutation");
  (void)afl_custom_havoc_fn;
  auto afl_custom_deinit_fn = (afl_deinit_fn)dlsym(handle, "afl_custom_deinit");
  using set_cfg_fn = void (*)(const char *);
  auto set_config_fn = (set_cfg_fn)dlsym(handle, "mutator_set_config_path");

  if (!afl_custom_init_fn || !afl_custom_mutator || !afl_custom_deinit_fn) {
    std::cerr << "[!] Failed to resolve required AFL custom mutator symbols\n"
              << "    Need: afl_custom_init, afl_custom_mutator, afl_custom_deinit\n"
              << "    Optional: afl_custom_havoc_mutation\n";
    dlclose(handle);
    return 1;
  }

  // Init mutator
  if (!A.config.empty()) {
    if (set_config_fn)
      set_config_fn(A.config.c_str());
    else
      std::cerr << "[!] --config ignored: mutator_set_config_path not available\n";
  }
  afl_custom_init_fn(nullptr);

  // Sanity: objdump availability
  {
    std::string probe = A.objdump + " --version > /dev/null 2>&1";
    int _unused_sys = std::system(probe.c_str());  // <-- replace system() call
    (void)_unused_sys;
  }

  // Prepare input
  std::vector<unsigned char> cur;
  if (!A.in.empty()) {
    if (!read_file(A.in, cur)) {
      std::cerr << "[!] Failed to read input file: " << A.in << "\n";
      afl_custom_deinit_fn(); dlclose(handle); return 1;
    }
  } else {
    std::string seed_path;
    if (find_seed_bin(seed_path)) {
      std::cout << "    seed:    " << seed_path << "\n";
      if (!read_file(seed_path, cur)) {
        std::cerr << "[!] Found seed but failed to read: " << seed_path << "\n";
        afl_custom_deinit_fn(); dlclose(handle); return 1;
      }
    } else {
      unsigned char tiny[] = {
        0x13, 0x05, 0x00, 0x00, // addi a0,zero,0
        0x33, 0x85, 0x40, 0x00  // add a0,a1,a6
      };
      std::cout << "    seed:    (no seeds/*.bin found; using tiny built-in sample)\n";
      cur.assign(tiny, tiny + sizeof(tiny));
    }
  }

  std::cout << "    input:   " << cur.size() << " bytes\n";
  dump_hex("BEFORE (initial)", cur);

  // Repeat mutations
  for (int step = 1; step <= A.repeat; ++step) {
    // Seed influence is internal to your mutator; we just vary it per step if needed.
    (void)A.seed; // kept for UI; not passed via classic API

    auto next = mutate_once(afl_custom_mutator, cur, /*seed*/ A.seed + (unsigned)(step - 1));
    dump_hex("AFTER  (this step)", next);
    show_diff_disasm(cur, next, A.objdump, A.rv64, step, A.repeat, A.pad, strategy);
    cur.swap(next);
  }

  afl_custom_deinit_fn();
  dlclose(handle);
  return 0;
}
