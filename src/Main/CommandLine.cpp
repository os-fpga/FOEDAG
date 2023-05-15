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

#include "CommandLine.h"

#include "Utils/FileUtils.h"

using namespace FOEDAG;

void CommandLine::ErrorAndExit(const std::string& message) {
  std::cerr << "ERROR: " << message << std::endl;
  exit(1);
}

void CommandLine::processArgs() {
  for (int i = 1; i < m_argc; i++) {
    std::string token(m_argv[i]);
    if (token == "--batch") {
      m_withQt = false;
    } else if (token == "--compiler") {
      i++;
      if (i < m_argc)
        m_compilerName = m_argv[i];
      else {
        ErrorAndExit("Specify a valid compiler!");
      }
    } else if (token == "--qml") {
      m_withQml = true;
    } else if (token == "--replay") {
      i++;
      if (i < m_argc) {
        m_runGuiTest = m_argv[i];
        if (!FileUtils::FileExists(m_runGuiTest)) {
          ErrorAndExit("Cannot open replay file: " + m_runGuiTest);
        }
      } else
        ErrorAndExit("Specify a replay file!");
    } else if (token == "--verific") {
      m_useVerific = true;
    } else if (token == "--script") {
      if (!m_projectFile.empty())
        ErrorAndExit("--script and --project can't be used at the same time!");
      i++;
      if (i < m_argc) {
        m_runScript = m_argv[i];
        if (!FileUtils::FileExists(m_runScript)) {
          ErrorAndExit("Cannot open script file: " + m_runScript);
        }
      } else
        ErrorAndExit("Specify a script file!");
    } else if (token == "--project") {
      if (!m_runScript.empty())
        ErrorAndExit("--script and --project can't be used at the same time!");
      i++;
      if (i < m_argc) {
        std::string arg{m_argv[i]};
        if (std::filesystem::is_directory(arg)) {
          auto file = FileUtils::FindFileByExtension(arg, ".ospr");
          m_projectFile = file.string();
          if (file.empty())
            ErrorAndExit("Cannot find project file *.ospr in " + arg);
        } else {
          m_projectFile = arg;
          if (!FileUtils::FileExists(m_projectFile))
            ErrorAndExit("Cannot open project file: " + m_projectFile);
        }
      } else {
        ErrorAndExit("Specify a project file!");
      }
    } else if (token == "--cmd") {
      i++;
      if (i < m_argc) {
        m_runTclCmd = m_argv[i];
      } else
        ErrorAndExit("Specify a Tcl command!");
    } else if (token == "--help") {
      m_help = true;
    } else if (token == "--version") {
      m_version = true;
    } else if (token == "--mute") {
      m_mute = true;
    } else if (token == "--device") {
      i++;
      m_device = m_argv[i];
    } else {
      std::cout << "ERROR Unknown command line option: " << m_argv[i]
                << std::endl;
      exit(1);
    }
  }
}

CommandLine::~CommandLine() {}
