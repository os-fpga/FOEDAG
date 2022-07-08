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

Version toVersion(const QString &s) {
  QStringList list = s.split(".");
  if (list.count() != 3) return Version{};
  return Version{list.at(0).toUInt(), list.at(1).toUInt(), list.at(2).toUInt()};
}

ProjectFileComponent::ProjectFileComponent(QObject *parent) : QObject(parent) {}

void ProjectFileComponent::Save(QXmlStreamWriter *writer) {
  writer->writeAttribute(VERSION, toString(m_version));
}

}  // namespace FOEDAG
