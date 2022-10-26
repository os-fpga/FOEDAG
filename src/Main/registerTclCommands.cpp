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
#include <QDialogButtonBox>
#include <QLabel>
#include <QMenu>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Command/CommandStack.h"
#include "CommandLine.h"
#include "Compiler/Log.h"
#include "Foedag.h"
#include "IpConfigurator/IpConfigWidget.h"
#include "Main/Tasks.h"
#include "Main/WidgetFactory.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "NewProject/Main/registerNewProjectCommands.h"
#include "Tcl/TclInterpreter.h"
#include "TextEditor/text_editor.h"
#include "qttclnotifier.hpp"

// This tag will be used as a property on found widgetes to store whatever their
// equivalent of "Text" is. This field will be compared against the
// FindWidgetRequest.widgetText field when filtering results
#define QT_TEST_TEXT "qt_test_Text"

struct FindWidgetRequest {
  QWidget* searchWidget{nullptr};
  QString widgetType{};
  QString widgetText{};
  QString objectname{};
  bool matchCase = false;
  bool includeHidden = true;
  int nthResult = -1;  // maybe rename? currently 0 based
};

QWidgetList findWidgets(FindWidgetRequest request) {
  QString type = request.widgetType.toLower();
  bool forceIncludeHidden = false;
  QWidgetList results{};

  // Any widget that determines its own searchWidget (like QDialog which must
  // search topLevelWidgets) can set this flag to false to make it into the
  // conditional below w/o having set a searchWidget
  bool searchWidgetRequired{true};
  if (type == "dialog" || type == "qdialog") {
    searchWidgetRequired = false;
  }

  // findChildren is intentionally passed null so that all widgets of that
  // type are returned (passing "" only returns objects whose objectName is
  // "") https://doc.qt.io/qt-5/qobject.html#findChildren
  if (!searchWidgetRequired || request.searchWidget) {
    // sma if matchCase and type aren't set and objectName is, we could probably
    // shortcut the search and just do findChild<QWidget*>(objectName)
    if (type == "qpushbutton" || type == "button" || type == "pushbutton" ||
        type == "btn") {
      for (auto btn : request.searchWidget->findChildren<QAbstractButton*>()) {
        btn->setProperty(QT_TEST_TEXT, btn->text());
        results.append(btn);
      }
    } else if (type == "qdialog" || type == "dialog") {
      // QDialogs are always top level widgets so request.searchWidget is
      // ignored https://doc.qt.io/qt-5/qdialog.html#details
      for (QWidget* widget : qApp->topLevelWidgets()) {
        auto ptr = qobject_cast<QDialog*>(widget);
        if (ptr) {
          ptr->setProperty(QT_TEST_TEXT, ptr->windowTitle());
          results.append(ptr);
        }
      }
    } else if (type == "qmenu" || type == "menu") {
      forceIncludeHidden = true;
      for (auto menu : request.searchWidget->findChildren<QMenu*>()) {
        menu->setProperty(QT_TEST_TEXT, menu->title());
        results.append(menu);
      }
      // } else if (type == "qaction" || type == "action") {
      //   auto actions = request.searchWidget->findChildren<QAction*>();
      // not yet implemented
    } else if (type.isEmpty()) {
      // Grab all widgets if no type was specified
      results = request.searchWidget->findChildren<QWidget*>();
    }
  } else {
    // error about parent not being provided
  }

  bool searchText = !request.widgetText.isEmpty();
  bool searchObjectName = !request.objectname.isEmpty();
  bool requiresMatch = searchText || searchObjectName;

  QString requestName = request.objectname;
  QString requestText = request.widgetText;

  // std::cout << "Pre-filter results count: " << results.count()
  //           << std::endl;

  // Step through the results and remove non-matches based on additional request
  // criteria
  QWidgetList keep{};
  if (requiresMatch) {
    for (auto result : results) {
      if (searchObjectName) {
        QString resultName = result->objectName();

        if (!request.matchCase) {
          resultName = resultName.toLower();
          requestName = requestName.toLower();
        }

        if (resultName == requestName) {
          keep.append(result);
        } else {
          continue;  // on a failed match we can skip further checks on this
                     // item
        }
      }
      if (searchText) {
        // QT_TEST_TEXT should be set by each widget type's find code since not
        // all widgets support ->text()
        QString resultText = result->property(QT_TEST_TEXT).toString();

        if (!request.matchCase) {
          resultText = resultText.toLower();
          requestText = requestText.toLower();
        }

        // Remove "&" from text so thay "&File" isn't required for menus etc
        // sma not sure if this should happen in all occasions. Only bad
        // scenario i can think of is a user searching for something that
        // counted on multiple &'s or a & in a specific spot which seems
        // unlikely
        resultText.replace("&", "");
        requestText.replace("&", "");

        if (resultText == requestText) {
          keep.append(result);
        } else {
          continue;  // on a failed match we can skip further checks on this
                     // item
        }
      }
    }

    // swap our results to the keep list if we had search criteria
    results = keep;
  }

  // Filter out hidden/non-visible widgets
  if (!request.includeHidden && !forceIncludeHidden) {
    QWidgetList keep{};
    for (auto result : results) {
      if (result->isVisible()) {
        keep.append(result);
      }
    }
    results = keep;
  }

  // Grab the nth result if requested
  if (request.nthResult > -1) {
    // Take the nth result only
    if (request.nthResult < results.count() - 1) {
      results = QWidgetList{results[request.nthResult]};
    }
  }

  // std::cout << "Final results count: " << results.count()
  //           << std::endl;  // sma remove
  return results;
}

