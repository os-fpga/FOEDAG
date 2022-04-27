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
#include "Main/Settings.h"
#include "Main/Tasks.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "NewProject/Main/registerNewProjectCommands.h"
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

  auto qt_getWidget = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    if (argc < 2) return TCL_ERROR;

    const QString widgetName{argv[1]};
    QWidget* w = static_cast<QWidget*>(clientData);
    QWidget* topWidget = QApplication::topLevelAt(w->mapToGlobal(QPoint()));
    if (!topWidget) topWidget = w;
    if (!topWidget) {
      Tcl_AppendResult(interp, qPrintable("topWidget == nullptr"), (char*)NULL);
      return TCL_ERROR;
    }

    QWidget* widget = topWidget->findChild<QWidget*>(widgetName);
    if (!widget) {
      Tcl_AppendResult(interp, qPrintable("No such widget"), (char*)NULL);
      return TCL_ERROR;
    }
    QString result =
        QString("QWidget(0x%1)")
            .arg(QString::number(reinterpret_cast<ulong>(widget), 16));
    Tcl_AppendResult(interp, qPrintable(result), (char*)NULL);

    return TCL_OK;
  };
  session->TclInterp()->registerCmd("qt_getWidget", qt_getWidget,
                                    GlobalSession->MainWindow(), nullptr);

  auto qt_showAllQtObjects = [](void* clientData, Tcl_Interp* interp, int argc,
                                const char* argv[]) -> int {
    QWidget* w = static_cast<QWidget*>(clientData);
    QWidget* topWidget = QApplication::topLevelAt(w->mapToGlobal(QPoint()));
    if (topWidget) {
      auto children = topWidget->findChildren<QObject*>();
      QStringList objectsNames;
      for (auto child : children) {
        if (!child->objectName().isEmpty()) objectsNames += child->objectName();
      }
      Tcl_AppendResult(interp, qPrintable(objectsNames.join(" ")), nullptr);
    }
    return TCL_OK;
  };
  session->TclInterp()->registerCmd("qt_showAllQtObjects", qt_showAllQtObjects,
                                    GlobalSession->MainWindow(), nullptr);

  auto qt_testWidget = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    if (argc < 2) {
      Tcl_AppendResult(interp, qPrintable("Usage: qt_testWidget ?widget?"),
                       nullptr);
      return TCL_ERROR;
    }
    QString widgetStr{argv[1]};
    widgetStr.remove("QWidget(");
    widgetStr.remove(")");
    bool ok{false};
    ulong widgetPtr = widgetStr.toULong(&ok, 16);
    if (!ok || (widgetPtr == 0)) {
      Tcl_AppendResult(
          interp, qPrintable("Wrong format. Expetced: QWidget(0x?number?)"),
          nullptr);
      return TCL_ERROR;
    }
    QWidget* widget = reinterpret_cast<QWidget*>(widgetPtr);

    QWidget* w = static_cast<QWidget*>(clientData);
    QWidget* topWidget = QApplication::topLevelAt(w->mapToGlobal(QPoint()));
    if (!topWidget) topWidget = w;
    if (!topWidget) {
      Tcl_AppendResult(interp, qPrintable("topWidget == nullptr"), nullptr);
      return TCL_ERROR;
    }
    auto children = topWidget->findChildren<QWidget*>();
    if (!children.contains(widget)) {
      Tcl_AppendResult(interp, qPrintable("Unknown widget"), nullptr);
      return TCL_ERROR;
    }

    Tcl_AppendResult(
        interp,
        qPrintable(
            QString("QWidget object name = %1").arg(widget->objectName())),
        nullptr);
    return TCL_OK;
  };
  session->TclInterp()->registerCmd("qt_testWidget", qt_testWidget,
                                    GlobalSession->MainWindow(), nullptr);
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

#include <QDebug>
void registerAllFoedagCommands(QWidget* widget, FOEDAG::Session* session) {
  // Used in "make test_install"
  auto hello = [](void* clientData, Tcl_Interp* interp, int argc,
                  const char* argv[]) -> int {
    GlobalSession->TclInterp()->evalCmd("puts Hello!");
    return 0;
  };
  session->TclInterp()->registerCmd("hello", hello, 0, 0);

  session->GetCompiler()->RegisterCommands(GlobalSession->TclInterp(), false);

  // GUI Mode
  if (widget) {
    // New Project Wizard
    if (FOEDAG::MainWindow* win = dynamic_cast<FOEDAG::MainWindow*>(widget)) {
      registerNewProjectCommands(win->NewProjectDialog(), session);
    }
    // Temp command to demo Settings prototype
    if (FOEDAG::MainWindow* win = dynamic_cast<FOEDAG::MainWindow*>(widget)) {
      auto DemoSettingsFn = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
        qDebug() << "Storing version number under \"Version\"";
        FOEDAG::Settings::setValue("Version", 0.1);

        qDebug() << "Example setting nested json";
        QJsonObject nested{{"nested", "test"}};
        QJsonObject val3{
            {"a", "1"}, {"b", 2}, {"c", 3.3}, {"nestTest", nested}};
        FOEDAG::Settings::setValue("testing", val3);
        fprintf(stderr, "Json: %s\n",
                qPrintable(FOEDAG::Settings::getJsonStr()));

        QJsonObject nested2{{"nested", "test-replace"}};
        QJsonObject val4{
            {"a", "1_2"}, {"b", 6}, {"c", 8.8}, {"nestTest", nested2}};
        FOEDAG::Settings::updateJson("testing", val4);
        FOEDAG::Settings::updateJson("empty", val4);

        QJsonObject user{{"user_value", 3.5}};
        FOEDAG::Settings::updateJson("empty", user);
        fprintf(stderr, "Json: %s\n",
                qPrintable(FOEDAG::Settings::getJsonStr()));

        qDebug() << "Load ./settings_test.json into \"Settings\" - "
                    "FOEDAG::Settings::loadJsonFile(\"./settings_test.json\", "
                    "\"Settings\")";
        FOEDAG::Settings::loadJsonFile("./settings_test.json", "Settings");
        fprintf(stderr, "Settings JSON after loadJsonFile(): %s\n",
                qPrintable(FOEDAG::Settings::getJsonStr()));

        qDebug() << "\nGet Test - FOEDAG::Settings::get(\"Version\");";
        qDebug() << "\t" << FOEDAG::Settings::get("Version");

        qDebug()
            << "\nGetNested Test - "
               "FOEDAG::Settings::getNested(\"Settings.Tasks.Synth\", \".\");";
        qDebug() << "\t"
                 << FOEDAG::Settings::getNested("Settings.Tasks.Synth", ".");

        // Try reading some data
        FOEDAG::Tasks::getTasks();

        return TCL_OK;
      };

      session->TclInterp()->registerCmd("DemoSettings", DemoSettingsFn, 0, 0);
    }
  }
}
