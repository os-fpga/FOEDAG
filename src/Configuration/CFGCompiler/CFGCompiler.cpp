/*
Copyright 2023 The Foedag team
GPL License
Copyright (c) 2023 The Open-Source FPGA Foundation
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
#include "CFGCompiler.h"

#include <filesystem>
#include <memory>

#include "Compiler/Log.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Configuration/CFGCommon/CFGArg_auto.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Programmer/Programmer.h"

using namespace FOEDAG;

static const CFGCompiler* m_CFGCompiler = nullptr;

static bool programmer_flow(CFGCompiler* cfgcompiler, int argc,
                            const char* argv[]) {
  // Do some customize flow to check the arguments
  // Each command has different checking rule
  // Set this argument in CFGCompiler
  // return if there is an error
  bool status{true};
  std::vector<std::string> errors;
  cfgcompiler->m_cmdarg.command = "programmer";
  auto compiler = cfgcompiler->GetCompiler();

  cfgcompiler->m_cmdarg.compilerName = compiler->Name();
  auto arg = std::make_shared<CFGArg_PROGRAMMER>();
  // drop the first executable arg,
  // create pointer to the second element
  // the parser expect the first arg is the command name
  const char** argvPtr = &argv[1];
  status = arg->parse(argc - 1, argvPtr, &errors);
  cfgcompiler->m_cmdarg.arg = arg;
  cfgcompiler->m_cmdarg.toolPath = compiler->GetProgrammerToolExecPath();
  cfgcompiler->m_cmdarg.searchPath = compiler->GetConfigFileSearchDirectory();
  return status;
}

CFGCompiler::CFGCompiler(Compiler* compiler) : m_compiler(compiler) {
  m_CFGCompiler = this;
  CFG_set_callback_message_function(Message, ErrorMessage,
                                    ExecuteAndMonitorSystemCommand);
}

Compiler* CFGCompiler::GetCompiler() const { return m_compiler; }

bool CFGCompiler::RegisterCommands(TclInterpreter* interp, bool batchMode) {
  bool status = true;
  if (batchMode) {
    auto programmer = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      CFGCompiler* cfgcompiler = (CFGCompiler*)clientData;
      if (programmer_flow(cfgcompiler, argc, argv)) {
        return CFGCompiler::Compile(cfgcompiler, true);
      } else {
        return TCL_ERROR;
      }
    };
    interp->registerCmd("programmer", programmer, this, 0);
  } else {
    auto programmer = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
      CFGCompiler* cfgcompiler = (CFGCompiler*)clientData;
      if (programmer_flow(cfgcompiler, argc, argv)) {
        return CFGCompiler::Compile(cfgcompiler, false);
      } else {
        return TCL_ERROR;
      }
    };
    interp->registerCmd("programmer", programmer, this, 0);
  }
  status = RegisterCallbackFunction("programmer", programmer_entry);
  return status;
}

bool CFGCompiler::RegisterCallbackFunction(std::string name,
                                           cfg_callback_function function) {
  bool status = false;
  if (name.size() > 0 && function != nullptr) {
    if (m_callback_function_map.find(name) == m_callback_function_map.end()) {
      m_callback_function_map[name] = function;
      status = true;
    } else if (m_callback_function_map[name] == function) {
      status = true;
    }
  }
  return status;
}

int CFGCompiler::Compile(CFGCompiler* cfgcompiler, bool batchMode) {
  // Set generic information that every command might need
  Compiler* compiler = cfgcompiler->GetCompiler();
  cfgcompiler->m_cmdarg.projectName = compiler->ProjManager()->projectName();
  cfgcompiler->m_cmdarg.device = compiler->ProjManager()->getTargetDevice();
  cfgcompiler->m_cmdarg.projectPath = compiler->ProjManager()->projectPath();
  cfgcompiler->m_cmdarg.taskPath =
      compiler->FilePath(Compiler::Action::Bitstream).string();
  cfgcompiler->m_cmdarg.analyzePath =
      compiler->FilePath(Compiler::Action::Analyze).string();
  cfgcompiler->m_cmdarg.synthesisPath =
      compiler->FilePath(Compiler::Action::Synthesis).string();
  cfgcompiler->m_cmdarg.searchPath = compiler->GetConfigFileSearchDirectory();
  cfgcompiler->m_cmdarg.binPath = compiler->GetBinPath().string();

  // Call Compile()
  if (batchMode) {
    if (!compiler->Compile(Compiler::Action::Configuration)) {
      return TCL_ERROR;
    }
  } else {
    WorkerThread* wthread = new WorkerThread(
        "configuration_th", Compiler::Action::Configuration, compiler);
    if (!wthread->start()) {
      return TCL_ERROR;
    }
  }
  return TCL_OK;
}

void CFGCompiler::Message(const std::string& message, const bool raw) {
  if (m_CFGCompiler != nullptr && m_CFGCompiler->GetCompiler() != nullptr) {
    m_CFGCompiler->GetCompiler()->Message(message, "", raw);
  } else {
    if (raw) {
      printf("%s", message.c_str());
    } else {
      printf("INFO: %s\n", message.c_str());
    }
    fflush(stdout);
  }
}

void CFGCompiler::ErrorMessage(const std::string& message, bool append) {
  if (m_CFGCompiler != nullptr && m_CFGCompiler->GetCompiler() != nullptr) {
    m_CFGCompiler->GetCompiler()->ErrorMessage(message, append);
  } else {
    printf("ERROR: %s\n", message.c_str());
    fflush(stdout);
  }
}

int CFGCompiler::ExecuteAndMonitorSystemCommand(const std::string& command,
                                                const std::string logFile,
                                                bool appendLog) {
  if (m_CFGCompiler != nullptr && m_CFGCompiler->GetCompiler() != nullptr) {
    return m_CFGCompiler->GetCompiler()->ExecuteAndMonitorSystemCommand(
        command, logFile, appendLog);
  } else {
    std::string output = "";
    std::atomic<bool> stop = false;
    return CFG_execute_cmd(command, output, nullptr, stop);
  }
}

/*
  This is single point entry to all configuration functions

  We use try-and-catch to prevent GUI crash

  We do not need to go to each function to make such change
*/
bool CFGCompiler::Configure() {
  bool status = false;
  if (m_callback_function_map.find(m_cmdarg.command) !=
      m_callback_function_map.end()) {
    try {
      m_callback_function_map[m_cmdarg.command](&m_cmdarg);
      status = true;
    } catch (std::exception& e) {
      // CFG assertion APIs already print the exception in error message
      //   format. Do not need to reprint the error message
      // Just print new error which command failed to execute
      CFG_POST_ERR("Fail to execute command: %s", m_cmdarg.command.c_str());
    }
  }
  return status;
}
