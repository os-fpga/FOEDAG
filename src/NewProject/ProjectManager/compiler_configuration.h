/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

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
#pragma once
#include <string>
#include <vector>

namespace FOEDAG {

class CompilerConfiguration {
 public:
  CompilerConfiguration() = default;

  const std::vector<std::string> &includePathList() const;
  void setIncludePathList(const std::vector<std::string> &newIncludePathList);
  void addIncludePath(const std::string &includePath);

  const std::vector<std::string> &libraryPathList() const;
  void setLibraryPathList(const std::vector<std::string> &newLibraryPathList);
  void addLibraryPath(const std::string &libraryPath);

  const std::vector<std::string> &libraryExtensionList() const;
  void setLibraryExtensionList(
      const std::vector<std::string> &newLibraryExtensionList);
  void addLibraryExtension(const std::string &libraryExt);

  void addMacro(const std::string &macroName, const std::string &macroValue);
  const std::vector<std::pair<std::string, std::string>> &macroList() const;

  void setTargetDevice(const std::string &deviceName);
  const std::string &getTargetDevice() const;

 private:
  std::vector<std::string> m_includePathList;
  std::vector<std::string> m_libraryPathList;
  std::vector<std::string> m_libraryExtList;
  std::vector<std::pair<std::string, std::string>> m_macroList;
  std::string m_deviceName;
};

}  // namespace FOEDAG
