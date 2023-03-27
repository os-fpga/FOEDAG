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
#include "StreamBuffer.h"

#include "Command/Logger.h"

namespace FOEDAG {

StreamBuffer::StreamBuffer() : m_stream(this) {}

std::ostream &StreamBuffer::getStream() { return m_stream; }

int StreamBuffer::overflow(int c) {
  char_type ch = static_cast<char_type>(c);
  if (ch == '\n') output(&ch, 1);
  return ch;
}

std::streamsize StreamBuffer::xsputn(const char_type *s,
                                     std::streamsize count) {
  output(s, count);
  return count;
}

BatchModeBuffer::BatchModeBuffer(Logger *logger) : m_logger(logger) {}

void BatchModeBuffer::output(const char_type *s, std::streamsize count) {
  std::string str{s, static_cast<std::string::size_type>(count)};
  m_logger->appendLog(str);
  m_stream << str;
}

TclConsoleBuffer::TclConsoleBuffer(QObject *parent) : QObject(parent) {}

void TclConsoleBuffer::output(const char_type *s, std::streamsize count) {
  QByteArray array = QByteArray::fromRawData(s, count);
  emit ready(array);
}

}  // namespace FOEDAG
