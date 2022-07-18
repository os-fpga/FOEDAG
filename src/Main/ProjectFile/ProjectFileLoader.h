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
#include <QObject>

#include "CompilerComponent.h"
#include "ProjectManagerComponent.h"
#include "TaskManagerComponent.h"

namespace FOEDAG {

class ProjectFileLoader : public QObject {
 public:
  explicit ProjectFileLoader(QObject *parent = nullptr);
  explicit ProjectFileLoader(
      const std::vector<ProjectFileComponent *> &components,
      QObject *parent = nullptr);
  ~ProjectFileLoader();
  void registerComponent(ProjectFileComponent *comp);

 public slots:
  void Load(const QString &filename);
  void Save();

 protected:
  static QString ProjectVersion(const QString &filename);

 private:
  std::vector<ProjectFileComponent *> m_components;
};

}  // namespace FOEDAG
