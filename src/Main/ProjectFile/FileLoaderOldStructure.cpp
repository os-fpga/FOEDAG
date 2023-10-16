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

#include "MainWindow/CompressProject.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Utils/StringUtils.h"
#include "qdebug.h"

namespace FOEDAG {

FileLoaderMigration::FileLoaderMigration(const QString &projectFileName,
                                         const QString &synthPath)
    : m_project(projectFileName), m_synthPath(synthPath) {}

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
  auto srcsFolder =
      ProjectManager::projectSrcsPath(projectPath.string(), projectName);
  std::error_code ec{};

  // cleanup synth_1_1 folder. This should be first.
  auto synthPath =
      ProjectManager::projectSynthSettingsPath(projectPath).parent_path();
  fs::remove_all(synthPath, ec);

  std::filesystem::create_directories(srcsFolder, ec);
  if (ec)
    return std::make_pair(
        false, QString::fromStdString(SU::format("Failed to create directory %",
                                                 srcsFolder.string())));
  std::filesystem::copy(oldFolder, srcsFolder,
                        std::filesystem::copy_options::recursive, ec);
  if (ec) {
    auto message = SU::format("Failed copy folder from % to %",
                              oldFolder.string(), srcsFolder.string());
    return std::make_pair(false, QString::fromStdString(message));
  }
  // remove old folder
  fs::remove_all(oldFolder);

  // move settings folder
  auto oldSettingsFolder = projectPath / SU::format("%.settings", projectName);
  auto synthSettingsFolder =
      ProjectManager::projectSynthSettingsPath(projectPath.string());
  auto implSettingsFolder =
      ProjectManager::projectImplSettingsPath(projectPath.string());
  MoveFolder(oldSettingsFolder, synthSettingsFolder);
  std::filesystem::create_directories(implSettingsFolder, ec);

  // move IPs folder
  auto oldIPsFolder = projectPath / SU::format("%.IPs", projectName);
  auto IPsFolder = ProjectManager::projectIPsPath(projectPath.string());
  MoveFolder(oldIPsFolder, IPsFolder);

  return {true, QString{}};
}

void FileLoaderMigration::MoveFolder(const std::filesystem::path &from,
                                     const std::filesystem::path &to) {
  std::error_code ec{};
  std::filesystem::create_directories(to, ec);
  std::filesystem::copy(from, to, std::filesystem::copy_options::recursive, ec);
  fs::remove_all(from, ec);
}

}  // namespace FOEDAG
