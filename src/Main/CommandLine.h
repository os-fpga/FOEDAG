/*
Copyright 2021 The Foedag team

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
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

extern const char* foedag_version_str;

namespace FOEDAG {

class CommandLine {
 private:
 public:
  CommandLine(int argc, char** argv) : m_argc(argc), m_argv(argv) {}

  virtual ~CommandLine();

  bool WithQt() const { return m_withQt; }

  bool WithQml() const { return m_withQml; }

  const std::vector<std::string>& Args() const { return m_args; }

  const std::string& GuiTestScript() const { return m_runGuiTest; }

  const std::string& Script() const { return m_runScript; }

  const std::string& TclCmd() const { return m_runTclCmd; }

  const std::string& CompilerName() const { return m_compilerName; }

  bool UseVerific() { return m_useVerific; }

  bool PrintHelp() { return m_help; }
  bool PrintVersion() { return m_version; }
  virtual void processArgs();

  int Argc() { return m_argc; }
  char** Argv() { return m_argv; }

  void ErrorAndExit(const std::string& message);
  bool FileExists(const std::filesystem::path& name);
  bool Mute() const { return m_mute; }

 protected:
  int m_argc = 0;
  char** m_argv = nullptr;
  std::vector<std::string> m_args;
  bool m_withQt = true;
  bool m_withQml = false;
  std::string m_runScript;
  std::string m_runGuiTest;
  std::string m_runTclCmd;
  std::string m_compilerName;
  bool m_help = false;
  bool m_version = false;
  bool m_useVerific = false;
  bool m_mute = false;
};

}  // namespace FOEDAG

#endif
