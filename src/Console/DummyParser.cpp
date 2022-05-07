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
#include "DummyParser.h"

#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>

namespace FOEDAG {

DummyParser::DummyParser() {}

LineParser::Result DummyParser::handleLine(const QString &message,
                                           OutputFormat format) {
  Q_UNUSED(format);
  const QRegularExpression getFile{"(?<=File: )(.*)(?= just)"};
  auto regExpMatch = getFile.match(message);
  if (regExpMatch.hasMatch()) {
    QString file = regExpMatch.captured();
    file.replace("\"", "");
    file = file.trimmed();
    const QFileInfo fileInfo{file};
    LinkSpec link{
        regExpMatch.capturedStart(), regExpMatch.capturedLength(),
        addLinkSpecForAbsoluteFilePath(fileInfo.absoluteFilePath(), "-1")};
    return Result{Status::Done, message, {link}};
  }
  return Result{Status::NotHandled};
}
}  // namespace FOEDAG
