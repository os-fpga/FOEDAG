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

#include <time.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "MainWindow/Session.h"

extern FOEDAG::Session* GlobalSession;

namespace FOEDAG {

std::string dateTimeToString(const tm& t, const char* format);
tm now();

template <class Logger, class T>
void log(Logger* logger, const T& msg) {
  if (logger) logger->log(msg);
}

template <class Logger>
void log(Logger* logger, const QString& msg) {
  if (logger) logger->log(msg.toStdString());
}

template <class Logger, class T>
void logAppend(Logger* logger, const T& msg) {
  if (logger) logger->appendLog(msg);
}

template <class Logger>
void logAppend(Logger* logger, const QString& msg) {
  if (logger) logger->appendLog(msg.toStdString());
}

// using PERF_LOGGER() << "some message";
#define PERF_LOGGER() (*GlobalSession->CmdStack()->PerfLogger())

// using PERF_LOG("Some message");
#define PERF_LOG(msg)                                                    \
  if (GlobalSession->CmdStack()->PerfLogger()) {                         \
    std::string t = FOEDAG::dateTimeToString(FOEDAG::now(), "%H:%M:%S"); \
    PERF_LOGGER() << "[ " << t << " ] " << msg << "\n";                  \
  }

// write log into output log file
#define LOG_OUTPUT(out) logAppend(GlobalSession->CmdStack()->OutLogger(), out)

// write cmd into replay command file
#define LOG_CMD(cmd) log(GlobalSession->CmdStack()->CmdLogger(), cmd);

}  // namespace FOEDAG
