#include "GoldenModel.hpp"
#include "SpikeHelpers.hpp"
#include "HarnessConfig.hpp"
#include <hwfuzz/Debug.hpp>

GoldenModel::GoldenModel() 
  : golden_ready_(false), trace_enabled_(false), golden_mode_("live") {
}

GoldenModel::~GoldenModel() {
  stop();
}

bool GoldenModel::initialize(const std::vector<unsigned char>& input, const HarnessConfig& cfg) {
  // Read configuration from HarnessConfig
  if (!cfg.golden_mode.empty()) {
    golden_mode_ = cfg.golden_mode;
  } else {
    golden_mode_ = "live";
  }
  spike_log_path_.clear();

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

  if (cfg.spike_bin.empty()) {
    hwfuzz::debug::logInfo("[GOLDEN] SPIKE_BIN not set in harness.conf; golden model disabled\n");
    return false;
  }

  std::string spike_bin(cfg.spike_bin);
  std::string spike_isa = cfg.spike_isa.empty() ? "rv32imc" : cfg.spike_isa;
  std::string pk_bin = cfg.pk_bin;

  // Set log path
  spike_log_path_ = cfg.spike_log_file;
  spike_.set_log_path(spike_log_path_);

  // Build temporary ELF from input
  tmp_elf_ = spike_helpers::build_spike_elf(input, cfg.ld_bin, cfg.linker_script);
  if (tmp_elf_.empty()) {
    hwfuzz::debug::logError("[GOLDEN] Failed to build Spike ELF; disabling golden model\n");
    return false;
  }

  // Start Spike process
  if (!spike_.start(spike_bin, tmp_elf_, spike_isa, pk_bin)) {
    hwfuzz::debug::logError("[GOLDEN] Failed to start Spike.\n  Command: %s\n  ELF: %s\n  Error: %s\n", 
                            spike_.command().c_str(), tmp_elf_.c_str(), spike_.last_error().c_str());
    if (!spike_log_path_.empty()) {
      hwfuzz::debug::logError("[GOLDEN]   See Spike log: %s\n", spike_log_path_.c_str());
      spike_helpers::print_log_tail(spike_log_path_.c_str(), 60);
    }
    return false;
  }

  golden_ready_ = true;
  hwfuzz::debug::logInfo("[GOLDEN] Spike golden model started successfully\n");

  // Setup golden trace if enabled
  trace_enabled_ = cfg.trace_enabled;

  if (trace_enabled_) {
    hwfuzz::debug::logInfo("[GOLDEN] Opening golden trace in %s\n", cfg.trace_dir.c_str());
    golden_tracer_.open_with_basename(cfg.trace_dir, "golden.trace");
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

  if (!spike_log_path_.empty()) {
    hwfuzz::debug::logWarn("[GOLDEN]   See Spike log: %s\n", spike_log_path_.c_str());
    spike_helpers::print_log_tail(spike_log_path_.c_str(), 60);
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
