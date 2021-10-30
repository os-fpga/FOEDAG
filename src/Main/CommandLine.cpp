/*
Copyright 2021 The Foedag team

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
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
  std::cout << "   --test_gui <script>: Replay GUI test" << std::endl;
  std::cout << "   --script   <script>: Execute a Tcl script" << std::endl;
  std::cout << "Tcl commands:" << std::endl;
  std::cout << "   gui_start" << std::endl;
  std::cout << "   gui_stop" << std::endl;
  std::cout << "   tcl_exit" << std::endl;
  std::cout << "-------------------------" << std::endl;
}

CommandLine::CommandLine(int argc, char** argv) {
  for (int i = 0; i < argc; i++) {
    std::string token(argv[i]);
    if (token == "--noqt") {
      m_withQt = false;
    } else if (token == "--gui_test") {
      i++;
      m_runGuiTest = argv[i];
    } else if (token == "--script") {
      i++;
      m_runScript = argv[i];
    } else if (token == "--help") {
      printHelp();
    }
  }
}

CommandLine::~CommandLine() {}
