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
#include <QQmlContext>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Command/CommandStack.h"
#include "CommandLine.h"
#include "Main/Foedag.h"
#include "Main/ToolContext.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "NewProject/ProjectManager/config.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "Tcl/TclInterpreter.h"
#include "qttclnotifier.hpp"

#if defined(_MSC_VER)
#include <direct.h>
#define PATH_MAX _MAX_PATH
#else
#include <sys/param.h>
#include <unistd.h>
#endif

FOEDAG::Session* GlobalSession;

using namespace FOEDAG;

FOEDAG::GUI_TYPE Foedag::getGuiType(const bool& withQt, const bool& withQml) {
  if (!withQt) return FOEDAG::GUI_TYPE::GT_NONE;
  if (withQml)
    return FOEDAG::GUI_TYPE::GT_QML;
  else
    return FOEDAG::GUI_TYPE::GT_WIDGET;
}

bool getFullPath(const std::filesystem::path& path,
                 std::filesystem::path* result) {
  std::error_code ec;
  std::filesystem::path fullPath = std::filesystem::canonical(path, ec);
  bool found = (!ec && std::filesystem::is_regular_file(fullPath));
  if (result != nullptr) {
    *result = found ? fullPath : path;
  }
  return found;
}

// Try to find the full absolute path of the program currently running.
static std::filesystem::path GetProgramNameAbsolutePath(const char* progname) {
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
  const char PATH_DELIMITER = ';';
#else
  char buf[PATH_MAX];
  // If the executable is invoked with a path, we can extract it from there,
  // otherwise, we use some operating system trick to find that path:
  // In Linux, the current running binary is symbolically linked from
  // /proc/self/exe which we can resolve.
  // It won't resolve anything on other platforms, but doesnt harm either.
  for (const char* testpath : {progname, "/proc/self/exe"}) {
    const char* const program_name = realpath(testpath, buf);
    if (program_name != nullptr) return program_name;
  }
  const char PATH_DELIMITER = ':';
#endif

  // Still not found, let's go through the $PATH and see what comes up first.
  const char* const path = std::getenv("PATH");
  if (path != nullptr) {
    std::stringstream search_path(path);
    std::string path_element;
    std::filesystem::path program_path;
    while (std::getline(search_path, path_element, PATH_DELIMITER)) {
      const std::filesystem::path testpath =
          path_element / std::filesystem::path(progname);
      if (getFullPath(testpath, &program_path)) {
        return program_path;
      }
    }
  }

  return progname;  // Didn't find anything, return progname as-is.
}

Foedag::Foedag(FOEDAG::CommandLine* cmdLine, MainWindowBuilder* mainWinBuilder,
               RegisterTclFunc* registerTclFunc, Compiler* compiler,
               Settings* settings, ToolContext* context)
    : m_cmdLine(cmdLine),
      m_mainWinBuilder(mainWinBuilder),
      m_registerTclFunc(registerTclFunc),
      m_compiler(compiler),
      m_settings(settings),
      m_context(context) {
  if (context == nullptr)
    m_context = new ToolContext("Foedag", "OpenFPGA", "foedag");
  if (m_context->BinaryPath().empty()) {
    std::filesystem::path exePath =
        GetProgramNameAbsolutePath(m_cmdLine->Argv()[0]);
    std::filesystem::path exeDirPath = exePath.parent_path();
    m_context->BinaryPath(exeDirPath);
    std::filesystem::path installDir = exeDirPath.parent_path();
    const std::string separator(1, std::filesystem::path::preferred_separator);
    std::filesystem::path dataDir = installDir.string() + separator +
                                    std::string("share") + separator +
                                    m_context->ExecutableName();
    m_context->DataPath(dataDir);
  }
}

bool Foedag::initGui() {
  // Gui mode with Qt Widgets
  int argc = m_cmdLine->Argc();
  QApplication app(argc, m_cmdLine->Argv());
  FOEDAG::TclInterpreter* interpreter =
      new FOEDAG::TclInterpreter(m_cmdLine->Argv()[0]);
  const std::string logFile = m_context->ExecutableName() + ".log";
  FOEDAG::CommandStack* commands =
      new FOEDAG::CommandStack(interpreter, logFile);
  Config::Instance()->dataPath(m_context->DataPath());
  QWidget* mainWin = nullptr;

  GlobalSession = new FOEDAG::Session(nullptr, interpreter, commands, m_cmdLine,
                                      m_context, m_compiler, m_settings);
  GlobalSession->setGuiType(GUI_TYPE::GT_WIDGET);
  if (m_mainWinBuilder) {
    mainWin = m_mainWinBuilder(GlobalSession);
    GlobalSession->MainWindow(mainWin);
  }

  registerBasicGuiCommands(GlobalSession);
  if (m_registerTclFunc) {
    m_registerTclFunc(GlobalSession->MainWindow(), GlobalSession);
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
    // Moved to after the GUI and console are created

    // --cmd \"tcl cmd\"
    if (!GlobalSession->CmdLine()->TclCmd().empty()) {
      int res =
          Tcl_EvalEx(interp, GlobalSession->CmdLine()->TclCmd().c_str(), -1, 0);
      if (res != TCL_OK) {
        std::cout << std::string(Tcl_GetStringResult(interp)) << std::endl;
      }
    }
    // --replay <script> Gui replay, invoke test
    if (!GlobalSession->CmdLine()->GuiTestScript().empty()) {
      std::string proc = "call_test";
      Tcl_EvalEx(interp, proc.c_str(), -1, 0);
    } else {
      Tcl_EvalEx(interp, "gui_start", -1, 0);
    }
    return 0;
  };

  // exit tcl after last window is closed
  QObject::connect(qApp, &QApplication::lastWindowClosed, [interpreter]() {
    Tcl_EvalEx(interpreter->getInterp(), "exit", -1, 0);
  });

  // Start Loop
  Tcl_MainEx(argc, m_cmdLine->Argv(), tcl_init, interpreter->getInterp());

  delete GlobalSession;
  return 0;
}

