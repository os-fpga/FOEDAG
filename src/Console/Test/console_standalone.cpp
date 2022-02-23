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
#include <thread>

#include "Compiler/Compiler.h"
#include "Compiler/TclInterpreterHandler.h"
#include "DummyParser.h"
#include "Main/CompilerNotifier.h"
#include "Main/Foedag.h"
#include "Main/qttclnotifier.hpp"
#include "StreamBuffer.h"
#include "Tcl/TclInterpreter.h"
#include "TclConsole.h"
#include "TclConsoleBuilder.h"
#include "TclConsoleWidget.h"
FOEDAG::Session *GlobalSession;

QWidget *mainWindowBuilder(FOEDAG::CommandLine *cmd,
                           FOEDAG::TclInterpreter *interpreter) {
  auto buffer = new FOEDAG::StreamBuffer;
  auto tclConsole = std::make_unique<FOEDAG::TclConsole>(
      interpreter->getInterp(), buffer->getStream());
  FOEDAG::TclConsole *c = tclConsole.get();
  FOEDAG::TclConsoleWidget *console = nullptr;
  QWidget *w =
      FOEDAG::createConsole(interpreter->getInterp(), std::move(tclConsole),
                            buffer, nullptr, &console);

  if (console) console->setParsers({new FOEDAG::DummyParser});

  std::string design("Some cool design");
  FOEDAG::Compiler *com = new FOEDAG::Compiler{
      interpreter, new FOEDAG::Design(design), buffer->getStream(),
      new FOEDAG::CompilerNotifier{c}};
  com->RegisterCommands(interpreter, false);
  return w;
}

int main(int argc, char **argv) {
  FOEDAG::CommandLine *cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  FOEDAG::GUI_TYPE guiType = FOEDAG::GUI_TYPE::GT_WIDGET;

  FOEDAG::Foedag *foedag = new FOEDAG::Foedag(
      cmd, mainWindowBuilder, nullptr /*registerExampleCommands*/);

  return foedag->init(guiType);
}
