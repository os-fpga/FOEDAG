/*
Copyright 2021-2022 The Foedag team

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
#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace FOEDAG {
class TclInterpreter;
class TclInterpreterHandler;
class Session;
class Compiler;

class Simulator {
 public:
  enum class SimulatorType { Verilator, Icarus, VCS, Questa, Xcelium };
  enum class SimulationType { RTL, Gate, PNR, Bitstream };

  // Most common use case, create the compiler in your main
  Simulator() = default;
  Simulator(TclInterpreter* interp, Compiler* compiler, std::ostream* out,
            TclInterpreterHandler* tclInterpreterHandler = nullptr);
  void SetInterpreter(TclInterpreter* interp) { m_interp = interp; }
  void SetOutStream(std::ostream* out) { m_out = out; };
  void SetErrStream(std::ostream* err) { m_err = err; };
  std::ostream* GetOutStream() { return m_out; }
  void SetTclInterpreterHandler(TclInterpreterHandler* tclInterpreterHandler);
  void SetSession(Session* session) { m_session = session; }
  Session* GetSession() const { return m_session; }
  virtual ~Simulator();

  bool Simulate(SimulationType action, SimulatorType type);
  void Stop();
  TclInterpreter* TclInterp() { return m_interp; }
  virtual bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  bool Clear();
  void start();
  void finish();

  std::string& getResult() { return m_result; }

  virtual void Help(std::ostream* out);
  virtual void Version(std::ostream* out);
  virtual void Message(const std::string& message);
  virtual void ErrorMessage(const std::string& message);

 protected:
  virtual bool SimulateRTL(SimulatorType type);
  virtual bool SimulateGate(SimulatorType type);
  virtual bool SimulatePNR(SimulatorType type);
  virtual bool SimulateBitstream(SimulatorType type);

  virtual std::string SimulatorName(SimulatorType type);
  virtual std::string IncludeDirective(SimulatorType type);
  virtual std::string LibraryPathDirective(SimulatorType type);
  virtual std::string LibraryFileDirective(SimulatorType type);
  virtual std::string LibraryExtDirective(SimulatorType type);
  virtual std::string MacroDirective(SimulatorType type);

  class ProjectManager* ProjManager() const;
  std::string FileList(SimulationType action);
  /* Propected members */
  TclInterpreter* m_interp = nullptr;
  Compiler* m_compiler = nullptr;
  Session* m_session = nullptr;
  bool m_stop = false;
  std::ostream* m_out = &std::cout;
  std::ostream* m_err = &std::cerr;
  std::string m_result;
  TclInterpreterHandler* m_tclInterpreterHandler{nullptr};

  std::string m_output;
};

}  // namespace FOEDAG

#endif
