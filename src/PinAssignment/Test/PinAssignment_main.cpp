/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

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
#include <QDebug>
#include <QHBoxLayout>

#include "Main/Foedag.h"
#include "PinAssignmentCreator.h"
#include "Tcl/TclInterpreter.h"
#include "tclutils/TclUtils.h"

QWidget* PinAssignmentBuilder(FOEDAG::Session* session) {
  FOEDAG::PinAssignmentCreator* creator = new FOEDAG::PinAssignmentCreator;
  QWidget* w = new QWidget;
  w->setLayout(new QHBoxLayout);
  w->layout()->addWidget(creator->GetPackagePinsWidget());
  w->layout()->addWidget(creator->GetPortsWidget());
  return w;
}

void registerPinAssignmentCommands(QWidget* widget, FOEDAG::Session* session) {
  FOEDAG::utils::Command::registerAllcommands(session->TclInterp()->getInterp(),
                                              widget);
}

int main(int argc, char** argv) {
  FOEDAG::utils::initCommandRegister();
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  FOEDAG::Compiler* compiler = new FOEDAG::Compiler();
  FOEDAG::Settings* settings = new FOEDAG::Settings();

  FOEDAG::GUI_TYPE guiType =
      FOEDAG::Foedag::getGuiType(cmd->WithQt(), cmd->WithQml());

  FOEDAG::Foedag* foedag =
      new FOEDAG::Foedag(cmd, PinAssignmentBuilder,
                         registerPinAssignmentCommands, compiler, settings);

  return foedag->init(guiType);
}
