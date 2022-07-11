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
#include "CompilerNotifier.h"

#include "Compiler/Compiler.h"
#include "Console/TclConsole.h"

namespace FOEDAG {

CompilerNotifier::CompilerNotifier(TclConsole *c) : m_console{c} {
  connect(m_console, &TclConsole::aborted, this, &CompilerNotifier::aborted);
}

void CompilerNotifier::initIterpreter(TclInterpreter *interp) {
  if (m_console) m_console->registerInterpreter(interp->getInterp());
}

void CompilerNotifier::notifyStart() {
  if (m_console) {
    m_queue.enqueue(0);
    if (m_queue.count() == 1) m_console->setTclCommandInProggress(true);
  }
}

void CompilerNotifier::notifyFinish() {
  if (m_console) {
    if (!m_queue.isEmpty()) m_queue.dequeue();
    if (m_queue.count() == 0) m_console->setTclCommandInProggress(false);
    emit compilerStateChanged(static_cast<int>(m_compiler->CompilerState()));
  }
}

void CompilerNotifier::aborted() {
  if (m_compiler) m_compiler->Stop();
}

}  // namespace FOEDAG