// Takes a string in the form of "QWidget(0x?number?)" or "0x?number?" and
// attempts to convert it to a QWidget*
QWidget* widgetAddrToPtr(const QString& addrStr, bool& convertStatus) {
  QString widgetStr = addrStr;
  // remove decoration that qt_getWidget adds
  widgetStr.remove("QWidget(");
  widgetStr.remove(")");
  convertStatus = false;
  ulong widgetPtr = widgetStr.toULong(&convertStatus, 16);
  // This can crash if a garbage pointer string is passed like 0x12
  QWidget* widget = reinterpret_cast<QWidget*>(widgetPtr);
  return widget;
}

void openMenu(QWidget* searchWidget, const QStringList& menuEntries) {
  if (searchWidget && !menuEntries.isEmpty()) {
    // Find the top menu
    FindWidgetRequest req;
    req.widgetText = menuEntries[0];
    req.widgetType = "QMenu";
    req.searchWidget = searchWidget;
    auto findResults = findWidgets(req);

    // @skyler-rs this currently loops through QMenu::actions() which means we
    // have to duplicate logic like request.matchCase. It might be better to
    // implement this using only findWidgets to find each child action which
    // will then filter based on the request struct.

    if (findResults.count() > 0) {
      QMenu* currentMenu = qobject_cast<QMenu*>(findResults[0]);
      // Track the menus that get opened so we can close them later
      QList<QMenu*> openMenus{currentMenu};
      for (auto menuEntry : menuEntries) {
        QString menuEntryText = menuEntry;
        menuEntryText.replace("&", "");
        if (!req.matchCase) {
          menuEntryText = menuEntryText.toLower();
        }
        for (auto action : currentMenu->actions()) {
          // Remove &'s so user doesn't have to type &File etc
          QString actionText = action->text();
          actionText.replace("&", "");
          if (!req.matchCase) {
            actionText = actionText.toLower();
          }
          if (actionText == menuEntryText) {
            if (action->menu()) {
              // If the current entry is a parent menu, open and track it
              currentMenu = action->menu();
              openMenus.append(currentMenu);
              // currently there is no logic to place the menu in the actual
              // place it should show when clicked
              currentMenu->popup(QPoint(0, 0));
              break;
            } else {
              // execute the target menu action
              action->activate(QAction::Trigger);
              break;
            }
          }
        }
      }

      // close the menus we opened
      std::reverse(openMenus.begin(), openMenus.end());
      for (auto menu : openMenus) {
        menu->close();
      }
    }
  }
}

// This is intended to be called on a ptr returned by findWidgets.
// It will attempt to convert the ptr to a button type and then programatically
// click it
void clickButton(QWidget* ptr) {
  QAbstractButton* btn = qobject_cast<QAbstractButton*>(ptr);
  if (btn) {
    btn->animateClick();
  }
}

