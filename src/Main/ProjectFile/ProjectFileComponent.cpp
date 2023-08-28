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
#include "ProjectFileComponent.h"

namespace FOEDAG {

QString toString(Version version) {
  return QString("%1.%2.%3")
      .arg(QString::number(version.maj), QString::number(version.min),
           QString::number(version.patch));
}

Version toVersion(const QString &s, bool *ok) {
  if (ok) *ok = true;
  QStringList list = s.split(".");
  if (list.count() != 3) {
    if (ok) *ok = false;
    return Version{};
  }
  Version version{};
  version.maj = list.at(0).toUInt(ok);
  if (ok && !(*ok)) return Version{};
  version.min = list.at(1).toUInt(ok);
  if (ok && !(*ok)) return Version{};
  version.patch = list.at(2).toUInt(ok);
  if (ok && !(*ok)) return Version{};
  return version;
}

ProjectFileComponent::ProjectFileComponent(QObject *parent) : QObject(parent) {}

void ProjectFileComponent::Save(QXmlStreamWriter *writer) {
  writer->writeAttribute(VERSION, toString(m_version));
}

bool operator!=(const Version &v1, const Version &v2) {
  return (v1.maj != v2.maj) || (v1.min != v2.min) || (v1.patch != v2.patch);
}

bool operator==(const Version &v1, const Version &v2) { return !(v1 != v2); }

bool operator<(const Version &v1, const Version &v2) {
  if (v1.maj < v2.maj)
    return true;
  else if (v1.maj == v2.maj) {
    if (v1.min < v2.min)
      return true;
    else if (v1.min == v2.min)
      return (v1.patch < v2.patch);
  }
  return false;
}

}  // namespace FOEDAG
