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

#include <QDebug>

namespace FOEDAG {

ProcessUtils::~ProcessUtils() { cleanup(); }

uint ProcessUtils::Utilization() const { return m_max_utiliation; }

void ProcessUtils::Frequency(uint p) { m_frequency = p; }

void ProcessUtils::Start(qint64 processId) {
  auto start = [processId, this]() {
    m_process = new QProcess;
    m_process->setProgram(Program());
    m_process->setArguments(Arguments(processId));
    QObject::connect(m_process, &QProcess::readyReadStandardOutput, [this]() {
      if (!m_stop) HandleOutput(m_process->readAllStandardOutput());
    });
    QObject::connect(m_process,
                     qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                     [&]() {
                       if (!m_stop) {
                         std::chrono::milliseconds dura(m_frequency);
                         std::this_thread::sleep_for(dura);
                         m_process->start();
                         m_process->waitForFinished();
                       }
                     });
    m_process->start();
    m_process->waitForFinished();
  };
  m_thread = new std::thread{start};
}

void ProcessUtils::Stop() {
  m_stop = true;
  m_process->terminate();
  m_thread->join();
  cleanup();
}

void ProcessUtils::HandleOutput(const QString &output) {
#if (defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__))
  // TODO: Windows
#else
  auto splitted = output.split(" ");
  if (splitted.count() > 23) {
    m_max_utiliation = std::max(m_max_utiliation, splitted.at(23).toUInt());
  }
#endif
}

QString ProcessUtils::Program() const {
#if (defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__))
  // TODO: Windows
  return QString();
#else
  return QString("cat");
#endif
}

QStringList ProcessUtils::Arguments(int processId) const {
#if (defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__))
  // TODO: Windows
  return QString();
#else
  return {QString("/proc/%1/stat").arg(processId)};
#endif
}

void ProcessUtils::cleanup() {
  delete m_process;
  m_process = nullptr;
  delete m_thread;
  m_thread = nullptr;
}

}  // namespace FOEDAG
