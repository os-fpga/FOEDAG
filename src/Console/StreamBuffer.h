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

class Logger;
class StreamBuffer : public QObject, public std::streambuf {
  Q_OBJECT
 public:
  explicit StreamBuffer(QObject *parent = nullptr);
  std::ostream &getStream();

 signals:
  void ready(const QString &);

 protected:
  int overflow(int c) override;
  std::streamsize xsputn(const char_type *s, std::streamsize count) override;

 private:
  std::ostream m_stream;
};

class FileLoggerBuffer : public std::streambuf {
 public:
  explicit FileLoggerBuffer(Logger *logger, std::streambuf *out);

 protected:
  int overflow(int c) override;
  int sync() override;
  std::streamsize xsputn(const char_type *s, std::streamsize count) override;

 private:
  Logger *m_logger{nullptr};
  std::ostream m_stream;
};

}  // namespace FOEDAG
