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
#include "Compiler/Design.h"
#include "ConsoleWidget.h"
#include "Tcl/TclInterpreter.h"
#include "TclConsole.h"
#include "TclController.h"

void worker(int argc, char **argv, Tcl_Interp *interp) {
  auto init = [](Tcl_Interp *interp) -> int {
    // init here
    return 0;
  };
  Tcl_MainEx(argc, argv, init, interp);
}

int main(int argc, char **argv) {
  QApplication a{argc, argv};
  FOEDAG::TclInterpreter *interpreter = new FOEDAG::TclInterpreter{argv[0]};
  std::string design("Some cool design");
  FOEDAG::Compiler *com =
      new FOEDAG::Compiler{interpreter, new FOEDAG::Design(design), std::cout};
  com->RegisterCommands(interpreter, false);
  ConsoleWidget *console = new ConsoleWidget{std::make_unique<TclConsole>()};
  TclController *tcl = new TclController{interpreter};

  QObject::connect(console, &ConsoleWidget::sendCommand, tcl,
                   &TclController::runCommand);
  QObject::connect(tcl, &TclController::sendOutput, console,
                   &ConsoleWidget::append);
  QObject::connect(console, &ConsoleWidget::abort, tcl, &TclController::abort);
  console->show();
  std::thread work{&worker, argc, argv, interpreter->getInterp()};
  work.detach();
  return a.exec();
}
