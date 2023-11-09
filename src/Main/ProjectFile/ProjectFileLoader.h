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

// the ID's should not be changed since it is define the order of save/load
enum class ComponentId {
  ProjectManager = 0,
  TaskManager = 1,
  Compiler = 2,
  Count,
};

class Project;
class ProjectFileComponent;
class ProjectFileLoader : public QObject {
 public:
  explicit ProjectFileLoader(Project *project, QObject *parent = nullptr);
  ~ProjectFileLoader() override;
  void registerComponent(ProjectFileComponent *comp, ComponentId id);
  ErrorCode Load(const QString &filename);
  void setParentWidget(QWidget *parent);

 public slots:
  void Save();

 protected:
  static QString ProjectVersion(const QString &filename);

 private:
  struct LoadResult {
    ErrorCode errorCode{};
    bool migrationDoneSuccessfully{false};
  };
  LoadResult LoadInternal(const QString &filename);
  static LoadResult LoadXml(
      const QString &filename,
      const std::vector<ProjectFileComponent *> &components);

 private:
  std::vector<ProjectFileComponent *> m_components;
  bool m_loadDone{true};
  QWidget *m_parent{nullptr};
};

}  // namespace FOEDAG
