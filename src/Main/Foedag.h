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
#include "MainWindow/main_window.h"

extern FOEDAG::Session* GlobalSession;

void registerBasicGuiCommands(FOEDAG::Session* session);
void registerBasicBatchCommands(FOEDAG::Session* session);

#ifndef FOEDAG_H
#define FOEDAG_H

namespace FOEDAG {

typedef QWidget*(MainWindowBuilder)(FOEDAG::CommandLine* cmdLine);

typedef void(RegisterTclFunc)(FOEDAG::Session* session);

class Foedag {
 private:
 public:
  Foedag(FOEDAG::CommandLine* cmdLine, MainWindowBuilder* mainWinBuilder,
         RegisterTclFunc* registerTclFunc = nullptr)
      : m_cmdLine(cmdLine),
        m_mainWinBuilder(mainWinBuilder),
        m_registerTclFunc(registerTclFunc) {}
  bool initGui();
  bool initBatch();

  virtual ~Foedag() {}

 protected:
  FOEDAG::CommandLine* m_cmdLine = nullptr;
  FOEDAG::MainWindow* m_mainWin = nullptr;
  MainWindowBuilder* m_mainWinBuilder = nullptr;
  RegisterTclFunc* m_registerTclFunc = nullptr;
};

}  // namespace FOEDAG

#endif
