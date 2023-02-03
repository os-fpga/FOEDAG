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
#include "MessageItemParser.h"

#include <QRegularExpression>

namespace FOEDAG {

MessageItemParser::~MessageItemParser() {}

std::pair<bool, FileInfo> VerificParser::parse(const QString &text) const {
  static const QRegularExpression warn{"(?<=VERIFIC-WARNING )(.*)(?=:(.*):)"};
  static const QRegularExpression err{"(?<=VERIFIC-ERROR )(.*)(?=:(.*):)"};

  for (const auto &[exp, level] :
       VectorRegLevel{{warn, Level::Warning}, {err, Level::Error}}) {
    auto match = exp.match(text);
    if (match.hasMatch()) {
      auto fileName = match.captured(0);
      auto line = match.captured(2).toInt();
      fileName = fileName.mid(fileName.indexOf(']') + 2);
      return std::make_pair(true, FileInfo{fileName, line, level});
    }
  }
  return std::make_pair(false, FileInfo{});
}

std::pair<bool, FileInfo> TimingAnalysisParser::parse(
    const QString &text) const {
  static const QRegularExpression warn{"(?<=Warning: )(.*)(?= line (.*), )"};
  static const QRegularExpression critical{
      "(?<=Critical: )(.*)(?= line (.*), )"};

  for (const auto &[exp, level] :
       VectorRegLevel{{warn, Level::Warning}, {critical, Level::Error}}) {
    auto match = exp.match(text);
    if (match.hasMatch()) {
      auto fileName = match.captured(0);
      auto line = match.captured(2).toInt();
      return std::make_pair(true, FileInfo{fileName, line, level});
    }
  }
  return std::make_pair(false, FileInfo{});
}

}  // namespace FOEDAG
