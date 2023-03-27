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

#include "ConsoleTestUtils.h"
#include "Main/Foedag.h"
#include "tclutils/TclUtils.h"

QWidget* mainWindowBuilder(FOEDAG::Session* session) {
  FOEDAG::TclConsoleWidget* console{nullptr};
  auto buffer = new FOEDAG::TclConsoleBuffer;
  auto w = FOEDAG::createConsole(
      session->TclInterp()->getInterp(),
      std::make_unique<FOEDAG::TclConsole>(session->TclInterp()->getInterp(),
                                           buffer->getStream()),
      buffer, nullptr, &console);
  return w;
}

void registerExampleCommands(QWidget* widget, FOEDAG::Session* session) {
  FOEDAG::utils::Command::registerAllcommands(
      GlobalSession->TclInterp()->getInterp(), GlobalSession->MainWindow());
}

int main(int argc, char** argv) {
  FOEDAG::utils::initCommandRegister();
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  FOEDAG::Compiler* compiler = new FOEDAG::Compiler();

  FOEDAG::GUI_TYPE guiType =
      FOEDAG::Foedag::getGuiType(cmd->WithQt(), cmd->WithQml());

  FOEDAG::Settings* settings = new FOEDAG::Settings();

  FOEDAG::Foedag* foedag = new FOEDAG::Foedag(
      cmd, mainWindowBuilder, registerExampleCommands, compiler, settings);

  return foedag->init(guiType);
}
