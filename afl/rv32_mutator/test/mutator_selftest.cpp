// ==========================================================
// mutator_selftest.cpp — RV32/RV64 Mutator Standalone Tester
// (with --repeat N, clear disassembly boundaries, and AFL API fallbacks)
// ==========================================================
// Features:
// - Dynamically loads your AFL++ custom mutator shared object
// - Supports BOTH afl_custom_fuzz (new API) and afl_custom_fuzz_b (old API)
// - Calls afl_custom_init(), afl_custom_deinit()
// - Disassembles BEFORE and AFTER buffers with system objdump:
//     riscv64-unknown-elf-objdump -b binary -m riscv:rv32 -M rvc,numeric -D -w
// - Prints side-by-side lines: "PC: HEX  ASM   →   PC: HEX  ASM"
// - Colors the right side green if the disassembly line changed
// - NEW: --repeat N  ⇒ applies N sequential mutations with banners
// - NEW: --width W   ⇒ set side-by-side padding width (default 64)
// - Safer output-length inference and better diagnostics
//
// Build:
//   g++ -std=c++17 mutator_selftest.cpp -ldl -o mutator_selftest
//
// Run (defaults):
//   ./mutator_selftest
//
// With custom lib / input / seed / repeats:
//   ./mutator_selftest \
//       --lib ./afl/rv32_mutator/librv32_mutator.so \
//       --in ./firmware/build/firmware.bin \
//       --seed 12345 \
//       --repeat 5
//
// Env knobs:
//   OBJDUMP=/path/to/riscv64-unknown-elf-objdump   (default: riscv64-unknown-elf-objdump)
//   NO_COLOR=1                                     (disable ANSI colors)
//   XLEN=32|64                                     (select disasm mode)
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

// ---------- AFL custom mutator signatures ----------
using afl_fuzz_fn   = unsigned char *(*)(unsigned char *buf, size_t buf_size,
                                         unsigned char *out_buf, size_t max_size,
                                         unsigned int seed);
using afl_fuzz_b_fn = size_t (*)(unsigned char *data, size_t size,
                                 unsigned char **out_buf, unsigned int seed);
using afl_init_fn   = int (*)(void *);
using afl_deinit_fn = void (*)();

// ---------- ANSI colors ----------
static bool g_color = true;
static inline const char* C(const char* s) { return g_color ? s : ""; }
static constexpr const char* C_RESET = "\033[0m";
static constexpr const char* C_GREEN = "\033[1;32m";
static constexpr const char* C_CYAN  = "\033[1;36m";
static constexpr const char* C_YEL   = "\033[1;33m";
static constexpr const char* C_MIDB  = "\033[1;34m";

// ---------- Helpers ----------
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
  // create temp file
  char tmpTemplate[] = "/tmp/mut_selftest_XXXXXX";
  int fd = mkstemp(tmpTemplate);
  if (fd < 0) {
    std::perror("mkstemp");
    return {};
  }
  std::string binpath = std::string(tmpTemplate) + ".bin";
  ::close(fd); // just needed a unique prefix
  if (!write_file_temp(binpath, bytes)) {
    std::fprintf(stderr, "[!] Failed to write temp binary: %s\n", binpath.c_str());
    ::unlink(tmpTemplate);
    return {};
  }

  // Build command
  std::ostringstream cmd;
  cmd << "'" << objdump_path << "'"
      << " -b binary"
      << " -m " << (rv64 ? "riscv:rv64" : "riscv:rv32")
      << " -M rvc,numeric"
      << " -D -w "
      << "'" << binpath << "' 2>/dev/null";

  // Run command
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
    // keep lines that start with hex address and a colon
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

// Format a side-by-side line with padding and change highlight
static void print_side_by_side(const std::string& left, const std::string& right, size_t pad = 64) {
  std::string L = left;
  std::string R = right;
  bool changed = (L != R);
  if (L.size() < pad) L.append(pad - L.size(), ' ');
  if (changed) {
    std::cout << L << " " << C(C_GREEN) << "→  " << R << C(C_RESET) << "\n";
  } else {
    std::cout << L << "   " << R << "\n";
  }
}

