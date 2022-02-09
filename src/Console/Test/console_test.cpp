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
#include <QDir>

#include "TestingUtils.h"

TCL_TEST(console_pwd) {
  FOEDAG::TclConsoleWidget *console = FOEDAG::InitConsole(interp);
  QString res = console->getPrompt() + "pwd\n" + QDir::currentPath() + "\n" +
                console->getPrompt();
  CHECK_EXPECTED("pwd", res)
  return TCL_OK;
}

TCL_TEST(console_proc) {
  if (argc < 3) {
    Tcl_Eval(interp, "error \"ERROR: Invalid arg number for console_proc\"");
    return TCL_ERROR;
  }
  QFile file(argv[1]);
  QString expected(argv[2]);
  if (!file.exists()) {
    Tcl_Eval(interp,
             std::string(std::string("error \"ERROR: File does not exit: \"") +
                         argv[2])
                 .c_str());
    return TCL_ERROR;
  }
  QString fullPath = file.fileName();
  FOEDAG::TclConsoleWidget *console = FOEDAG::InitConsole(interp);
  CHECK_EXPECTED("source " + fullPath, expected)
  return TCL_OK;
}

TCL_TEST(console_multiline) {
  FOEDAG::TclConsoleWidget *console = FOEDAG::InitConsole(interp);
  QString script =
      R"("
proc test {} {
  puts test
} 
test
")";
  QString res = console->getPrompt() + script + console->getPrompt();
  CHECK_EXPECTED(script, res)
  return TCL_OK;
}
