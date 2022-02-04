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

#include <QApplication>
#include <QDir>
#include <thread>

#include "ConsoleTestUtils.h"
#include "Main/Foedag.h"

FOEDAG::Session* GlobalSession;

void registerExampleCommands(FOEDAG::Session* session) {
  auto console_pwd = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    Q_UNUSED(clientData)
    Q_UNUSED(argv)
    Q_UNUSED(argc)
    FOEDAG::TclConsoleWidget* console = FOEDAG::Init(interp);
    QString res = console->getPrompt() + "pwd\n" + QDir::currentPath() + "\n" +
                  console->getPrompt();
    CHECK_EXPECTED("pwd", res)
    return TCL_OK;
  };
  session->TclInterp()->registerCmd("console_pwd", console_pwd,
                                    GlobalSession->TclInterp(), nullptr);

  auto console_proc = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Q_UNUSED(clientData)
    if (argc < 2) return TCL_ERROR;
    QDir path{argv[1]};
    QString fileName{"gui_console_proc.tcl"};
    if (!path.exists(fileName)) return TCL_ERROR;
    QString fullPath = path.filePath(fileName);
    FOEDAG::TclConsoleWidget* console = FOEDAG::Init(interp);
    QString res = console->getPrompt() + "source " + fullPath +
                  "\nHello world\n" + console->getPrompt();
    CHECK_EXPECTED("source " + fullPath, res)
    return TCL_OK;
  };
  session->TclInterp()->registerCmd("console_proc", console_proc,
                                    GlobalSession->TclInterp(), nullptr);

  auto console_multiline = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    Q_UNUSED(clientData)
    Q_UNUSED(argv)
    Q_UNUSED(argc)
    FOEDAG::TclConsoleWidget* console = FOEDAG::Init(interp);
    QString res = console->getPrompt() + "proc test {} {\nputs test\n}\n" +
                  "test\n" + console->getPrompt();
    CHECK_EXPECTED("proc test {} {\nputs test\n}\ntest", res)
    return TCL_OK;
  };
  session->TclInterp()->registerCmd("console_multiline", console_multiline,
                                    GlobalSession->TclInterp(), nullptr);
}

int main(int argc, char** argv) {
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  FOEDAG::Foedag* foedag = new FOEDAG::Foedag(
      cmd, FOEDAG::GUI_TYPE::GT_TCL_CONSOLE, registerExampleCommands);
  return foedag->init(cmd->WithQt());
}
