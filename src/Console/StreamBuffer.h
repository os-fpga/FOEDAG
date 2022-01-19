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

class StreamBuffer : public QObject, public std::streambuf {
  Q_OBJECT
 public:
  explicit StreamBuffer(QObject *parent = nullptr);
  std::ostream &getStream();

 signals:
  void ready(const QString &);

 protected:
  int overflow(int c) override;
  int uflow() override;
  int underflow() override;

 private:
  int m_last;
  std::ostream m_stream;
  char m_buffer[100];
  int m_buffer_index{0};
};
