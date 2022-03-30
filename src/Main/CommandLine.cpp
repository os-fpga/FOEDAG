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

void CommandLine::printHelp() {
  std::cout << "-------------------------" << std::endl;
  std::cout << "-----  FOEDAG HELP  -----" << std::endl;
  std::cout << "-------------------------" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "   --help:  This help" << std::endl;
  std::cout << "   --noqt:  Tcl only, no GUI" << std::endl;
  std::cout << "   --batch: Tcl only, no GUI" << std::endl;
  std::cout << "   --replay <script>: Replay GUI test" << std::endl;
  std::cout << "   --script <script>: Execute a Tcl script" << std::endl;
  std::cout << "   --compiler <name>: Compiler name {openfpga...}, default is a dummy compiler" << std::endl;
  std::cout << "Tcl commands:" << std::endl;
  std::cout << "   help" << std::endl;
  std::cout << "   gui_start" << std::endl;
  std::cout << "   gui_stop" << std::endl;
  std::cout << "   create_design <name>" << std::endl;
  std::cout << "   add_design_file <file> <type> (VHDL_1987, VHDL_1993, VHDL_2008, V_1995, "
                       "V_2001, SV_2005, SV_2009, SV_2012, SV_2017) " << std::endl;
  std::cout << "   set_top_module <top>" << std::endl;
  std::cout << "   ipgenerate" << std::endl;
  std::cout << "   synthesize" << std::endl;
  std::cout << "   global_placement" << std::endl;
  std::cout << "   placement" << std::endl;
  std::cout << "   routing" << std::endl;
  std::cout << "   sta" << std::endl;
  std::cout << "   power" << std::endl;
  std::cout << "   bitstream" << std::endl;
  std::cout << "   tcl_exit" << std::endl;
  std::cout << "-------------------------" << std::endl;
}

void CommandLine::processArgs() {
  for (int i = 1; i < m_argc; i++) {
    std::string token(m_argv[i]);
    if (token == "--noqt") {
      m_withQt = false;
    } else if (token == "--batch") {
      m_withQt = false;
    } else if (token == "--compiler") {
      i++;
      m_compilerName =  m_argv[i];
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
      printHelp();
      exit(0);
    } else {
      std::cout << "Unknown command line option: " << m_argv[i] << std::endl;
    }
  }
}

CommandLine::~CommandLine() {}
