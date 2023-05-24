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

#include "NewProject/ProjectManager/project_manager.h"
#include "ProjectFileComponent.h"

namespace FOEDAG {

class ProjectManagerComponent : public ProjectFileComponent {
 public:
  ProjectManagerComponent(ProjectManager *pManager, QObject *parent = nullptr);
  void Save(QXmlStreamWriter *writer) override;
  void Load(QXmlStreamReader *reader) override;

 protected:
  QString relatedPath(const QString &path) const;
  QString absPath(const QString &path) const;

  QString relatedPathList(const QStringList &pathList) const;
  QString absPathList(const QStringList &pathList) const;

 protected:
  ProjectManager *m_projectManager{nullptr};
};

}  // namespace FOEDAG
