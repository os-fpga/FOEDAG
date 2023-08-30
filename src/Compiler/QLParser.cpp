#include "QLParser.h"

#include <QRegularExpression>

#include <optional>

namespace FOEDAG {

std::unordered_map<std::string, std::optional<int>> QLParser::extractDeviceAvailableResourcesFromVprLogContent(const std::string& content)
{
  static QRegularExpression clbLogPattern(R"(Architecture\s+(\d+)\s+blocks of type: clb)");
  static QRegularExpression dspLogPattern(R"(Architecture\s+(\d+)\s+blocks of type: dsp)");
  static QRegularExpression bramLogPattern(R"(Architecture\s+(\d+)\s+blocks of type: bram)");

  auto tryExtractSubInt = [](const QRegularExpression& pattern, const std::string& content) -> std::optional<int> {
    std::optional<int> result;
    auto match = pattern.match(content.c_str());
    if (match.hasMatch() && (match.lastCapturedIndex() == 1)) {
      bool ok;
      int candidate = match.captured(1).toInt(&ok);
      if (ok) {
        result = candidate;
      }
    }
    return result;
  };

  std::unordered_map<std::string, std::optional<int>> result;
  result["clb"] = tryExtractSubInt(clbLogPattern, content);
  result["dsp"] = tryExtractSubInt(dspLogPattern, content);
  result["bram"] = tryExtractSubInt(bramLogPattern, content);

  return result;
}

}  // namespace FOEDAG
