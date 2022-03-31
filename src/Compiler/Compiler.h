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
class Session;
class Compiler {
 public:
  enum Action {
    NoAction,
    IPGen,
    Synthesis,
    Global,
    Detailed,
    Routing,
    STA,
    Power,
    Bitstream,
    Batch
  };
  enum State {
    None,
    IPGenerated,
    Synthesized,
    GloballyPlaced,
    Placed,
    Routed,
    TimingAnalyzed,
    PowerAnalyzed,
    BistreamGenerated
  };
  // Most common use case, create the compiler in your main
  Compiler() {}

  Compiler(TclInterpreter* interp, std::ostream* out,
           TclInterpreterHandler* tclInterpreterHandler = nullptr);
  void SetInterpreter(TclInterpreter* interp) { m_interp = interp; }
  void SetOutStream(std::ostream* out) { m_out = out; };
  void SetTclInterpreterHandler(TclInterpreterHandler* tclInterpreterHandler) {
    m_tclInterpreterHandler = nullptr;
  }
  void SetSession(Session* session) { m_session = session; }
  Session* GetSession() { return m_session; }
  ~Compiler();

  void BatchScript(const std::string& script) { m_batchScript = script; }
  State CompilerState() { return m_state; }
  bool Compile(Action action);
  void Stop();
  TclInterpreter* TclInterp() { return m_interp; }
  Design* GetActiveDesign() { return m_design; }
  Design* GetDesign(const std::string name);
  void SetDesign(Design* design);
  bool SetActiveDesign(const std::string name);
  bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  bool Clear();
  void start();
  void finish();

  std::string& getResult() { return m_result; }

  void setTaskManager(TaskManager* newTaskManager);

 protected:
  /* Methods that can be customized for each new compiler flow */
  virtual bool IPGenerate();
  virtual bool Synthesize();
  virtual bool GlobalPlacement();
  virtual bool Placement();
  virtual bool Route();
  virtual bool TimingAnalysis();
  virtual bool PowerAnalysis();
  virtual bool GenerateBitstream();

  /* Compiler class utilities */
  bool RunBatch();
  bool RunCompileTask(Action action);
  virtual bool ExecuteSystemCommand(const std::string& command);
  virtual bool ExecuteAndMonitorSystemCommand(const std::string& command);
  void Message(const std::string& message) {
    // m_interp->evalCmd("puts " + message + "; flush stdout; ");
    if (m_out) (*m_out) << message << std::flush;
  }
  std::string replaceAll(std::string_view str, std::string_view from,
                         std::string_view to);
  TclInterpreter* m_interp = nullptr;
  Session* m_session = nullptr;
  Design* m_design = nullptr;
  bool m_stop = false;
  State m_state = None;
  std::ostream* m_out = &std::cout;
  std::string m_batchScript;
  std::string m_result;
  TclInterpreterHandler* m_tclInterpreterHandler{nullptr};
  TaskManager* m_taskManager{nullptr};
  std::vector<Design*> m_designs;
};

}  // namespace FOEDAG

#endif
