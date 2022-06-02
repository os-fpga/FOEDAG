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
#pragma once

#include <QProcess>
#include <QString>
#include <thread>

namespace FOEDAG {

class ProcessUtils {
 public:
  ProcessUtils() = default;
  ~ProcessUtils();

  /*!
   * \brief Utilization
   * \return max usage of virtual memory in kiB.
   */
  uint Utilization() const;

  /*!
   * \brief Period sets the frequency of measurment
   */
  void Frequency(uint p);
  void Start(qint64 processId);
  void Stop();

 private:
  void HandleOutput(const QString &output);
  QString Program() const;
  QStringList Arguments(int processId) const;
  void cleanup();

  uint m_max_utiliation{0};
  uint m_frequency{500};
  QProcess *m_process{nullptr};
  bool m_stop{false};
  std::thread *m_thread{nullptr};
};

}  // namespace FOEDAG
