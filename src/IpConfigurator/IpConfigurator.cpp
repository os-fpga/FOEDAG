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
#include "IpConfigurator/IpConfigurator.h"

#include "IpConfigurator/IpTreesWidget.h"
#include "MainWindow/Session.h"
using namespace FOEDAG;

void IpConfigurator::RegisterCommands(FOEDAG::Session* session) {
  registerIpConfiguratorCommands(this, session);
}

void FOEDAG::registerIpConfiguratorCommands(QWidget* ipconfigurator,
                                            FOEDAG::Session* session) {
  auto show = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::IpConfigurator* configurator =
        (FOEDAG::IpConfigurator*)(clientData);
    configurator->ShowIpTrees();
    return 0;
  };
  session->TclInterp()->registerCmd("ipconfigurator_show", show, ipconfigurator,
                                    0);

  auto hide = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::IpConfigurator* configurator =
        (FOEDAG::IpConfigurator*)(clientData);
    configurator->CloseIpTrees();
    return 0;
  };
  session->TclInterp()->registerCmd("ipconfigurator_close", hide,
                                    ipconfigurator, 0);
}

IpConfigurator::IpConfigurator(QWidget* parent) : QWidget(parent) {
  IpTreesWidget::Instance()->Init();
}

void IpConfigurator::ShowIpTrees() { IpTreesWidget::Instance()->show(); }

void IpConfigurator::CloseIpTrees() { IpTreesWidget::Instance()->hide(); }

QWidget* IpConfigurator::GetIpTreesWidget() {
  return IpTreesWidget::Instance();
}