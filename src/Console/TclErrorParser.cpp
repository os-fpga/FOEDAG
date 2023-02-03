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
#include "TclErrorParser.h"

#include <QFileInfo>
#include <QRegularExpression>

namespace FOEDAG {

TclErrorParser::TclErrorParser() {}

LineParser::Result TclErrorParser::handleLine(const QString &message,
                                              OutputFormat format) {
  Q_UNUSED(format);
  // use static to fix use-static-qregularexpression clazy warning
  static const QRegularExpression getFile{"(?<=file \")(.*)(?=\" line*)"};
  static const QRegularExpression getLine{"(?<=line )(\\d+)"};
  auto regExpMatch = getFile.match(message);
  auto lineMatch = getLine.match(message);
  if (regExpMatch.hasMatch() && lineMatch.hasMatch()) {
    QString file = regExpMatch.captured();
    file.replace("\"", "");
    file = file.trimmed();
    const QFileInfo fileInfo{file};
    const QString line = lineMatch.captured();
    LinkSpec link{
        regExpMatch.capturedStart(), regExpMatch.capturedLength(),
        addLinkSpecForAbsoluteFilePath(fileInfo.absoluteFilePath(), line)};
    return Result{Status::Done, message, {link}};
  }
  return Result{Status::NotHandled};
}
}  // namespace FOEDAG
