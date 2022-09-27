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
#include "compiler_configuration.h"

namespace FOEDAG {

const std::vector<std::string> &CompilerConfiguration::includePathList() const {
  return m_includePathList;
}

void CompilerConfiguration::setIncludePathList(
    const std::vector<std::string> &newIncludePathList) {
  m_includePathList = newIncludePathList;
}

void CompilerConfiguration::addIncludePath(const std::string &includePath) {
  m_includePathList.push_back(includePath);
}

const std::vector<std::string> &CompilerConfiguration::libraryPathList() const {
  return m_libraryPathList;
}

void CompilerConfiguration::setLibraryPathList(
    const std::vector<std::string> &newLibraryPathList) {
  m_libraryPathList = newLibraryPathList;
}

void CompilerConfiguration::addLibraryPath(const std::string &libraryPath) {
  m_libraryPathList.push_back(libraryPath);
}

const std::vector<std::string> &CompilerConfiguration::libraryExtensionList()
    const {
  return m_libraryExtList;
}

void CompilerConfiguration::setLibraryExtensionList(
    const std::vector<std::string> &newLibraryExtensionList) {
  m_libraryExtList = newLibraryExtensionList;
}

void CompilerConfiguration::addLibraryExtension(const std::string &libraryExt) {
  m_libraryExtList.push_back(libraryExt);
}

void CompilerConfiguration::addMacro(const std::string &macroName,
                                     const std::string &macroValue) {
  m_macroList.push_back(std::pair(macroName, macroValue));
}

const std::vector<std::pair<std::string, std::string> >
    &CompilerConfiguration::macroList() const {
  return m_macroList;
}
}  // namespace FOEDAG
