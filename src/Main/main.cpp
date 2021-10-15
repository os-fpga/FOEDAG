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

#if defined(_MSC_VER)
#include <direct.h>
#include <process.h>
#else
#include <sys/param.h>
#include <unistd.h>
#endif

#include <string.h>
#include <sys/stat.h>

#include <QApplication>
#include <QLabel>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Tcl/TclInterpreter.h"

int main(int argc, char** argv) {
  TclInterpreter interpreter(argv[0]);
  std::string result =
      interpreter.evalCmd("puts \"Hello Foedag, you have Tcl!\"");
  std::cout << result << '\n';
  if (argc >= 2) {
    if (std::string(argv[1]) == "-noqt") {
      return 0;
    }
  }
  QApplication app(argc, argv);
  QLabel* label = new QLabel("Hello Qt!");
  label->show();
  return app.exec();
}