void registerBasicGuiCommands(FOEDAG::Session* session) {
  auto gui_start = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    LOG_CMD("gui_start");
    GlobalSession->windowShow();
    return 0;
  };
  session->TclInterp()->registerCmd("gui_start", gui_start, 0, 0);

  auto gui_stop = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    LOG_CMD("gui_stop");
    GlobalSession->windowHide();
    return 0;
  };
  session->TclInterp()->registerCmd("gui_stop", gui_stop, 0, 0);

  auto create_project = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Q_UNUSED(interp);
    LOG_CMD("create_project");
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
    Tcl_Exit(0);  // Cannot use Tcl_Finalize that issues signals probably due
                  // to the Tcl/QT loop
    return 0;
  };
  session->TclInterp()->registerCmd("tcl_exit", tcl_exit, 0, 0);

  auto help = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    LOG_CMD("help");
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
            .arg(QString::number(reinterpret_cast<uint64_t>(widget), 16));
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
    bool ok{false};
    QWidget* widget = widgetAddrToPtr(argv[1], ok);
    if (!ok || !widget) {
      Tcl_AppendResult(
          interp, qPrintable("Wrong format. Expetced: QWidget(0x?number?)"),
          nullptr);
      return TCL_ERROR;
    }

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

  auto show_about = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    if (argc < 6) {
      Tcl_AppendResult(interp,
                       qPrintable("Expected Syntax: show_about name, "
                                  "version, git_hash, date, url, build_type"),
                       nullptr);
      return TCL_ERROR;
    }
    FOEDAG::ProjectInfo info{argv[1], argv[2], argv[3], argv[4], argv[5]};
    QWidget* parent = static_cast<QWidget*>(clientData);
    FOEDAG::AboutWidget* about = new FOEDAG::AboutWidget(
        info, GlobalSession->Context()->DataPath(), parent);
    about->show();
    return TCL_OK;
  };

  session->TclInterp()->registerCmd("show_about", show_about,
                                    GlobalSession->MainWindow(), nullptr);

  auto qt_findWidgets = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    QString parent{};
    QString type{};
    QString text{};
    QString objectName{};
    int nth{-1};
    bool matchCase{false};
    bool includeHidden{false};
    QWidget* topWidget{};

    // increment the current index and grab the next argv
    auto nextArg = [argc, argv](int& currentIndex) -> QString {
      currentIndex++;
      if (currentIndex < argc) {
        return argv[currentIndex];
      } else {
        return "";
      }
    };

    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-parent") {
        if (i < argc) {
          bool ok{false};
          topWidget = widgetAddrToPtr(argv[i + 1], ok);
          if (!ok || !topWidget) {
            // error msg, revert to top level ui for topWidget
          }
        } else {
          // error msg
        }
      } else if (arg == "-type") {
        type = nextArg(i);
        if (type == "") {
          // error msg
        }
      } else if (arg == "-text") {
        text = nextArg(i);
        if (text == "") {
          // error msg
        }
      } else if (arg == "-object_name") {
        objectName = nextArg(i);
        if (objectName == "") {
          // error msg
        }
      } else if (arg == "-nth") {
        auto str = nextArg(i);
        if (str == "") {
          // error msg
        } else {
          nth = str.toInt();
        }
      } else if (arg == "-match_case") {
        matchCase = true;
      } else if (arg == "-include_hidden") {
        includeHidden = true;
      }
    }

    if (!text.isEmpty() && type.isEmpty()) {
      Tcl_AppendResult(
          interp,
          qPrintable(
              "qt_findWidgets: -text requires that -type be set as well"),
          (char*)NULL);
      return TCL_ERROR;
    }

    // qApp might be a higher target to search? Don't know if it matters
    QWidget* w = static_cast<QWidget*>(clientData);
    if (!topWidget) {
      topWidget = QApplication::topLevelAt(w->mapToGlobal(QPoint()));
    }
    if (!topWidget) topWidget = w;
    if (!topWidget) {
      Tcl_AppendResult(interp, qPrintable("topWidget == nullptr"), (char*)NULL);
      return TCL_ERROR;
    }

    FindWidgetRequest request{topWidget, type,          text, objectName,
                              matchCase, includeHidden, nth};

    QWidgetList widgets = findWidgets(request);

    if (widgets.count()) {
      QString result{};
      for (auto widget : widgets) {
        result +=
            QString("QWidget(0x%1)\n")
                .arg(QString::number(reinterpret_cast<uint64_t>(widget), 16));

        // sma remove, temp highlight found widgets in red
        widget->setStyleSheet("background-color: red;");
      }
      Tcl_AppendResult(interp, qPrintable(result), (char*)NULL);
      return TCL_OK;
    }
    Tcl_AppendResult(interp, qPrintable("Couldn't find any matching widgets"),
                     (char*)NULL);
    return TCL_ERROR;
  };
  session->TclInterp()->registerCmd("qt_findWidgets", qt_findWidgets,
                                    GlobalSession->MainWindow(), nullptr);

  auto qt_clickButton = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    if (argc < 1) {
      Tcl_AppendResult(interp,
                       qPrintable("Expected Syntax: qt_clickButton 0x<ptr> or "
                                  "qt_clickButton QWidget(0x<ptr>)"),
                       nullptr);
      return TCL_ERROR;
    }

    bool ok{false};
    QWidget* widget = widgetAddrToPtr(argv[1], ok);
    if (ok) {
      clickButton(widget);
    }

    return TCL_OK;
  };
  session->TclInterp()->registerCmd("qt_clickButton", qt_clickButton,
                                    GlobalSession->MainWindow(), nullptr);

  auto qt_dlg_test = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    // This is a proof of concept to show that a modal dialog can be interacted
    // with by storing the desired actions in a lambda and firing it w/ a delay
    // so it executes after the modal dialog has started blocking
    // Still need to come up with a way of queuing these through the tcl
    // interface
    if (argc < 1) {
      Tcl_AppendResult(interp,
                       qPrintable("Expected Syntax: qt_dlg_test 0x<ptr> or "
                                  "qt_dlg_test QWidget(0x<ptr>)"),
                       nullptr);
      return TCL_ERROR;
    }

    QWidget* widget = static_cast<QWidget*>(clientData);
    if (widget) {
      QTimer::singleShot(1000, []() {
        std::cout << "Continuing\n" << std::endl;

        FindWidgetRequest findDlg;
        findDlg.widgetType = "qdialog";
        findDlg.widgetText = "Open File";
        auto dlgs = findWidgets(findDlg);

        if (dlgs.count()) {
          std::cout << "found dlg" << std::endl;
          FindWidgetRequest cancelBtn;
          cancelBtn.widgetType = "qpushbutton";
          cancelBtn.widgetText = "cancel";
          cancelBtn.searchWidget = dlgs[0];
          auto btn = findWidgets(cancelBtn);

          if (btn.count()) {
            std::cout << "found btn " << btn[0] << " "
                      << btn[0]->metaObject()->className() << std::endl;
            clickButton(btn[0]);
          }
        }
      });
      // Grab the top widget to search through
      widget = QApplication::topLevelAt(widget->mapToGlobal(QPoint()));
      openMenu(widget, {"File", "Open File..."});
    }

    return TCL_OK;
  };
  session->TclInterp()->registerCmd("qt_dlg_test", qt_dlg_test,
                                    GlobalSession->MainWindow(), nullptr);

  auto qt_openMenu = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    if (argc < 2) {
      Tcl_AppendResult(
          interp,
          qPrintable("Expected Syntax: qt_openMenu menuEntryText {menu entry "
                     "text with spaces} menuEntry ..."),
          nullptr);
      return TCL_ERROR;
    }

    // Get the menu names requested
    QStringList menuPath{};
    for (int i = 1; i < argc; i++) {
      menuPath.append(argv[i]);
    }

    // This currently doesn't use a parent widget to search through assuming the
    // menus are always accesible from the top level. If we need finer control,
    // this should be updated to support -parent as an arg
    QWidget* widget = static_cast<QWidget*>(clientData);
    if (widget) {
      // Grab the top widget to search through
      widget = QApplication::topLevelAt(widget->mapToGlobal(QPoint()));
      openMenu(widget, menuPath);
    } else {
      // error
    }

    return TCL_OK;
  };
  session->TclInterp()->registerCmd("qt_openMenu", qt_openMenu,
                                    GlobalSession->MainWindow(), nullptr);

  auto qt_isEnabled = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    if (argc < 1) {
      Tcl_AppendResult(interp,
                       qPrintable("Expected Syntax: qt_isEnabled 0x<ptr> or "
                                  "qt_clickButton QWidget(0x<ptr>)"),
                       nullptr);
      return TCL_ERROR;
    }

    bool ok{false};
    QWidget* widget = widgetAddrToPtr(argv[1], ok);
    if (ok && widget) {
      QString result = QString::number(widget->isEnabled());
      Tcl_AppendResult(interp, qPrintable(result), nullptr);
      return TCL_OK;
    } else {
      QString msg =
          QString("Failed to find a qt widget with address of %1").arg(argv[1]);
      Tcl_AppendResult(interp, qPrintable(msg), nullptr);
      return TCL_ERROR;
    }
  };
  session->TclInterp()->registerCmd("qt_isEnabled", qt_isEnabled,
                                    GlobalSession->MainWindow(), nullptr);
}

