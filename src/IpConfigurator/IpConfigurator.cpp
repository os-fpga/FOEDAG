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

#include <QDockWidget>

#include "IpConfigurator/IpConfigWidget.h"
#include "IpConfigurator/IpTreesWidget.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"

extern FOEDAG::Session* GlobalSession;

using namespace FOEDAG;

IpConfigurator::IpConfigurator(QWidget* parent) : QWidget(parent) {
  IpTreesWidget::Instance()->Init();
}

void IpConfigurator::ShowIpTrees() {
  QWidget* widget = IpTreesWidget::Instance();
  if (widget) {
    ((IpTreesWidget*)widget)->refresh();

    // Check if this widget is contained by a parent QDockWidget
    QObject* parent = widget->parent();
    while (parent) {
      QDockWidget* dw = qobject_cast<QDockWidget*>(parent);
      if (dw) {
        // Update pointer so we show the QDockWidget instead of the child widget
        widget = (QWidget*)dw;
      }
      parent = parent->parent();
    }

    widget->show();
    widget->raise();
  }
}

void IpConfigurator::HideIpTrees() {
  QWidget* widget = IpTreesWidget::Instance();
  if (widget) {
    // Check if this widget is contained by a parent QDockWidget
    QObject* parent = widget->parent();
    while (parent) {
      QDockWidget* dw = qobject_cast<QDockWidget*>(parent);
      if (dw) {
        // Update pointer so we hide the QDockWidget instead of the child widget
        widget = (QWidget*)dw;
      }
      parent = parent->parent();
    }

    widget->hide();
  }
}

void IpConfigurator::ShowConfigDlg() {
  IpConfigWidget widget;
  widget.show();
}

QWidget* IpConfigurator::GetIpTreesWidget() {
  return IpTreesWidget::Instance();
}

void IpConfigurator::ReloadIps() {
  FOEDAG::Compiler* compiler = nullptr;
  ProjectManager* projManager = nullptr;
  if (GlobalSession && (compiler = GlobalSession->GetCompiler()) &&
      (projManager = compiler->ProjManager())) {
    // Get Ip Configuration/Generate cmds from project file
    QString projPath = projManager->getProjectPath();
    auto cmds = projManager->ipInstanceCmdList();

    // If the project path is valid and there are IP cmds
    if (!projPath.isEmpty() && cmds.size() > 0) {
      // Execute each configure/generate command
      for (const auto& cmd : cmds) {
        GlobalSession->CmdStack()->push_and_exec(new Command(cmd));
      }
    }
  }
}

void FOEDAG::registerIpConfiguratorCommands(QWidget* widget,
                                            FOEDAG::Session* session) {
  auto show = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    QWidget* w = static_cast<QWidget*>(clientData);
    w->show();
    return TCL_OK;
  };
  session->TclInterp()->registerCmd("ipconfigurator_show", show, widget, 0);

  auto hide = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    QWidget* w = static_cast<QWidget*>(clientData);
    w->hide();
    return TCL_OK;
  };
  session->TclInterp()->registerCmd("ipconfigurator_hide", hide, widget, 0);

  auto show_dlg = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    QWidget* w = static_cast<QWidget*>(clientData);
    IpConfigWidget* widget = new IpConfigWidget(w);
    widget->show();
    return 0;
  };
  session->TclInterp()->registerCmd("ipconfigurator_show_dlg", show_dlg, widget,
                                    0);
}
