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

constexpr auto GENERIC_OPTION{"Option"};
constexpr auto GENERIC_NAME{"Name"};
constexpr auto GENERIC_VAL{"Val"};

constexpr auto IP_CONFIG{"IpConfig"};
constexpr auto IP_INSTANCE_PATHS{"InstancePaths"};
constexpr auto IP_CATALOG_PATHS{"CatalogPaths"};
constexpr auto IP_INSTANCE_CMDS{"InstanceCmds"};

class ProjectManagerComponent : public ProjectFileComponent {
 public:
  explicit ProjectManagerComponent(ProjectManager *pManager,
                                   QObject *parent = nullptr);
  void Save(QXmlStreamWriter *writer) override;
  ErrorCode Load(QXmlStreamReader *reader) override;
  void LoadDone() override;
  ProjectManager *ProjManager() const;

 protected:
  virtual void ReadIPProperties(QXmlStreamReader &reader);
  QString relatedPath(const QString &path) const;
  virtual QString absPath(const QString &path, ErrorCode &ec) const;

  QString relatedPathList(const QStringList &pathList) const;
  QString absPathList(const QStringList &pathList, ErrorCode &ec) const;

 protected:
  ProjectManager *m_projectManager{nullptr};
};

}  // namespace FOEDAG