void registerBasicBatchCommands(FOEDAG::Session* session) {
  auto tcl_exit = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    delete GlobalSession;
    // Do not log this command
    Tcl_Exit(0);  // Cannot use Tcl_Finalize that issues signals probably due
                  // to the Tcl/QT loop
    return 0;
  };
  session->TclInterp()->registerCmd("tcl_exit", tcl_exit, 0, 0);

  auto help = [](void* clientData, Tcl_Interp* interp, int argc,
                 const char* argv[]) -> int {
    LOG_CMD("help");
    return 0;
  };
  session->TclInterp()->registerCmd("help", help, 0, 0);
}

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

    if (FOEDAG::MainWindow* win = dynamic_cast<FOEDAG::MainWindow*>(widget)) {
      auto DemoWidgetsFn = [](void* clientData, Tcl_Interp* interp, int argc,
                              const char* argv[]) -> int {
        FOEDAG::createTaskDialog("TclExample")->show();
        return TCL_OK;
      };
      session->TclInterp()->registerCmd("DemoWidgets", DemoWidgetsFn, 0, 0);

      auto EditSettingsFn = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
        QString category = "";
        if (argc > 1) {
          category = argv[1];
        }
        QDialog* dlg = FOEDAG::createTopSettingsDialog(
            GlobalSession->GetSettings()->getJson(), category);
        // flag to open the dlg non-modally so testing scripts can interact
        // with it
        if (argc > 2 && strcmp(argv[2], "1") == 0) {
          dlg->show();
        } else {
          dlg->exec();
        }

        return TCL_OK;
      };
      session->TclInterp()->registerCmd("EditSettings", EditSettingsFn, 0, 0);

      auto EditTaskSettingsFn = [](void* clientData, Tcl_Interp* interp,
                                   int argc, const char* argv[]) -> int {
        if (argc == 2) {
          FOEDAG::createTaskDialog(argv[1])->show();
          return TCL_OK;
        } else {
          Tcl_AppendResult(
              interp, qPrintable("Expected Syntax: EditTaskSettings TaskName"),
              nullptr);
          return TCL_ERROR;
        }
      };
      session->TclInterp()->registerCmd("EditTaskSettings", EditTaskSettingsFn,
                                        0, 0);

      auto ipconfiguratorDlgFn = [](void* clientData, Tcl_Interp* interp,
                                    int argc, const char* argv[]) -> int {
        QWidget* w = static_cast<QWidget*>(clientData);

        if (argc == 2) {
          FOEDAG::IpConfigWidget* widget =
              new FOEDAG::IpConfigWidget(w, argv[1]);
          widget->show();

          return TCL_OK;
        } else {
          Tcl_AppendResult(
              interp,
              qPrintable("Expected Syntax: ipconfigurator_show_dlg IpName"),
              nullptr);
          return TCL_ERROR;
        }
      };
      session->TclInterp()->registerCmd("ipconfigurator_show_dlg",
                                        ipconfiguratorDlgFn, 0, 0);
    }
  }
}
