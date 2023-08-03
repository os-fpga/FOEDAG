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
#include <sys/types.h>

#include <QObject>
#include <QXmlStreamWriter>

namespace FOEDAG {

struct Version {
  uint maj{0};
  uint min{0};
  uint patch{0};
  constexpr Version(uint mj, uint mn, uint p) : maj(mj), min(mn), patch(p) {}
  constexpr Version() = default;
};

QString toString(Version version);
Version toVersion(const QString &s);

constexpr auto VERSION = "Version";

struct ErrorCode {
  bool hasError{false};
  QString message{};
  explicit operator bool() const { return hasError; }
};

class ProjectFileComponent : public QObject {
  Q_OBJECT
 public:
  explicit ProjectFileComponent(QObject *parent = nullptr);
  virtual ~ProjectFileComponent() = default;
  virtual void Save(QXmlStreamWriter *writer);
  virtual ErrorCode Load(QXmlStreamReader *reader) { return {}; }
  virtual void LoadDone() {}

 signals:
  void saveFile();

 protected:
  Version m_version{0, 0, 0};
};
}  // namespace FOEDAG
