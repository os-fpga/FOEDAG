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

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Command/Command.h"
#include "Command/CommandStack.h"
#include "Compiler/Design.h"
#include "Main/CommandLine.h"
#include "Tcl/TclInterpreter.h"

#ifndef COMPILER_H
#define COMPILER_H

namespace FOEDAG {

class Compiler {
 public:
  enum Action { NoAction, Synthesis, Global, Detailed, Routing, STA, Bitream };
  enum State {
    None,
    Synthesized,
    GloballyPlaced,
    Placed,
    Routed,
    TimingAnalyzed,
    BistreamGenerated
  };

  Compiler(TclInterpreter* interp, Design* design, std::ostream& out)
      : m_interp(interp), m_design(design), m_out(out) {}

  ~Compiler();

  State CompilerState() { return m_state; }
  bool compile(Action action);
  void stop() { m_stop = true; }
  TclInterpreter* TclInterp() { return m_interp; }
  Design* GetDesign() { return m_design; }
  bool registerCommands();
  bool Clear();
  bool Synthesize();
  bool GlobalPlacement();
  bool Placement();
  bool Route();
  bool TimingAnalysis();
  bool GenerateBitstream();

 private:
  TclInterpreter* m_interp = nullptr;
  Design* m_design = nullptr;
  bool m_stop = false;
  State m_state = None;
  std::ostream& m_out;
};

}  // namespace FOEDAG

#endif
