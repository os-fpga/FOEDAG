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
#include "FileLoaderOldStructure.h"

#include <filesystem>

#include "MainWindow/CompressProject.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Utils/StringUtils.h"

namespace FOEDAG {

FileLoaderMigration::FileLoaderMigration(const QString &projectFileName)
    : m_project(projectFileName) {}

std::pair<bool, QString> FileLoaderMigration::Migrate() const {
  if (m_project.isEmpty())
    return {false, "Project path not specified"};  // internal
  auto path = fs::path{m_project.toStdString()};

  auto result = CompressProject::CompressZip(path.parent_path(),
                                             path.stem().filename().string());
  if (!result.first) return {false, "Failed to create backup"};

  // move <project>.srcs under run_1 folder
  auto projectName = path.stem().filename().string();
  auto projectPath = path.parent_path();
  auto oldFolder = projectPath / SU::format("%.srcs", projectName);
  auto baseFolder =
      ProjectManager::projectSrcsPath(projectPath.string(), projectName);

  // cleanup run folder
  fs::remove_all(baseFolder.parent_path());

  std::error_code ec{};
  std::filesystem::create_directories(baseFolder, ec);
  if (ec)
    return std::make_pair(
        false, QString::fromStdString(SU::format("Failed to create directory %",
                                                 baseFolder.string())));
  std::filesystem::copy(oldFolder, baseFolder,
                        std::filesystem::copy_options::recursive, ec);
  if (ec) {
    auto message = SU::format("Failed copy folder from % to %",
                              oldFolder.string(), baseFolder.string());
    return std::make_pair(false, QString::fromStdString(message));
  }
  // remove old folder
  fs::remove_all(oldFolder);
  return {true, QString{}};
}

}  // namespace FOEDAG
