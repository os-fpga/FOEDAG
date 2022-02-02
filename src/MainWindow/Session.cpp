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

#include "Main/Foedag.h"

using namespace FOEDAG;

Session::~Session() {
  m_mainWindow->deleteLater();
  delete m_interp;
  delete m_stack;
  delete m_cmdLine;
}

void Session::mainWindowShow() {
  // if (m_guiType != FOEDAG::GUI_TYPE::GT_MAIN_WINDOW)
  m_mainWindow->show();
  // else {  // TODO: qml model show()
  //}
}

void Session::mainWindowHide() {
  // if (m_guiType != FOEDAG::GUI_TYPE::GT_MAIN_WINDOW)
  m_mainWindow->hide();
  // else {  // TODO: qml model hide()
  //}
}

void Session::setGuiType(GUI_TYPE newGuiType) { m_guiType = newGuiType; }
