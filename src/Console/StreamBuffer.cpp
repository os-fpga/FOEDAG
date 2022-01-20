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

StreamBuffer::StreamBuffer(QObject *parent)
    : QObject{parent}, m_last{traits_type::eof()}, m_stream(this) {}

std::ostream &StreamBuffer::getStream() { return m_stream; }

int StreamBuffer::overflow(int c) {
  if (c == traits_type::eof()) {
    return traits_type::eof();
  }

  char_type ch = static_cast<char_type>(c);
  m_buffer.append(ch);
  if (ch == 10) {
    emit ready(m_buffer);
    m_buffer.clear();
  }

  return ch;
}

int StreamBuffer::uflow() {
  const int c = underflow();
  m_last = traits_type::eof();
  return c;
}

int StreamBuffer::underflow() { return m_last; }
