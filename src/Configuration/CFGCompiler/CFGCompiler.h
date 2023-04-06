#ifndef CFGCompiler_H
#define CFGCompiler_H

#include <chrono>
#include <map>
#include <string>

#include "Configuration/CFGCommon/CFGCommon.h"

typedef void (*cfg_callback_function)(const CFGCommon_ARG* cmdarg);
typedef std::map<std::string, cfg_callback_function> cfg_callback_function_map;

namespace FOEDAG {

class TclInterpreter;
class Compiler;

class CFGCompiler {
 public:
  CFGCompiler(Compiler* compiler);
  virtual ~CFGCompiler() {}
  Compiler* GetCompiler() const;
  bool RegisterCommands(TclInterpreter* interp, bool batchMode);
  void RegisterCallbackFunction(std::string name,
                                cfg_callback_function function);
  bool Configure();

 public:
  static int Compile(CFGCompiler* cfgcompiler, bool batchMode);
  static void Message(const std::string& message, const bool raw);
  static void ErrorMessage(const std::string& message, bool append);
  static int ExecuteAndMonitorSystemCommand(const std::string& command,
                                            const std::string logFile,
                                            bool appendLog);

 public:
  CFGCommon_ARG m_cmdarg;

 protected:
  Compiler* m_compiler;
  cfg_callback_function_map m_callback_function_map;
};

}  // namespace FOEDAG

#endif