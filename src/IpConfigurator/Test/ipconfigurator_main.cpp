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
#include <QHBoxLayout>

#include "IpConfigurator/IpConfigurator.h"
#include "IpConfiguratorCreator.h"
#include "Main/Foedag.h"
#include "Main/qttclnotifier.hpp"
#include "Tcl/TclInterpreter.h"

QWidget* ipconfiguratorBuilder(FOEDAG::Session* session) {
  FOEDAG::IpConfiguratorCreator* creator = new FOEDAG::IpConfiguratorCreator;
  QWidget* w = new QWidget;
  w->setLayout(new QHBoxLayout);
  w->layout()->addWidget(creator->GetAvailableIpsWidget());
  return w;
}

int main(int argc, char** argv) {
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();

  FOEDAG::Compiler* compiler = new FOEDAG::Compiler();
  FOEDAG::Settings* settings = new FOEDAG::Settings();

  FOEDAG::GUI_TYPE guiType =
      FOEDAG::Foedag::getGuiType(cmd->WithQt(), cmd->WithQml());

  FOEDAG::Foedag* foedag = new FOEDAG::Foedag(
      cmd, ipconfiguratorBuilder, FOEDAG::registerIpConfiguratorCommands,
      compiler, settings);

  return foedag->init(guiType);
}
