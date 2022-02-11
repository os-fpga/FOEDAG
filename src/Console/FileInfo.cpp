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
#include "FileInfo.h"

#include <QDir>

FileInfo::FileInfo() {}

QString FileInfo::getRefString(const QString &filePath) {
  const QFileInfo fileInfo {filePath};
  return QString::fromLatin1("<a href=\"%1\">%2</a>").arg(filePath, fileInfo.fileName());
}

QStringList FileInfo::getFileList(const QString &path,
                                  const QStringList &filter) {
  QDir dir{QDir::toNativeSeparators(path)};
  auto fileInfo = dir.entryInfoList({"*.tcl"}, QDir::Files);
  QStringList files;
  for (const auto &fInfo : fileInfo) files.append(fInfo.fileName());
  return files;
}

QChar FileInfo::separator() { return QDir::separator(); }
