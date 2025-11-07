#include "SpikeExit.hpp"

#include <regex>

namespace {
std::string rstrip(const std::string& s) {
  size_t end = s.size();
  while (end > 0 && (s[end - 1] == '\n' || s[end - 1] == '\r')) {
    --end;
  }
  return s.substr(0, end);
}
}

bool detect_spike_fatal_trap(const std::string& line, std::string& summary) {
  if (line.find("core") == std::string::npos || line.find("exception") == std::string::npos) {
    return false;
  }
  static const std::regex trap_re(R"(core\s+\d+:\s+exception\s+([A-Za-z0-9_]+),\s+epc\s+0x([0-9a-fA-F]+))");
  std::smatch match;
  if (std::regex_search(line, match, trap_re)) {
    summary = match[1].str();
    summary += " at epc=0x";
    summary += match[2].str();
  } else {
    summary = rstrip(line);
  }
  return true;
}
