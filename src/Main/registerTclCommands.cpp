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
#include <QLabel>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Command/CommandStack.h"
#include "CommandLine.h"
#include "Foedag.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "Tcl/TclInterpreter.h"
#include "TextEditor/text_editor.h"
#include "qttclnotifier.hpp"

void registerBasicGuiCommands(FOEDAG::Session* session) {
  auto gui_start = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    GlobalSession->CmdStack()->CmdLogger()->log("gui_start");
    GlobalSession->windowShow();
    return 0;
  };
  session->TclInterp()->registerCmd("gui_start", gui_start, 0, 0);

  auto gui_stop = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    GlobalSession->CmdStack()->CmdLogger()->log("gui_stop");
    GlobalSession->windowHide();
    return 0;
  };
  session->TclInterp()->registerCmd("gui_stop", gui_stop, 0, 0);

  auto create_project = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Q_UNUSED(interp);
    GlobalSession->CmdStack()->CmdLogger()->log("create_project");
    FOEDAG::MainWindow* mainwindow = (FOEDAG::MainWindow*)(clientData);
    mainwindow->Tcl_NewProject(argc, argv);
    return 0;
  };
  session->TclInterp()->registerCmd("create_project", create_project,
                                    GlobalSession->MainWindow(), 0);

  auto tcl_exit = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    delete GlobalSession;
    // Do not log this command
    Tcl_Exit(0);  // Cannot use Tcl_Finalize that issues signals probably due to
                  // the Tcl/QT loop
    return 0;
  };
  session->TclInterp()->registerCmd("tcl_exit", tcl_exit, 0, 0);

  auto help = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    GlobalSession->CmdStack()->CmdLogger()->log("help");
    GlobalSession->CmdLine()->printHelp();
    return 0;
  };
  session->TclInterp()->registerCmd("help", help, 0, 0);

  auto process_qt_events = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
    QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    return 0;
  };
  session->TclInterp()->registerCmd("process_qt_events", process_qt_events, 0,
                                    0);
}

void registerBasicBatchCommands(FOEDAG::Session* session) {
  auto tcl_exit = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    delete GlobalSession;
    // Do not log this command
    Tcl_Exit(0);  // Cannot use Tcl_Finalize that issues signals probably due to
                  // the Tcl/QT loop
    return 0;
  };
  session->TclInterp()->registerCmd("tcl_exit", tcl_exit, 0, 0);

  auto help = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    GlobalSession->CmdStack()->CmdLogger()->log("help");
    GlobalSession->CmdLine()->printHelp();
    return 0;
  };
  session->TclInterp()->registerCmd("help", help, 0, 0);
}

void registerTextEditorCommands(FOEDAG::TextEditor* editor,
                                FOEDAG::Session* session) {
  auto openfile = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    Tcl_ResetResult(interp);
    if (argc != 2) {
      const QString usageMsg = QString("Usage: %1 ?filename?").arg(argv[0]);
      Tcl_AppendResult(interp, qPrintable(usageMsg), (char*)nullptr);
      return TCL_ERROR;
    }
    auto editor = static_cast<FOEDAG::TextEditor*>(clientData);
    const QString file{argv[1]};
    const QFileInfo info{file};
    if (!info.exists()) {
      const QString msg = QString("File \"%1\" doesn't exists").arg(file);
      Tcl_AppendResult(interp, qPrintable(msg), (char*)nullptr);
      return TCL_ERROR;
    }
    editor->SlotOpenFile(file);
    return TCL_OK;
  };
  session->TclInterp()->registerCmd(
      "openfile", openfile, static_cast<void*>(editor), nullptr /*deleteProc*/);
}
