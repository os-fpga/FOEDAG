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

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || \
    defined(_MSC_VER) || defined(__CYGWIN__)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <direct.h>
#include <process.h>
#ifndef __SIZEOF_INT__
#define __SIZEOF_INT__ sizeof(int)
#endif
#include <psapi.h>
#else
#include <stdlib.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <thread>

namespace FOEDAG {

class ProcessUtils {
 public:
  ProcessUtils() = default;
  ~ProcessUtils();
  using uint = unsigned int;

  /*!
   * \brief Utilization
   * \return max usage of virtual memory in kiB.
   */
  uint Utilization() const;

  /*!
   * \brief Period sets the frequency of measurment
   */
  void Frequency(uint p);
  void Start(int64_t processId);
  void Stop();

 private:
  void cleanup();

  uint m_max_utiliation{0};
  uint m_frequency{10};
  bool m_stop{false};
  std::thread *m_thread{nullptr};
  double m_vm{0};
};

}  // namespace FOEDAG
