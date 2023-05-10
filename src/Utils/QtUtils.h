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

#include <QDir>
#include <QStringList>
#include <QTimer>
#include <string>

namespace FOEDAG {

class QtUtils {
 public:
  // no empty parts
  static QStringList StringSplit(const QString &str, const QChar &sep);

  // return true if str is equal to s with Qt::CaseInsensitive
  static bool IsEqual(const QString &str, const QString &s);

  static QString replaceTags(QString &data, const QStringList &tags);
  static std::string replaceTags(const std::string &data,
                                 const std::vector<std::string> &tags);

  template <class Functor>
  static void AppendToEventQueue(Functor functor) {
    QTimer::singleShot(1, functor);
  }

  // variadic tamplate to create path with any number of folders (files)
  template <class St, class... String>
  static St CreatePath(St s, String... args) {
    return s + QDir::separator() + CreatePath(args...);
  }

  template <class St>
  static St CreatePath(St s) {
    return s;
  }
};

}  // namespace FOEDAG
