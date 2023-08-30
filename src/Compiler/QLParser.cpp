#include "QLParser.h"

#include <regex>

namespace FOEDAG {

std::unordered_map<std::string, std::optional<int>> QLParser::extractDeviceAvailableResourcesFromVprLogContent(const std::string& content)
{
  static std::regex gridGenericLogPattern(R"(FPGA sized to \d+ x \d+: \d+ grid tiles \((\d+)x(\d+)\))");
  static std::regex clbLogPattern(R"(Architecture\s+(\d+)\s+blocks of type: clb)");
  static std::regex dspLogPattern(R"(Architecture\s+(\d+)\s+blocks of type: dsp)");
  static std::regex bramLogPattern(R"(Architecture\s+(\d+)\s+blocks of type: bram)");

  std::smatch match;
  std::unordered_map<std::string, std::optional<int>> result;
  if (std::regex_search(content, match, clbLogPattern)) {
    result["clb"] = std::atoi(match[1].str().c_str());
  }
  if (std::regex_search(content, match, dspLogPattern)) {
    result["dsp"] = std::atoi(match[1].str().c_str());
  }
  if (std::regex_search(content, match, bramLogPattern)) {
    result["bram"] = std::atoi(match[1].str().c_str());
  }

  return result;
}

}  // namespace FOEDAG
