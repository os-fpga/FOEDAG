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
#include "Main/Settings.h"
#include "Main/ToolContext.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"

extern FOEDAG::Session* GlobalSession;

namespace FOEDAG {
class TextEditor;
}

void registerBasicGuiCommands(FOEDAG::Session* session);
void registerBasicBatchCommands(FOEDAG::Session* session);
void registerAllFoedagCommands(QWidget* widget, FOEDAG::Session* session);

#ifndef FOEDAG_H
#define FOEDAG_H

namespace FOEDAG {

typedef QWidget*(MainWindowBuilder)(FOEDAG::Session* session);

typedef void(RegisterTclFunc)(QWidget* widget, FOEDAG::Session* session);

class TclWorker;
class Foedag {
 public:
  Foedag(FOEDAG::CommandLine* cmdLine, MainWindowBuilder* mainWinBuilder,
         RegisterTclFunc* registerTclFunc = nullptr,
         Compiler* compiler = nullptr, Settings* settings = nullptr,
         ToolContext* context = nullptr);
  virtual ~Foedag();

  bool init(GUI_TYPE guiType);

 private:
  bool initQmlGui();

 public:
  static FOEDAG::GUI_TYPE getGuiType(const bool& withQt, const bool& withQml);
  bool initGui();
  bool initBatch();
  const ToolContext* Context() { return m_context; }

 protected:
  FOEDAG::CommandLine* m_cmdLine = nullptr;
  FOEDAG::MainWindow* m_mainWin = nullptr;
  MainWindowBuilder* m_mainWinBuilder = nullptr;
  RegisterTclFunc* m_registerTclFunc = nullptr;
  ToolContext* m_context = nullptr;
  Compiler* m_compiler = nullptr;
  Settings* m_settings = nullptr;
  TclWorker* m_tclChannelHandler{nullptr};
};

}  // namespace FOEDAG

#endif
