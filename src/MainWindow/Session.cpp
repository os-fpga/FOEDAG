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

#include "MainWindow/Session.h"

#include "TopLevelInterface.h"

using namespace FOEDAG;

Session::~Session() {
  m_mainWindow->deleteLater();
  delete m_interp;
  delete m_stack;
  delete m_cmdLine;
}

void Session::windowShow() {
  switch (m_guiType) {
    case GUI_TYPE::GT_WIDGET:
      m_mainWindow->show();
      if (auto topLevel = dynamic_cast<TopLevelInterface *>(m_mainWindow)) {
        topLevel->gui_start();
      }
      if (!CmdLine()->Script().empty()) {
        int returnCode{TCL_OK};
        if (m_compiler) m_compiler->start();
        auto result = TclInterp()->evalFile(CmdLine()->Script(), &returnCode);
        if (m_compiler) {
          if (returnCode == TCL_OK) m_compiler->Message(result);
          m_compiler->finish();
        }
      }
      break;
    case GUI_TYPE::GT_QML:
      m_windowModel->setIsVisible(true);
      break;
    case GUI_TYPE::GT_NONE:
      break;
  }
}

void Session::windowHide() {
  switch (m_guiType) {
    case GUI_TYPE::GT_WIDGET:
      m_mainWindow->hide();
      break;
    case GUI_TYPE::GT_QML:
      m_windowModel->setIsVisible(false);
      break;
    case GUI_TYPE::GT_NONE:
      break;
  }
}

void Session::setGuiType(FOEDAG::GUI_TYPE newGuiType) {
  m_guiType = newGuiType;
}

void Session::setWindowModel(MainWindowModel *newWindowModel) {
  m_windowModel = newWindowModel;
}