// Disassemble both and show side-by-side lines (PC: HEX ASM)
// Adds a clear banner boundary per step.
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

  // Tail boundary line
  std::cout << C(C_CYAN)
            << "──────────────────────────────────────────────────────────────────────────────\n"
            << C(C_RESET);
}

// ---------- Hex dump (compact) ----------
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

// ---------- Seed discovery ----------
static bool glob_first_bin(const std::string& pattern, std::string& out_path) {
  glob_t g;
  memset(&g, 0, sizeof(g));
  int rc = ::glob(pattern.c_str(), GLOB_TILDE, nullptr, &g);
  if (rc != 0 || g.gl_pathc == 0) { globfree(&g); return false; }
  // Pick the first lexicographic match
  out_path = std::string(g.gl_pathv[0]);
  globfree(&g);
  return true;
}

static bool find_seed_bin(std::string& out_path) {
  // Try typical locations relative to afl/rv32_mutator/test/
  const char* patterns[] = {
    "../../seeds/*.bin",  // project-root/afl/seeds
    "../seeds/*.bin",     // afl/rv32_mutator/seeds
    "seeds/*.bin"         // afl/rv32_mutator/test/seeds (rare)
  };
  for (const char* p : patterns) {
    if (glob_first_bin(p, out_path)) return true;
  }
  return false;
}

// ---------- CLI parsing ----------
struct Args {
  std::string lib = "../librv32_mutator.so"; // relative to test/
  std::string in  = "";
  unsigned int seed = 12345;
  int repeat = 1;
  std::string objdump = env_or("OBJDUMP", "riscv64-unknown-elf-objdump");
  bool rv64 = false; // future-ready
  size_t pad = 64;   // side-by-side width
};

static void usage(const char* prog) {
  std::cout <<
    "Usage: " << prog << " [--lib path.so] [--in input.bin] [--seed N] [--repeat N] [--objdump PATH] [--width W]\n"
    "Env:   NO_COLOR=1 disables colors\n"
    "       OBJDUMP=/path/to/riscv64-unknown-elf-objdump (default)\n"
    "       XLEN=32|64 (affects disassembly only)\n";
}

