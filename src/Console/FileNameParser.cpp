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
#include "FileNameParser.h"

#include <QFileInfo>
#include <QRegularExpression>

namespace FOEDAG {

LineParser::Result FileNameParser::handleLine(const QString &message,
                                              OutputFormat format) {
  // use static to fix use-static-qregularexpression clazy warning
  static const QRegularExpression fileWithLine{
      "(\\S+[^\\.]\\.[a-zA-Z]+)(?:[:]|[(]|\\s)(\\d+)"};

  auto regExpMatch = fileWithLine.match(message);
  if (regExpMatch.hasMatch()) {
    const int cap{1};
    QString file = regExpMatch.captured(cap);
    file = file.trimmed();
    const QFileInfo fileInfo{file};
    const QString filePath =
        fileInfo.exists() ? fileInfo.absoluteFilePath() : file;
    const QString line = regExpMatch.captured(2);
    LinkSpec link{regExpMatch.capturedStart(cap),
                  regExpMatch.capturedLength(cap),
                  addLinkSpecForAbsoluteFilePath(filePath, line)};
    return Result{Status::Done, message, {link}};
  }

  // use static to fix use-static-qregularexpression clazy warning
  static const QRegularExpression all{
      "((?:[\\.\\/\\[a-zA-Z]+)[\\/\\:]+\\S+[^\\.]\\.[a-zA-Z]+)"};
  regExpMatch = all.match(message);
  if (regExpMatch.hasMatch()) {
    QString file = regExpMatch.captured(1);
    file = file.trimmed();
    const QFileInfo fileInfo{file};
    const QString filePath =
        fileInfo.exists() ? fileInfo.absoluteFilePath() : file;
    const QString line = "-1";
    LinkSpec link{regExpMatch.capturedStart(1), regExpMatch.capturedLength(1),
                  addLinkSpecForAbsoluteFilePath(filePath, line)};
    return Result{Status::Done, message, {link}};
  }
  return Result{Status::NotHandled};
}

}  // namespace FOEDAG