bool Foedag::initQmlGui() {
  // Gui mode with QML
  int argc = m_cmdLine->Argc();
  QApplication app(argc, m_cmdLine->Argv());
  FOEDAG::TclInterpreter* interpreter =
      new FOEDAG::TclInterpreter(m_cmdLine->Argv()[0]);
  const std::string logFile = m_context->ExecutableName() + ".log";
  FOEDAG::CommandStack* commands =
      new FOEDAG::CommandStack(interpreter, logFile);

  MainWindowModel* windowModel = new MainWindowModel(interpreter);

  QQmlApplicationEngine engine;
  engine.addImportPath(QStringLiteral("qrc:/"));
  const QUrl url(QStringLiteral("qrc:/mainWindow.qml"));
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject* obj, const QUrl& objUrl) {
        if (!obj && url == objUrl) QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);

  engine.rootContext()->setContextProperty(QStringLiteral("windowModel"),
                                           windowModel);

  engine.load(url);

  GlobalSession = new FOEDAG::Session(nullptr, interpreter, commands, m_cmdLine,
                                      m_context, m_compiler, m_settings);
  GlobalSession->setGuiType(GUI_TYPE::GT_QML);
  GlobalSession->setWindowModel(windowModel);

  registerBasicGuiCommands(GlobalSession);
  if (m_registerTclFunc) {
    m_registerTclFunc(GlobalSession->MainWindow(), GlobalSession);
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
      int res =
          Tcl_EvalFile(interp, GlobalSession->CmdLine()->Script().c_str());
      if (res != TCL_OK) {
        std::cout << std::string(Tcl_GetStringResult(interp)) << std::endl;
      }
    }
    // --cmd \"tcl cmd\"
    if (!GlobalSession->CmdLine()->TclCmd().empty()) {
      int res =
          Tcl_EvalEx(interp, GlobalSession->CmdLine()->TclCmd().c_str(), -1, 0);
      if (res != TCL_OK) {
        std::cout << std::string(Tcl_GetStringResult(interp)) << std::endl;
      }
    }
    // --replay <script> Gui replay, invoke test
    if (!GlobalSession->CmdLine()->GuiTestScript().empty()) {
      std::string proc = "call_test";
      Tcl_EvalEx(interp, proc.c_str(), -1, 0);
    }
    return 0;
  };

  // Start Loop
  char** argv = new char*[1];
  argv[0] = strdup(m_cmdLine->Argv()[0]);
  Tcl_MainEx(argc, argv, tcl_init, interpreter->getInterp());

  delete GlobalSession;
  return 0;
}

bool Foedag::init(GUI_TYPE guiType) {
  if (m_cmdLine->PrintHelp()) {
    m_compiler->Help(&std::cout);
    return true;
  }
  if (m_cmdLine->PrintVersion()) {
    m_compiler->Version(&std::cout);
    return true;
  }
  bool result;
  switch (guiType) {
    case GUI_TYPE::GT_NONE:
      result = initBatch();
      break;
    case GUI_TYPE::GT_WIDGET:
      result = initGui();
      break;
    case GUI_TYPE::GT_QML:
      result = initQmlGui();
      break;
    default:
      break;
  }
  return result;
}

bool Foedag::initBatch() {
  // Batch mode
  FOEDAG::TclInterpreter* interpreter =
      new FOEDAG::TclInterpreter(m_cmdLine->Argv()[0]);
  const std::string logFile = m_context->ExecutableName() + ".log";
  FOEDAG::CommandStack* commands =
      new FOEDAG::CommandStack(interpreter, logFile);
  GlobalSession =
      new FOEDAG::Session(m_mainWin, interpreter, commands, m_cmdLine,
                          m_context, m_compiler, m_settings);
  GlobalSession->setGuiType(GUI_TYPE::GT_NONE);
  m_compiler->setGuiTclSync(
      new TclCommandIntegration{new ProjectManager, nullptr});

  registerBasicBatchCommands(GlobalSession);
  if (m_registerTclFunc) {
    m_registerTclFunc(nullptr, GlobalSession);
  }
  // Tcl_AppInit
  auto tcl_init = [](Tcl_Interp* interp) -> int {
    // --script <script>
    if (!GlobalSession->CmdLine()->Script().empty()) {
      int res =
          Tcl_EvalFile(interp, GlobalSession->CmdLine()->Script().c_str());
      if (res != TCL_OK) {
        GlobalSession->ReturnStatus(res);
        Tcl_EvalEx(interp, "puts $errorInfo", -1, 0);
      }
      exit(GlobalSession->ReturnStatus());
    }
    // --cmd \"tcl cmd\"
    if (!GlobalSession->CmdLine()->TclCmd().empty()) {
      int res =
          Tcl_EvalEx(interp, GlobalSession->CmdLine()->TclCmd().c_str(), -1, 0);
      if (res != TCL_OK) {
        GlobalSession->ReturnStatus(res);
        Tcl_EvalEx(interp, "puts $errorInfo", -1, 0);
      }
      exit(GlobalSession->ReturnStatus());
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
  char** argv = new char*[1];
  argv[0] = strdup(m_cmdLine->Argv()[0]);
  Tcl_MainEx(1, argv, tcl_init, interpreter->getInterp());
  int returnStatus = GlobalSession->ReturnStatus();
  delete GlobalSession;
  return returnStatus;
}
