/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

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
#pragma once

#include <QObject>
#include <QQueue>

#include "Compiler/TclInterpreterHandler.h"

namespace FOEDAG {

class TclConsole;
/*!
 * \brief The CompilerNotifier class implements interaction between compiler and
 * console backend
 */
class CompilerNotifier : public QObject, public FOEDAG::TclInterpreterHandler {
  Q_OBJECT
  TclConsole *m_console;

 public:
  CompilerNotifier(TclConsole *c);
  /*!
   * \brief initIterpreter. Register interpreter \param interp in the console
   * backend for the batch command.
   */
  void initIterpreter(TclInterpreter *interp) override;

  /*!
   * \brief notifyStart. Notify that compilation process has started.
   */
  void notifyStart() override;

  /*!
   * \brief notifyFinish. Notify that compilation process has finished.
   */
  void notifyFinish() override;

 signals:
  void compilerStateChanged(int);

 private slots:
  void aborted();

 private:
  QQueue<int> m_queue;  // handle recursive start/stop
};
}  // namespace FOEDAG