static Args parse_args(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    std::string s(argv[i]);
    if (s == "--lib" && i+1 < argc) a.lib = argv[++i];
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

// ---------- Mutate once (prefers afl_custom_fuzz_b if available) ----------
static std::vector<unsigned char> mutate_once(afl_fuzz_fn afl_custom_fuzz,
                                              afl_fuzz_b_fn afl_custom_fuzz_b,
                                              const std::vector<unsigned char>& before,
                                              std::vector<unsigned char>& scratch_outbuf,
                                              unsigned int seed) {
  if (afl_custom_fuzz_b) {
    // Use old API to get authoritative length
    unsigned char* out_ptr = nullptr;
    size_t n = afl_custom_fuzz_b(const_cast<unsigned char*>(before.data()), before.size(), &out_ptr, seed);
    if (!out_ptr || n == 0) return before; // no-op fallback
    return std::vector<unsigned char>(out_ptr, out_ptr + n);
  }

  // Fallback to modern API
  scratch_outbuf.assign(std::max<size_t>(before.size() + 16, 4096), 0);
  unsigned char* mutated = afl_custom_fuzz(
    const_cast<unsigned char*>(before.data()), before.size(),
    scratch_outbuf.data(), scratch_outbuf.size(), seed);

  if (!mutated) return before; // defensive

  if (mutated == scratch_outbuf.data()) {
    // Infer output size by probing non-zero tail (best effort)
    size_t cap = scratch_outbuf.size();
    size_t rounded = ((before.size() + 3u) / 4u) * 4u;
    size_t probe_max = std::min(rounded + 16, cap);
    size_t last_nz = 0;
    for (size_t i = 0; i < probe_max; ++i) {
      if (scratch_outbuf[i] != 0) last_nz = i + 1;
    }
    size_t after_len = std::max(rounded, last_nz);
    return std::vector<unsigned char>(scratch_outbuf.begin(), scratch_outbuf.begin() + after_len);
  } else {
    // Heap pointer case: keep same length (mutator didn't provide a size)
    return std::vector<unsigned char>(mutated, mutated + before.size());
  }
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

  // Load library
  void* handle = dlopen(A.lib.c_str(), RTLD_NOW);
  if (!handle) {
    std::cerr << "[!] dlopen failed: " << dlerror() << "\n";
    return 1;
  }

  auto afl_custom_init    = (afl_init_fn  )dlsym(handle, "afl_custom_init");
  auto afl_custom_fuzz    = (afl_fuzz_fn  )dlsym(handle, "afl_custom_fuzz");
  auto afl_custom_fuzz_b  = (afl_fuzz_b_fn)dlsym(handle, "afl_custom_fuzz_b");
  auto afl_custom_deinit  = (afl_deinit_fn)dlsym(handle, "afl_custom_deinit");
  if (!afl_custom_init || !afl_custom_fuzz || !afl_custom_deinit) {
    std::cerr << "[!] Failed to resolve required AFL custom mutator symbols\n"
              << "    Need: afl_custom_init, afl_custom_fuzz, afl_custom_deinit\n"
              << "    Optional: afl_custom_fuzz_b\n";
    dlclose(handle);
    return 1;
  }

  // Init mutator
  afl_custom_init(nullptr);

  // Sanity: verify objdump exists in PATH (best effort)
  {
    std::string probe = A.objdump + " --version > /dev/null 2>&1";
    int rc = std::system(probe.c_str());
    if (rc != 0) {
      std::cerr << C(C_YEL) << "[!] Warning: " << A.objdump
                << " not invokable. Set OBJDUMP env to your riscv64-unknown-elf-objdump.\n"
                << "    Disassembly output will be empty." << C(C_RESET) << "\n";
    }
  }

  // Prepare input
  std::vector<unsigned char> cur;
  if (!A.in.empty()) {
    if (!read_file(A.in, cur)) {
      std::cerr << "[!] Failed to read input file: " << A.in << "\n";
      afl_custom_deinit(); dlclose(handle); return 1;
    }
  } else {
    // Auto-pick a seed from seeds/*.bin under afl/
    std::string seed_path;
    if (find_seed_bin(seed_path)) {
      std::cout << "    seed:    " << seed_path << "\n";
      if (!read_file(seed_path, cur)) {
        std::cerr << "[!] Found seed but failed to read: " << seed_path << "\n";
        afl_custom_deinit(); dlclose(handle); return 1;
      }
    } else {
      // Fallback tiny default
      unsigned char tiny[] = {
        0x13, 0x05, 0x00, 0x00, // addi a0,zero,0
        0x33, 0x85, 0x40, 0x00  // add a0,a1,a6
      };
      std::cout << "    seed:    (no seeds/*.bin found; using tiny built-in sample)\n";
      cur.assign(tiny, tiny + sizeof(tiny));
    }
  }

  std::cout << "    input:   " << cur.size() << " bytes\n";

  // Initial hex dump (before any mutation)
  dump_hex("BEFORE (initial)", cur);

  // Repeat mutations
  std::vector<unsigned char> scratch;
  for (int step = 1; step <= A.repeat; ++step) {
    unsigned int step_seed = A.seed + (unsigned)(step - 1);
    auto next = mutate_once(afl_custom_fuzz, afl_custom_fuzz_b, cur, scratch, step_seed);

    dump_hex("AFTER  (this step)", next);
    show_diff_disasm(cur, next, A.objdump, A.rv64, step, A.repeat, A.pad, strategy);

    cur.swap(next);
  }

  // Done
  afl_custom_deinit();
  dlclose(handle);
  return 0;
}