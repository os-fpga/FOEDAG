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
#include "TaskManager.h"
#include "Tcl/TclInterpreter.h"

#ifndef COMPILER_H
#define COMPILER_H

namespace FOEDAG {

class TclInterpreterHandler;
class Compiler {
 public:
  enum Action {
    NoAction,
    Synthesis,
    Global,
    Detailed,
    Routing,
    STA,
    Bitstream,
    Batch
  };
  enum State {
    None,
    Synthesized,
    GloballyPlaced,
    Placed,
    Routed,
    TimingAnalyzed,
    BistreamGenerated
  };

  Compiler(TclInterpreter* interp, std::ostream& out,
           TclInterpreterHandler* tclInterpreterHandler = nullptr);

  ~Compiler();
  void BatchScript(const std::string& script) { m_batchScript = script; }
  State CompilerState() { return m_state; }
  bool Compile(Action action);
  void Stop();
  TclInterpreter* TclInterp() { return m_interp; }
  Design* GetActiveDesign() { return m_design; }
  Design* GetDesign(const std::string name);
  void SetDesign(Design* design) {
    m_design = design;
    m_designs.push_back(design);
  }
  bool SetActiveDesign(const std::string name);
  bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  bool Clear();
  void start();
  void finish();

  std::string& getResult() { return m_result; }

  void setTaskManager(TaskManager* newTaskManager);

 protected:
  virtual bool Synthesize();
  virtual bool GlobalPlacement();
  virtual bool Placement();
  virtual bool Route();
  virtual bool TimingAnalysis();
  virtual bool GenerateBitstream();
  virtual bool RunBatch();
  bool RunCompileTask(Action action);
  virtual bool ExecuteSystemCommand(const std::string& command);
  void Message(const std::string& message) { m_out << message << std::flush; }

 private:
  TclInterpreter* m_interp = nullptr;
  Design* m_design = nullptr;
  bool m_stop = false;
  State m_state = None;
  std::ostream& m_out;
  std::string m_batchScript;
  std::string m_result;
  TclInterpreterHandler* m_tclInterpreterHandler;
  TaskManager* m_taskManager{nullptr};
  std::vector<Design*> m_designs;
};

}  // namespace FOEDAG

#endif
