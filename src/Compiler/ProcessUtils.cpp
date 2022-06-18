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

#include <string>
#if (defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__))
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

void process_mem_usage(int64_t processId, const char *proccessIdStr,
                       double &vm_usage /*in kiB*/) {
#if (defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__))
  PROCESS_MEMORY_COUNTERS_EX pmc;
  auto p = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
  GetProcessMemoryInfo(p, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
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
  m_thread->join();
  cleanup();
  m_max_utiliation = static_cast<uint>(m_vm);
}

void ProcessUtils::cleanup() {
  delete m_thread;
  m_thread = nullptr;
}

}  // namespace FOEDAG
