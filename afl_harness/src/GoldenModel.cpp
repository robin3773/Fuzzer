#include "GoldenModel.hpp"
#include "SpikeHelpers.hpp"
#include <hwfuzz/Debug.hpp>
#include <cstdlib>

GoldenModel::GoldenModel() 
  : golden_ready_(false), trace_enabled_(false), golden_mode_("live") {
}

GoldenModel::~GoldenModel() {
  stop();
}

bool GoldenModel::initialize(const std::vector<unsigned char>& input, const char* trace_dir) {
  // Read configuration from environment
  const char* golden_mode_env = std::getenv("GOLDEN_MODE");
  const char* spike_env = std::getenv("SPIKE_BIN");
  const char* spike_isa_env = std::getenv("SPIKE_ISA");
  const char* pk_env = std::getenv("PK_BIN");
  const char* spike_log_env = std::getenv("SPIKE_LOG_FILE");
  const char* trace_mode_env = std::getenv("TRACE_MODE");

  if (golden_mode_env && *golden_mode_env) {
    golden_mode_ = std::string(golden_mode_env);
  }

  // Check if golden model should be disabled
  if (golden_mode_ == "off" || golden_mode_ == "none" || golden_mode_ == "0") {
    hwfuzz::debug::logInfo("[GOLDEN] Golden model disabled (GOLDEN_MODE=%s)\n", golden_mode_.c_str());
    return false;
  }

  if (golden_mode_ == "batch" || golden_mode_ == "replay") {
    hwfuzz::debug::logInfo("[GOLDEN] GOLDEN_MODE=%s; external replay/tools should be used.\n", golden_mode_.c_str());
    return false;
  }

  // Only proceed for "live" mode
  if (golden_mode_ != "live") {
    hwfuzz::debug::logWarn("[GOLDEN] Unknown GOLDEN_MODE=%s, defaulting to live\n", golden_mode_.c_str());
    golden_mode_ = "live";
  }

  if (!spike_env || !*spike_env) {
    hwfuzz::debug::logInfo("[GOLDEN] SPIKE_BIN not set; golden model disabled\n");
    return false;
  }

  std::string spike_bin(spike_env);
  std::string spike_isa = spike_isa_env && *spike_isa_env ? std::string(spike_isa_env) : "rv32imc";
  std::string pk_bin = pk_env && *pk_env ? std::string(pk_env) : "";

  // Set log path
  if (spike_log_env && *spike_log_env) {
    spike_.set_log_path(spike_log_env);
  }

  // Build temporary ELF from input
  tmp_elf_ = spike_helpers::build_spike_elf(input);
  if (tmp_elf_.empty()) {
    hwfuzz::debug::logError("[GOLDEN] Failed to build Spike ELF; disabling golden model\n");
    return false;
  }

  // Start Spike process
  if (!spike_.start(spike_bin, tmp_elf_, spike_isa, pk_bin)) {
    hwfuzz::debug::logError("[GOLDEN] Failed to start Spike.\n  Command: %s\n  ELF: %s\n", 
                            spike_.command().c_str(), tmp_elf_.c_str());
    if (spike_log_env && *spike_log_env) {
      hwfuzz::debug::logError("[GOLDEN]   See Spike log: %s\n", spike_log_env);
      spike_helpers::print_log_tail(spike_log_env, 60);
    }
    return false;
  }

  golden_ready_ = true;
  hwfuzz::debug::logInfo("[GOLDEN] Spike golden model started successfully\n");

  // Setup golden trace if enabled
  trace_enabled_ = true;
  if (trace_mode_env && (std::string(trace_mode_env) == "off" || std::string(trace_mode_env) == "0")) {
    trace_enabled_ = false;
  }

  if (trace_enabled_) {
    hwfuzz::debug::logInfo("[GOLDEN] Opening golden trace in %s\n", trace_dir);
    golden_tracer_.open_with_basename(trace_dir, "golden.trace");
  }

  return true;
}

bool GoldenModel::next_commit(CommitRec& rec) {
  if (!golden_ready_) return false;

  if (spike_.next_commit(rec)) {
    write_trace(rec);
    return true;
  }

  // Spike stopped producing commits
  golden_ready_ = false;

  if (spike_.saw_fatal_trap()) {
    const std::string& trap = spike_.fatal_trap_summary();
    hwfuzz::debug::logWarn("[GOLDEN] Spike aborted on fatal trap (%s); disabling golden checks.\n"
                           "  Command: %s\n  ELF: %s\n  Status: %s\n",
                           trap.empty() ? "unknown" : trap.c_str(),
                           spike_.command().c_str(),
                           tmp_elf_.c_str(),
                           spike_.status_string().c_str());
  } else {
    hwfuzz::debug::logWarn("[GOLDEN] Spike stopped producing commits; disabling golden checks.\n"
                           "  Command: %s\n  ELF: %s\n",
                           spike_.command().c_str(), tmp_elf_.c_str());
  }

  const char* spike_log = std::getenv("SPIKE_LOG_FILE");
  if (spike_log && *spike_log) {
    hwfuzz::debug::logWarn("[GOLDEN]   See Spike log: %s\n", spike_log);
    spike_helpers::print_log_tail(spike_log, 60);
  }

  return false;
}

void GoldenModel::write_trace(const CommitRec& rec) {
  if (trace_enabled_) {
    golden_tracer_.write(rec);
  }
}

void GoldenModel::stop() {
  if (golden_ready_) {
    spike_.stop();
    golden_ready_ = false;
  }
  if (!tmp_elf_.empty()) {
    ::unlink(tmp_elf_.c_str());
    tmp_elf_.clear();
  }
}
