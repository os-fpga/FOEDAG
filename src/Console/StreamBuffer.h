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
#pragma once

#include <QObject>
#include <iostream>
#include <streambuf>

namespace FOEDAG {

class StreamBuffer : public std::streambuf {
 public:
  explicit StreamBuffer();
  std::ostream &getStream();

  virtual void output(const char_type *s, std::streamsize count) {}

 protected:
  int overflow(int c) override;
  std::streamsize xsputn(const char_type *s, std::streamsize count) override;

 protected:
  std::ostream m_stream;
};

class Logger;
class BatchModeBuffer : public StreamBuffer {
 public:
  explicit BatchModeBuffer(Logger *logger);
  void output(const char_type *s, std::streamsize count) override;

 private:
  Logger *m_logger{};
};

class TclConsoleBuffer : public QObject, public StreamBuffer {
  Q_OBJECT
 public:
  TclConsoleBuffer(QObject *parent = nullptr);
  void output(const char_type *s, std::streamsize count) override;

 signals:
  void ready(const QString &str);
};

}  // namespace FOEDAG
