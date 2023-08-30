#pragma once

#include <unordered_map>
#include <string>
#include <optional>

namespace FOEDAG {

class QLParser {
 public:
  static std::unordered_map<std::string, std::optional<int>> extractDeviceAvailableResourcesFromVprLogContent(const std::string&);
};

}  // namespace FOEDAG


