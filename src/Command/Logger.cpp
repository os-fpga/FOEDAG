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

#include "Command/Logger.h"

using namespace FOEDAG;

Logger::Logger(const std::string& filePath) {
  m_fileName = filePath;
  m_stream = new std::ofstream(filePath, std::fstream::out);
}

void Logger::open() {
  if (m_stream == nullptr) {
    m_stream = new std::ofstream(m_fileName, std::fstream::app);
  }
}

void Logger::close() {
  if (m_stream) {
    delete m_stream;
    m_stream = nullptr;
  }
}

void Logger::log(const std::string& text) {
  if (m_stream) {
    *m_stream << text << std::endl << std::flush;
  }
}

void Logger::appendLog(const std::string& text) {
  if (m_stream) {
    *m_stream << text << std::flush;
  }
}

Logger::~Logger() { close(); }

Logger& Logger::operator<<(const std::string& log) {
  appendLog(log);
  return *this;
}
