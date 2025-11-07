#include "DutExit.hpp"

const char* exit_reason_text(ExitReason reason) {
  switch (reason) {
    case ExitReason::Finish:   return "dut_finish";
    case ExitReason::Tohost:   return "tohost";
    case ExitReason::Ecall:    return "ecall";
    case ExitReason::SpikeDone:return "spike_done";
    default:                   return "none";
  }
}
