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

#include "NewProject/ProjectManager/project_manager.h"
#include "gtest/gtest.h"
using namespace FOEDAG;

TEST(ProjectManager, ProjectFilesPath_withFile) {
  QString projPath{"path"};
  QString projName{"name"};
  QString fileSet{"set"};
  QString file{"fileName"};
  auto actualPath =
      ProjectManager::ProjectFilesPath(projPath, projName, fileSet, file);
  std::string folder = projName.toStdString() + ".srcs";
  std::filesystem::path p = projPath.toStdString();
  p /= folder;
  p /= fileSet.toStdString();
  p /= file.toStdString();
  EXPECT_EQ(actualPath.toStdString(), p.string());
}

TEST(ProjectManager, ProjectFilesPath_noFile) {
  QString projPath{"path"};
  QString projName{"name"};
  QString fileSet{"set"};
  auto actualPath =
      ProjectManager::ProjectFilesPath(projPath, projName, fileSet);
  std::string folder = projName.toStdString() + ".srcs";
  std::filesystem::path p = projPath.toStdString();
  p /= folder;
  p /= fileSet.toStdString();
  EXPECT_EQ(actualPath.toStdString(), p.string());
}

TEST(ProjectManager, ParseMacroEmpty) {
  QString macro{};
  auto list = ProjectManager::ParseMacro(macro);
  EXPECT_EQ(list.size(), 0);
}

TEST(ProjectManager, ParseMacroOneElement) {
  QString macro{"p=20"};
  auto list = ProjectManager::ParseMacro(macro);
  EXPECT_EQ(list.size(), 1);
  EXPECT_EQ(list.at(0).first, "p");
  EXPECT_EQ(list.at(0).second, "20");
}

TEST(ProjectManager, ParseMacroTwoElements) {
  QString macro{"p=20 k=30"};
  auto list = ProjectManager::ParseMacro(macro);
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list.at(0).first, "p");
  EXPECT_EQ(list.at(0).second, "20");

  EXPECT_EQ(list.at(1).first, "k");
  EXPECT_EQ(list.at(1).second, "30");
}

TEST(ProjectManager, ParseMacroWithDefine) {
  QString macro{"def= k=30"};
  auto list = ProjectManager::ParseMacro(macro);
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list.at(0).first, "def");
  EXPECT_EQ(list.at(0).second, "");

  EXPECT_EQ(list.at(1).first, "k");
  EXPECT_EQ(list.at(1).second, "30");
}
