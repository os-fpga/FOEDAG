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

StreamBuffer::StreamBuffer(QObject *parent) : QObject{parent}, m_stream(this) {}

std::ostream &StreamBuffer::getStream() { return m_stream; }

int StreamBuffer::overflow(int c) {
  char_type ch = static_cast<char_type>(c);
  if (ch == '\n') emit ready(QString{ch});
  return ch;
}

std::streamsize StreamBuffer::xsputn(const char_type *s,
                                     std::streamsize count) {
  const QByteArray array = QByteArray::fromRawData(s, count);
  emit ready(QString{array});
  return count;
}

FileLoggerBuffer::FileLoggerBuffer(Logger *logger, std::streambuf *out)
    : m_logger(logger), m_stream(out) {}

int FileLoggerBuffer::overflow(int c) {
  char_type ch = static_cast<char_type>(c);
  if (ch == traits_type::eof()) return ch;
  m_stream.put(c);
  return c;
}

int FileLoggerBuffer::sync() {
  m_stream.flush();
  return 0;
}

std::streamsize FileLoggerBuffer::xsputn(const char_type *s,
                                         std::streamsize count) {
  m_logger->appendLog(std::string{s});
  return std::streambuf::xsputn(s, count);
}

}  // namespace FOEDAG
