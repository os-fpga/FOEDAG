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

#include <filesystem>
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

class TaskManager;
class TclInterpreterHandler;
class Session;
class DesignManager;
class TclCommandIntegration;
class Constraints;
class Compiler {
 public:
  enum Action {
    NoAction,
    IPGen,
    Synthesis,
    Pack,
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
    Packed,
    GloballyPlaced,
    Placed,
    Routed,
    TimingAnalyzed,
    PowerAnalyzed,
    BistreamGenerated
  };
  enum SynthesisOpt { NoOpt, Area, Delay, Mixed };
  // Most common use case, create the compiler in your main
  Compiler() = default;

  Compiler(TclInterpreter* interp, std::ostream* out,
           TclInterpreterHandler* tclInterpreterHandler = nullptr);
  void SetInterpreter(TclInterpreter* interp) { m_interp = interp; }
  void SetOutStream(std::ostream* out) { m_out = out; };
  void SetErrStream(std::ostream* err) { m_err = err; };
  std::ostream* GetOutStream() { return m_out; }
  void SetTclInterpreterHandler(TclInterpreterHandler* tclInterpreterHandler);
  void SetSession(Session* session) { m_session = session; }
  Session* GetSession() { return m_session; }
  virtual ~Compiler();

  void BatchScript(const std::string& script) { m_batchScript = script; }
  State CompilerState() { return m_state; }
  bool Compile(Action action);
  void Stop();
  TclInterpreter* TclInterp() { return m_interp; }
  Design* GetActiveDesign() const;
  Design* GetDesign(const std::string name);
  void SetDesign(Design* design);
  bool SetActiveDesign(const std::string& name);
  virtual bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  bool Clear();
  void start();
  void finish();

  std::string& getResult() { return m_result; }

  void setTaskManager(TaskManager* newTaskManager);
  Constraints* getConstraints() { return m_constraints; }
  void setGuiTclSync(TclCommandIntegration* tclCommands);
  virtual void Help(std::ostream* out);
  virtual void Version(std::ostream* out);
  virtual void Message(const std::string& message);
  virtual void ErrorMessage(const std::string& message);
  void SetUseVerific(bool on) { m_useVerific = on; }
  void SetHardError(bool on) { m_hardError = on; }
  void ChannelWidth(uint32_t width) { m_channel_width = width; }
  void LutSize(uint32_t size) { m_lut_size = size; }
  SynthesisOpt SynthOpt() { return m_synthOpt; }
  void SynthOpt(SynthesisOpt opt) { m_synthOpt = opt; }

 protected:
  /* Methods that can be customized for each new compiler flow */
  virtual bool IPGenerate();
  virtual bool Synthesize();
  virtual bool Packing();
  virtual bool GlobalPlacement();
  virtual bool Placement();
  virtual bool Route();
  virtual bool TimingAnalysis();
  virtual bool PowerAnalysis();
  virtual bool GenerateBitstream();

  bool CreateDesign(const std::string& name);

  /* Compiler class utilities */
  bool RunBatch();
  bool RunCompileTask(Action action);
  virtual bool ExecuteSystemCommand(const std::string& command);
  virtual int ExecuteAndMonitorSystemCommand(const std::string& command);
  std::string ReplaceAll(std::string_view str, std::string_view from,
                         std::string_view to);
  bool FileExists(const std::filesystem::path& name);
  void Tokenize(std::string_view str, std::string_view separator,
                std::vector<std::string>& result);
  /* Propected members */
  TclInterpreter* m_interp = nullptr;
  Session* m_session = nullptr;
  Design* m_design = nullptr;
  bool m_stop = false;
  State m_state = None;
  std::ostream* m_out = &std::cout;
  std::ostream* m_err = &std::cerr;
  std::string m_batchScript;
  std::string m_result;
  TclInterpreterHandler* m_tclInterpreterHandler{nullptr};
  TaskManager* m_taskManager{nullptr};
  std::vector<Design*> m_designs;
  TclCommandIntegration* m_tclCmdIntegration{nullptr};
  Constraints* m_constraints = nullptr;
  std::string m_output;
  bool m_useVerific = false;
  bool m_hardError = false;
  SynthesisOpt m_synthOpt = SynthesisOpt::NoOpt;
  uint32_t m_channel_width = 100;
  uint32_t m_lut_size = 6;
};

}  // namespace FOEDAG

#endif
