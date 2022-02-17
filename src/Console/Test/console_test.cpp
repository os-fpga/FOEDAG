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

#include "ConsoleTestUtils.h"
#include "TestingUtils.h"

TCL_TEST(console_pwd) {
  FOEDAG::TclConsoleWidget *console = FOEDAG::InitConsole(clientData);
  QString res = console->getPrompt() + "pwd\n" + QDir::currentPath() + "\n" +
                console->getPrompt();
  CHECK_EXPECTED("pwd\n", res)
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
  FOEDAG::TclConsoleWidget *console = FOEDAG::InitConsole(clientData);
  CHECK_EXPECTED("source " + fullPath + "\n", expected)
  return TCL_OK;
}

TCL_TEST(console_multiline) {
  FOEDAG::TclConsoleWidget *console = FOEDAG::InitConsole(clientData);
  QString script =
      R"(proc test {} {
  debug test
} 
test
)";
  const QString pt = console->getPrompt();
  QString res =
      pt + "proc test {} {\n  debug test\n} \n" + pt + "test\ntest\n" + pt;
  CHECK_EXPECTED_FOR_FEW_COMMANDS(script, res, 2)
  return TCL_OK;
}

TCL_TEST(console_cancel) {
  FOEDAG::TclConsoleWidget *console = FOEDAG::InitConsole(clientData);
  QString command =
      R"(proc test {} {
)";
  QString result = console->getPrompt() + command + console->getPrompt() +
                   "\n" + console->getPrompt();
  command += FOEDAG::controlC;
  command += "\n";
  CHECK_EXPECTED_NOW(command, result)
  return TCL_OK;
}

TCL_TEST(console_history) {
  FOEDAG::TclConsoleWidget *console = FOEDAG::InitConsole(clientData);
  QString command = R"(history clear
<pt>proc test {} {
debug test
}
<pt>history
)";

  const QString pt = console->getPrompt();
  QString script = command;
  QString result = pt + command.replace("<pt>", pt) +
                   "1\tproc test {} {\n"
                   "\tdebug test\n"
                   "\t}\n"
                   "2\thistory\n" +
                   pt;

  CHECK_EXPECTED_FOR_FEW_COMMANDS(script.replace("<pt>", ""), result, 3)
  return TCL_OK;
}
