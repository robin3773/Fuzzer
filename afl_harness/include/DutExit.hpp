#pragma once

#include "HarnessConfig.hpp"
#include <cstdint>
#include <string>
#include <vector>

// =============================================================================
// DUT Exit Handling — Clean termination and exit stub generation
// =============================================================================

enum class ExitReason {
  None,
  Finish,
  Tohost,
  Ecall,
  SpikeDone
};

const char* exit_reason_text(ExitReason reason);
