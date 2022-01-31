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

#include <QApplication>

#include "Main/Foedag.h"
#include "Main/qttclnotifier.hpp"
#include "Tcl/TclInterpreter.h"
#include "TextEditor/text_editor.h"

FOEDAG::Session* GlobalSession;

void registerTextEditorCommands(FOEDAG::Session* session) {
  auto texteditorshow = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::TextEditor* textEditor = (FOEDAG::TextEditor*)(clientData);
    textEditor->ShowTextEditor();
    return 0;
  };
  session->TclInterp()->registerCmd("texteditor_show", texteditorshow,
                                    GlobalSession->MainWindow(), 0);

  auto texteditorhide = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::TextEditor* textEditor = (FOEDAG::TextEditor*)(clientData);
    textEditor->ClosetextEditor();
    return 0;
  };
  session->TclInterp()->registerCmd("texteditor_close", texteditorhide,
                                    GlobalSession->MainWindow(), 0);

  auto openfile = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::TextEditor* textEditor = (FOEDAG::TextEditor*)(clientData);
    textEditor->GetTextEditor()->resize(850, 650);
    textEditor->ShowTextEditor();
    if (argc > 1) {
      textEditor->SlotOpenFile(argv[1]);
    }
    return 0;
  };
  session->TclInterp()->registerCmd("openfile", openfile,
                                    GlobalSession->MainWindow(), 0);

  //  session->TclInterp()->evalCmd(
  //      "puts \"Put texteditor_show to test projnavigator GUI.\"");
}

int main(int argc, char** argv) {
  FOEDAG::CommandLine* cmd = new FOEDAG::CommandLine(argc, argv);
  cmd->processArgs();
  FOEDAG::Foedag* foedag = new FOEDAG::Foedag(
      cmd, FOEDAG::GUI_TYPE::GT_TEXT_EDITOR, registerTextEditorCommands);
  return foedag->init(cmd->WithQt());
}
