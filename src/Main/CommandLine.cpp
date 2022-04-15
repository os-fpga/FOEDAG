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

using namespace FOEDAG;

void CommandLine::processArgs() {
  for (int i = 1; i < m_argc; i++) {
    std::string token(m_argv[i]);
    if (token == "--noqt") {
      m_withQt = false;
    } else if (token == "--batch") {
      m_withQt = false;
    } else if (token == "--compiler") {
      i++;
      m_compilerName = m_argv[i];
    } else if (token == "--qml") {
      m_withQml = true;
    } else if (token == "--replay") {
      i++;
      m_runGuiTest = m_argv[i];
    } else if (token == "--script") {
      i++;
      m_runScript = m_argv[i];
    } else if (token == "--cmd") {
      i++;
      m_runTclCmd = m_argv[i];
    } else if (token == "--help") {
      m_help = true;
    } else {
      std::cout << "Unknown command line option: " << m_argv[i] << std::endl;
    }
  }
}

CommandLine::~CommandLine() {}
