/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <filesystem>
#include <string>

#ifndef TOOL_CONTEXT_H
#define TOOL_CONTEXT_H

namespace FOEDAG {

class ToolContext {
 public:
  ToolContext(std::string ToolName, std::string CompanyName,
              std::string ExecutableName, std::filesystem::path BinaryPath = "",
              std::filesystem::path DataPath = "")
      : m_toolName(ToolName),
        m_companyName(CompanyName),
        m_executableName(ExecutableName),
        m_binaryPath(BinaryPath),
        m_dataPath(DataPath) {}
  ~ToolContext() {}

  const std::string& ToolName() const { return m_toolName; }
  const std::string& CompanyName() const { return m_companyName; }
  const std::string& ExecutableName() const { return m_executableName; }
  const std::filesystem::path& BinaryPath() const { return m_binaryPath; }
  const std::filesystem::path& DataPath() const { return m_dataPath; }
  const std::filesystem::path& ProgrammerGuiPath() const {
    return m_programmerGuiPath;
  }

  void ToolName(const std::string& name) { m_toolName = name; }
  void CompanyName(const std::string& name) { m_companyName = name; }
  void ExecutableName(const std::string& name) { m_executableName = name; }
  void BinaryPath(const std::filesystem::path& path) { m_binaryPath = path; }
  void DataPath(const std::filesystem::path& path) { m_dataPath = path; }
  void ProgrammerGuiPath(const std::filesystem::path& path) {
    m_programmerGuiPath = path;
  }

 private:
  std::string m_toolName;
  std::string m_companyName;
  std::string m_executableName;
  std::filesystem::path m_binaryPath;
  std::filesystem::path m_dataPath;
  std::filesystem::path m_programmerGuiPath;
};

}  // namespace FOEDAG

#endif
