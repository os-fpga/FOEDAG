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
#include "ProjectManagerComponentMigration.h"

#include "Utils/StringUtils.h"

namespace FOEDAG {

ProjectManagerComponentMigration::ProjectManagerComponentMigration(
    ProjectManager *pManager, QObject *parent)
    : ProjectManagerComponent(pManager, parent) {}

QString ProjectManagerComponentMigration::absPath(const QString &path,
                                                  ErrorCode &ec) const {
  if (!m_projectManager) return path;
  if (path.contains(PROJECT_OSRCDIR)) {
    auto pathToRaplace =
        SU::buildPath(fs::path{PROJECT_OSRCDIR},
                      SU::format("%.srcs", m_projectManager->projectName()));
    auto pathToRaplaceQString = ProjectManager::ToQString(pathToRaplace);

    if (!path.contains(pathToRaplaceQString))  // external files
      return ProjectManagerComponent::absPath(path, ec);

    QString absPath = path;
    auto newPath =
        ProjectManager::projectBasePath(m_projectManager->projectPath());
    auto replated =
        absPath.replace(PROJECT_OSRCDIR, ProjectManager::ToQString(newPath));
    std::filesystem::path path{replated.toStdString()};
    std::error_code errorCode{};
    path = std::filesystem::canonical(path, errorCode);
    if (errorCode) {
      ec = {true, "Failed to open file " + replated};
    } else {
      return QString::fromStdString(path.string());
    }
  }
  return path;
}

}  // namespace FOEDAG
