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
#include "ProcessUtils.h"

#include <QProcess>
#include <filesystem>
#include <string>

#include "Compiler/Log.h"
#include "QtUtils.h"

#if (defined(_MSC_VER) || defined(__CYGWIN__))
#define NOMINMAX  // prevent error with std::max
#include <windows.h>
// include order metters
#include <psapi.h>
#else
#include <fstream>
#endif

namespace FOEDAG {

ProcessUtils::~ProcessUtils() { cleanup(); }

ProcessUtils::uint ProcessUtils::Utilization() const {
  return m_max_utiliation;
}

void ProcessUtils::Frequency(uint p) { m_frequency = p; }

void process_mem_usage(int64_t processId, const char* proccessIdStr,
                       double& vm_usage /*in kiB*/) {
#if (defined(_MSC_VER) || defined(__CYGWIN__))
  PROCESS_MEMORY_COUNTERS_EX pmc;
  auto p = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
  GetProcessMemoryInfo(p, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
  vm_usage = pmc.PrivateUsage / 1024.0;
#else
  std::ifstream stat_stream(proccessIdStr, std::ios_base::in);

  // dummy vars for leading entries in stat that we don't care about
  std::string pid, comm, state, ppid, pgrp, session, tty_nr;
  std::string tpgid, flags, minflt, cminflt, majflt, cmajflt;
  std::string utime, stime, cutime, cstime, priority, nice;
  std::string O, itrealvalue, starttime;

  unsigned long vsize;

  stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr >>
      tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt >> utime >>
      stime >> cutime >> cstime >> priority >> nice >> O >> itrealvalue >>
      starttime >> vsize;  // don't care about the rest

  stat_stream.close();
  vm_usage = vsize / 1024.0;
#endif
}

void ProcessUtils::Start(int64_t processId) {
  auto start = [processId, this]() {
    auto str = ("/proc/" + std::to_string(processId) + "/stat");
    while (!m_stop) {
      double vm;
      process_mem_usage(processId, str.c_str(), vm);
      m_vm = std::max(m_vm, vm);

      std::chrono::milliseconds dura(m_frequency);
      std::this_thread::sleep_for(dura);
    }
  };
  m_thread = new std::thread{start};
}

void ProcessUtils::Stop() {
  m_stop = true;
  if (m_thread) m_thread->join();
  cleanup();
  m_max_utiliation = static_cast<uint>(m_vm);
}

void ProcessUtils::cleanup() {
  delete m_thread;
  m_thread = nullptr;
}

int ExecuteAndMonitorSystemCommand(const std::string& command,
                                   const std::string& projectPath,
                                   std::ostream* std_out, std::ostream* std_err,
                                   const std::string& logFile, bool appendLog) {
  using Time = std::chrono::high_resolution_clock;
  using ms = std::chrono::milliseconds;
  auto start = Time::now();
  PERF_LOG("Command: " + command);
  (*std_out) << "Command: " << command << std::endl;
  std::error_code ec;
  auto path = std::filesystem::current_path();  // getting path
  std::filesystem::current_path(projectPath,
                                ec);  // setting path
  // new QProcess must be created here to avoid issues related to creating
  // QObjects in different threads
  auto m_process = new QProcess;
  QStringList env = QProcess::systemEnvironment();
  std::map<std::string, std::string> m_environmentVariableMap;
  if (!m_environmentVariableMap.empty()) {
    for (std::map<std::string, std::string>::iterator itr =
             m_environmentVariableMap.begin();
         itr != m_environmentVariableMap.end(); itr++) {
      env << strdup(((*itr).first + "=" + (*itr).second).c_str());
    }
  }
  m_process->setEnvironment(env);
  std::ofstream ofs;
  if (!logFile.empty()) {
    std::ios_base::openmode openMode{std::ios_base::out};
    if (appendLog) openMode = std::ios_base::out | std::ios_base::app;
    ofs.open(logFile, openMode);
    QObject::connect(m_process, &QProcess::readyReadStandardOutput,
                     [&ofs, &m_process, &std_out]() {
                       qint64 bytes = m_process->bytesAvailable();
                       QByteArray bufout = m_process->readAllStandardOutput();
                       ofs.write(bufout, bytes);
                       std_out->write(bufout, bytes);
                     });
    QObject::connect(m_process, &QProcess::readyReadStandardError,
                     [&ofs, &m_process, &std_err]() {
                       QByteArray data = m_process->readAllStandardError();
                       int bytes = data.size();
                       ofs.write(data, bytes);
                       std_err->write(data, bytes);
                     });
  } else {
    QObject::connect(m_process, &QProcess::readyReadStandardOutput,
                     [&std_out, &m_process]() {
                       std_out->write(m_process->readAllStandardOutput(),
                                      m_process->bytesAvailable());
                     });
    QObject::connect(m_process, &QProcess::readyReadStandardError,
                     [&std_err, &m_process]() {
                       QByteArray data = m_process->readAllStandardError();
                       std_err->write(data, data.size());
                     });
  }
  ProcessUtils utils;
  QObject::connect(m_process, &QProcess::started, [&m_process, &utils]() {
    utils.Start(m_process->processId());
  });

  QString cmd{command.c_str()};
  QStringList args = QtUtils::StringSplit(cmd, ' ');
  QStringList adjustedArgs;

  QString program = args.first();
  args.pop_front();  // remove program
  QString current_arg;
  for (int i = 0; i < args.size(); i++) {
    QString arg = args[i];
    if (args[i].front() == '\"' &&
        args[i].back() != '\"') {      // Starting single-quote
      current_arg = arg.remove(0, 1);  // remove leading quote
    } else if (args[i].front() != '\"' &&
               args[i].back() == '\"') {    // Ending single-quote
      current_arg += " " + arg.chopped(1);  // remove trailing quote
      adjustedArgs.push_back(current_arg);
      current_arg = "";
    } else if (args[i].front() == '\"' &&
               args[i].back() == '\"') {  // Single-quoted argument
      current_arg += " " + arg;
      adjustedArgs.push_back(current_arg);
      current_arg = "";
    } else if (!current_arg.isEmpty()) {  // Continuing single-quoted argument
      current_arg += " " + arg;
    } else {  // Non-quoted argument
      adjustedArgs.push_back(arg);
    }
  }

  m_process->start(program, adjustedArgs);
  std::filesystem::current_path(path);
  m_process->waitForFinished(-1);
  utils.Stop();
  // DEBUG: (*std_out) << "Changed path to: " << (path).string() << std::endl;
  uint max_utiliation{utils.Utilization()};
  auto status = m_process->exitStatus();
  auto exitCode = m_process->exitCode();
  delete m_process;
  m_process = nullptr;
  if (!logFile.empty()) {
    ofs.close();
  }
  auto end = Time::now();
  auto fs = end - start;
  ms d = std::chrono::duration_cast<ms>(fs);
  std::stringstream stream;
  stream << "Duration: " << d.count() << " ms. Max utilization: ";
  if (max_utiliation <= 1024) {
    stream << max_utiliation << " kiB";
  } else {
    stream << max_utiliation / 1024 << " MB";
  }
  PERF_LOG(stream.str());
  return (status == QProcess::NormalExit) ? exitCode : -1;
}

}  // namespace FOEDAG
