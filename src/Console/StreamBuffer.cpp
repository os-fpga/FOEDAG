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

OutputBuffer::OutputBuffer(const HandlerFunction &bufferHandler)
    : m_stream(this), m_bufferHandler(bufferHandler) {}

std::ostream &OutputBuffer::getStream() { return m_stream; }

int OutputBuffer::overflow(int c) {
  char_type ch = static_cast<char_type>(c);
  if (ch == '\n') {
    auto data = std::string{ch};
    m_bufferHandler(data);
    m_stream << data;
  }
  return ch;
}

std::streamsize OutputBuffer::xsputn(const char_type *s,
                                     std::streamsize count) {
  auto data = std::string{s, static_cast<std::string::size_type>(count)};
  m_bufferHandler(data);
  m_stream << data;
  return count;
}

}  // namespace FOEDAG
