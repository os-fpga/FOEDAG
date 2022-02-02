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

#if defined(_MSC_VER)
#include <direct.h>
#include <process.h>
#else
#include <sys/param.h>
#include <unistd.h>
#endif

#include <string.h>
#include <sys/stat.h>
extern "C" {
#include <tcl.h>
}

#include <QApplication>
#include <QGuiApplication>
#include <QLabel>
#include <QQmlApplicationEngine>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Command/CommandStack.h"
#include "CommandLine.h"
#include "Console/StreamBuffer.h"
#include "Console/TclConsole.h"
#include "Console/TclConsoleWidget.h"
#include "DesignRuns/runs_form.h"
#include "Foedag.h"
#include "Main/Foedag.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "NewFile/new_file.h"
#include "NewProject/new_project_dialog.h"
#include "ProjNavigator/sources_form.h"
#include "Tcl/TclInterpreter.h"
#include "TextEditor/text_editor.h"
#include "qttclnotifier.hpp"

using namespace FOEDAG;

bool Foedag::init(bool initWithGui) {
  if (initWithGui)
    return initGui();
  else
    return initBatch();
}

bool Foedag::initGui() {
  // Gui mode
  int argc = m_cmdLine->Argc();
  QApplication app(argc, m_cmdLine->Argv());
  FOEDAG::TclInterpreter* interpreter =
      new FOEDAG::TclInterpreter(m_cmdLine->Argv()[0]);
  FOEDAG::CommandStack* commands = new FOEDAG::CommandStack(interpreter);
  QQmlApplicationEngine engine;

  switch (m_guiType) {
    case GUI_TYPE::GT_NONE:
      break;

    case GUI_TYPE::GT_MAIN_WINDOW: {
      engine.addImportPath(QStringLiteral("qrc:/"));
      const QUrl url(QStringLiteral("qrc:/main_window.qml"));
      QObject::connect(
          &engine, &QQmlApplicationEngine::objectCreated, &app,
          [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) QCoreApplication::exit(-1);
          },
          Qt::QueuedConnection);
      engine.load(url);
      break;
    }
    case GUI_TYPE::GT_NEW_FILE:       // fall-through
    case GUI_TYPE::GT_DESIGN_RUNS:    // fall-through
    case GUI_TYPE::GT_TEXT_EDITOR:    // fall-through
    case GUI_TYPE::GT_PRO_NAVIGATOR:  // fall-through
    case GUI_TYPE::GT_NEW_PROJECT:    // fall-through
    case GUI_TYPE::GT_TCL_CONSOLE: {
      break;
    }
  }

  QWidget* mainWin = nullptr;
  switch (m_guiType) {
    case GUI_TYPE::GT_NONE:
      break;
    case GUI_TYPE::GT_MAIN_WINDOW:
      mainWin = new FOEDAG::MainWindow(interpreter);
      break;
    case GUI_TYPE::GT_NEW_FILE:
      mainWin = new FOEDAG::NewFile();
      break;
    case GUI_TYPE::GT_DESIGN_RUNS: {
      if (argc > 2)
        mainWin = new FOEDAG::RunsForm(m_cmdLine->Argv()[2]);
      else
        mainWin = new FOEDAG::RunsForm("/testproject");
      break;
    }
    case GUI_TYPE::GT_TEXT_EDITOR:
      mainWin = new FOEDAG::TextEditor();
      break;
    case GUI_TYPE::GT_PRO_NAVIGATOR: {
      if (argc > 2)
        mainWin = new FOEDAG::SourcesForm(m_cmdLine->Argv()[2]);
      else
        mainWin = new FOEDAG::SourcesForm("/testproject");
      break;
    }
    case GUI_TYPE::GT_NEW_PROJECT:
      mainWin = new FOEDAG::newProjectDialog();
      break;
    case GUI_TYPE::GT_TCL_CONSOLE:
      mainWin = new TclConsoleWidget{
          interpreter->getInterp(),
          std::make_unique<TclConsole>(interpreter, std::cout),
          new StreamBuffer};
      break;
  }

  if (mainWin)
    GlobalSession =
        new FOEDAG::Session(mainWin, interpreter, commands, m_cmdLine);
  GlobalSession->setGuiType(m_guiType);
  registerBasicGuiCommands(GlobalSession);
  if (m_registerTclFunc) {
    m_registerTclFunc(GlobalSession);
  }

  QtTclNotify::QtTclNotifier::setup();  // Registers notifier with Tcl

  // Tell Tcl to run Qt as the main event loop once the interpreter is
  // initialized
  Tcl_SetMainLoop([]() { QApplication::exec(); });

  // --replay <script> Gui replay, register test
  if (!GlobalSession->CmdLine()->GuiTestScript().empty()) {
    interpreter->evalGuiTestFile(GlobalSession->CmdLine()->GuiTestScript());
  }

  // Tcl_AppInit
  auto tcl_init = [](Tcl_Interp* interp) -> int {
    // --script <script>
    if (!GlobalSession->CmdLine()->Script().empty()) {
      Tcl_EvalFile(interp, GlobalSession->CmdLine()->Script().c_str());
    }
    // --cmd \"tcl cmd\"
    if (!GlobalSession->CmdLine()->TclCmd().empty()) {
      Tcl_EvalEx(interp, GlobalSession->CmdLine()->TclCmd().c_str(), -1, 0);
    }
    // --replay <script> Gui replay, invoke test
    if (!GlobalSession->CmdLine()->GuiTestScript().empty()) {
      std::string proc = "call_test";
      Tcl_EvalEx(interp, proc.c_str(), -1, 0);
    }
    return 0;
  };

  // Start Loop
  Tcl_MainEx(argc, m_cmdLine->Argv(), tcl_init, interpreter->getInterp());

  delete GlobalSession;
  return 0;
}

bool Foedag::initBatch() {
  // Batch mode
  FOEDAG::TclInterpreter* interpreter =
      new FOEDAG::TclInterpreter(m_cmdLine->Argv()[0]);
  FOEDAG::CommandStack* commands = new FOEDAG::CommandStack(interpreter);
  GlobalSession =
      new FOEDAG::Session(m_mainWin, interpreter, commands, m_cmdLine);
  registerBasicBatchCommands(GlobalSession);
  if (m_registerTclFunc) {
    m_registerTclFunc(GlobalSession);
  }
  std::string result = interpreter->evalCmd("puts \"Tcl only mode\"");
  // --script <script>
  if (!m_cmdLine->Script().empty())
    result = interpreter->evalFile(m_cmdLine->Script());
  if (result != "") {
    std::cout << result << '\n';
  }

  // Tcl_AppInit
  auto tcl_init = [](Tcl_Interp* interp) -> int {
    // --script <script>
    if (!GlobalSession->CmdLine()->Script().empty()) {
      Tcl_EvalFile(interp, GlobalSession->CmdLine()->Script().c_str());
    }
    // --cmd \"tcl cmd\"
    if (!GlobalSession->CmdLine()->TclCmd().empty()) {
      Tcl_EvalEx(interp, GlobalSession->CmdLine()->TclCmd().c_str(), -1, 0);
    }
    // --replay <script> Gui replay, invoke test
    if (!GlobalSession->CmdLine()->GuiTestScript().empty()) {
      std::string proc = "call_test";
      Tcl_EvalEx(interp, proc.c_str(), -1, 0);
    }
    return 0;
  };

  // Start Loop
  int argc = m_cmdLine->Argc();
  Tcl_MainEx(argc, m_cmdLine->Argv(), tcl_init, interpreter->getInterp());

  delete GlobalSession;
  return 0;
}
