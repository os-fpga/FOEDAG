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

#include "Compiler/TaskManager.h"
#include "Main/TclSimpleParser.h"
#include "TopLevelInterface.h"

using namespace FOEDAG;

Session::~Session() {
  if (m_mainWindow) m_mainWindow->deleteLater();
  delete m_interp;
  delete m_stack;
  delete m_cmdLine;
}

void Session::windowShow() {
  switch (m_guiType) {
    case GUI_TYPE::GT_WIDGET: {
      m_mainWindow->show();
      const auto cmdLine = CmdLine();
      auto hasScriptCmd = !cmdLine->Script().empty();
      auto hasProjectFile = !cmdLine->ProjectFile().empty();
      auto topLevel = dynamic_cast<FOEDAG::TopLevelInterface *>(m_mainWindow);
      if (topLevel) {
        topLevel->gui_start(!hasScriptCmd && !hasProjectFile &&
                            cmdLine->GuiTestScript().empty());
      }
      if (hasProjectFile) {
        topLevel->openProject(QString::fromStdString(cmdLine->ProjectFile()),
                              false, false);
      } else if (hasScriptCmd) {
        int returnCode{TCL_OK};
        if (m_compiler) {
          TclSimpleParser tclParser;
          std::vector<uint> ids;
          tclParser.parse(cmdLine->Script(), ids);
          int counter{static_cast<int>(ids.size())};
          m_compiler->GetTaskManager()->setTaskCount(counter);
          const auto tableTasks =
              m_compiler->GetTaskManager()->tableTasks(true);
          for (auto task : tableTasks) {
            auto taskId = m_compiler->GetTaskManager()->taskId(task);
            task->setEnable(
                std::find(ids.begin(), ids.end(), taskId) != ids.end(),
                task->isEnableDefault());
          }
          if (topLevel) {
            topLevel->ProgressVisible(true);
          }
          m_compiler->start();
        }
        auto result = TclInterp()->evalFile(cmdLine->Script(), &returnCode);
        if (m_compiler) {
          if (returnCode == TCL_OK)
            m_compiler->Message(result);
          else
            m_compiler->ErrorMessage(result);
          m_compiler->finish();
          if (topLevel) {
            topLevel->ScriptFinished();
          }
        }
      }
      break;
    }
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

void Session::ProjectFileLoader(
    std::shared_ptr<FOEDAG::ProjectFileLoader> projectFileLoader) {
  m_projectFileLoader = projectFileLoader;
}

std::shared_ptr<ProjectFileLoader> Session::ProjectFileLoader() const {
  return m_projectFileLoader;
}
