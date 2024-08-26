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
#include "QtUtils.h"

namespace FOEDAG {

QStringList QtUtils::StringSplit(const QString &str, const QChar &sep) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  return str.split(sep, Qt::SkipEmptyParts);
#else
  return str.split(sep, QString::SkipEmptyParts);
#endif
}

bool QtUtils::IsEqual(const QString &str, const QString &s) {
  return str.compare(s, Qt::CaseInsensitive) == 0;
}

QSet<QString> QtUtils::convertToSet(const QList<QString>& l) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
  return QSet<QString>{l.begin(), l.end()};
#else
  return QSet<QString>::fromList(l);
#endif
}

}  // namespace FOEDAG
