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

#include "Compiler/CompilerDefines.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Utils/QtUtils.h"
#include "gtest/gtest.h"
using namespace FOEDAG;
namespace fs = std::filesystem;

TEST(ProjectManager, ProjectFilesPath_withFile) {
  QString projPath{"path"};
  QString projName{"name"};
  QString fileSet{"set"};
  QString file{"fileName"};
  auto actualPath =
      ProjectManager::ProjectFilesPath(projPath, projName, fileSet, file);
  std::string folder = projName.toStdString() + ".srcs";
  std::filesystem::path p = projPath.toStdString();
  p /= "run_1";
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
  p /= "run_1";
  p /= folder;
  p /= fileSet.toStdString();
  EXPECT_EQ(actualPath.toStdString(), p.string());
}

TEST(ProjectManager, projectBasePath) {
  const std::string projectPath{"path"};
  std::filesystem::path expected{"path"};
  expected /= "run_1";
  EXPECT_EQ(ProjectManager::projectBasePath(projectPath), expected);
}

TEST(ProjectManager, projectBasePathStdWithPath) {
  std::string projectPath{"projectPath"};
  std::string projectName{"projectName"};
  auto actual = ProjectManager::projectSrcsPath(projectPath, projectName);
  std::filesystem::path expected{"projectPath/run_1/projectName.srcs"};
  EXPECT_EQ(actual, expected);
}

TEST(ProjectManager, projectBasePathQStringWithPath) {
  QString projectPath{"projectPath"};
  QString projectName{"projectName"};
  auto actual = ProjectManager::projectSrcsPath(projectPath, projectName);
  std::filesystem::path expected{"projectPath/run_1/projectName.srcs"};
  EXPECT_EQ(actual, expected);
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

TEST(ProjectManager, AddFilesOneLibraryOneGroup) {
  filedata data0{false,      "v",    "filename0", Design::VERILOG_2001,
                 "filepath", "lib0", "gr0"};
  filedata data1{false,      "v",    "filename1", Design::VERILOG_2001,
                 "filepath", "lib0", "gr0"};
  ProjectOptions::FileData data{{data0, data1}, false};

  bool fileAdded{false};  // make sure addFilesFunction was call
  auto addFilesFunction = [&fileAdded](const QString& commands,
                                       const QString& libs,
                                       const QStringList& fileNames, int lang,
                                       const QString& grName, bool isFileCopy,
                                       bool localToProject, QStringList) {
    fileAdded = true;
    QStringList expected = {{"filepath/filename0"}, {"filepath/filename1"}};
    EXPECT_EQ(commands, "-work");
    EXPECT_EQ(libs, "lib0");
    EXPECT_EQ(fileNames, expected);
    EXPECT_EQ(lang, Design::VERILOG_2001);
    EXPECT_EQ(grName, "gr0");
    EXPECT_EQ(isFileCopy, false);
    EXPECT_EQ(localToProject, false);
  };
  ProjectManager::AddFiles(data, addFilesFunction);
  EXPECT_EQ(fileAdded, true);
}

TEST(ProjectManager, AddFilesWithSameName) {
  filedata data0{false,      "v",    "filename0", Design::VERILOG_2001,
                 "filepath", "lib0", "gr0"};
  filedata data1{false,      "v",    "filename0", Design::VERILOG_2001,
                 "filepath", "lib0", "gr0"};
  ProjectOptions::FileData data{{data0, data1}, false};

  bool fileAdded{false};  // make sure addFilesFunction was call
  auto addFilesFunction =
      [&fileAdded](const QString& commands, const QString& libs,
                   const QStringList& fileNames, int lang,
                   const QString& grName, bool isFileCopy, bool localToProject,
                   const QStringList& override) {
        fileAdded = true;
        QStringList expected = {{"filepath/filename0"}, {"filepath/filename0"}};
        EXPECT_EQ(commands, "-work");
        EXPECT_EQ(libs, "lib0");
        EXPECT_EQ(fileNames, expected);
        EXPECT_EQ(lang, Design::VERILOG_2001);
        EXPECT_EQ(grName, "gr0");
        EXPECT_EQ(isFileCopy, false);
        EXPECT_EQ(localToProject, false);
        EXPECT_EQ(override.size(), 0);
      };
  ProjectManager::AddFiles(data, addFilesFunction);
  EXPECT_EQ(fileAdded, true);
}

TEST(ProjectManager, AddFilesWithSameNameOverride) {
  filedata data0{false,      "v",    "filename0", Design::VERILOG_2001,
                 "filepath", "lib0", "gr0"};
  filedata data1{false,      "v",    "filename0", Design::VERILOG_2001,
                 "filepath", "lib0", "gr0",       true};
  ProjectOptions::FileData data{{data0, data1}, false};

  bool fileAdded{false};  // make sure addFilesFunction was call
  auto addFilesFunction =
      [&fileAdded](const QString& commands, const QString& libs,
                   const QStringList& fileNames, int lang,
                   const QString& grName, bool isFileCopy, bool localToProject,
                   const QStringList& override) {
        fileAdded = true;
        QStringList expected = {{"filepath/filename0"}, {"filepath/filename0"}};
        EXPECT_EQ(commands, "-work");
        EXPECT_EQ(libs, "lib0");
        EXPECT_EQ(fileNames, expected);
        EXPECT_EQ(lang, Design::VERILOG_2001);
        EXPECT_EQ(grName, "gr0");
        EXPECT_EQ(isFileCopy, false);
        EXPECT_EQ(localToProject, false);
        EXPECT_EQ(override.size(), 1);
      };
  ProjectManager::AddFiles(data, addFilesFunction);
  EXPECT_EQ(fileAdded, true);
}

TEST(ProjectManager, AddFilesTwoLibrariesOneGroup) {
  filedata data0{false,      "v",    "filename0", Design::VERILOG_2001,
                 "filepath", "lib0", "gr0"};
  filedata data1{false,      "v",    "filename1", Design::VERILOG_2001,
                 "filepath", "lib1", "gr0"};
  ProjectOptions::FileData data{{data0, data1}, false};

  bool fileAdded{false};  // make sure addFilesFunction was call
  auto addFilesFunction = [&fileAdded](const QString& commands,
                                       const QString& libs,
                                       const QStringList& fileNames, int lang,
                                       const QString& grName, bool isFileCopy,
                                       bool localToProject, QStringList) {
    fileAdded = true;
    QStringList expected = {{"filepath/filename0"}, {"filepath/filename1"}};
    EXPECT_EQ(commands, "-work");
    EXPECT_EQ(libs, "lib0 lib1");
    EXPECT_EQ(fileNames, expected);
    EXPECT_EQ(lang, Design::VERILOG_2001);
    EXPECT_EQ(grName, "gr0");
    EXPECT_EQ(isFileCopy, false);
    EXPECT_EQ(localToProject, false);
  };
  ProjectManager::AddFiles(data, addFilesFunction);
  EXPECT_EQ(fileAdded, true);
}

TEST(ProjectManager, AddFilesLanguageMismatch) {
  filedata data0{false,      "v",    "filename0", Design::VERILOG_2001,
                 "filepath", "lib0", "gr0"};
  filedata data1{false,      "v",    "filename1", Design::VERILOG_1995,
                 "filepath", "lib0", "gr0"};
  ProjectOptions::FileData data{{data0, data1}, false};

  bool fileAdded{false};
  auto addFilesFunction =
      [&fileAdded](const QString& commands, const QString& libs,
                   const QStringList& fileNames, int lang,
                   const QString& grName, bool isFileCopy, bool localToProject,
                   QStringList) { fileAdded = true; };
  ProjectManager::AddFiles(data, addFilesFunction);
  EXPECT_EQ(fileAdded, false);
}

TEST(ProjectManager, AddFilesOneLibraryTwoGroups) {
  filedata data0{false,      "v", "filename0", Design::VERILOG_2001,
                 "filepath", "",  "gr0"};
  filedata data1{false,      "v",    "filename1", Design::VERILOG_1995,
                 "filepath", "lib0", "gr1"};
  ProjectOptions::FileData data{{data0, data1}, false};

  uint counter{0};
  auto addFilesFunction =
      [&counter](const QString& commands, const QString& libs,
                 const QStringList& fileNames, int lang, const QString& grName,
                 bool isFileCopy, bool localToProject, QStringList) {
        if (counter == 0) {
          EXPECT_EQ(commands, QString{});
          EXPECT_EQ(libs, QString{});
          EXPECT_EQ(fileNames, QStringList{"filepath/filename0"});
          EXPECT_EQ(lang, Design::VERILOG_2001);
          EXPECT_EQ(grName, "gr0");
          EXPECT_EQ(isFileCopy, false);
          EXPECT_EQ(localToProject, false);
        } else {
          EXPECT_EQ(commands, "-work");
          EXPECT_EQ(libs, "lib0");
          EXPECT_EQ(fileNames, QStringList{"filepath/filename1"});
          EXPECT_EQ(lang, Design::VERILOG_1995);
          EXPECT_EQ(grName, "gr1");
          EXPECT_EQ(isFileCopy, false);
          EXPECT_EQ(localToProject, false);
        }
        counter++;
      };
  ProjectManager::AddFiles(data, addFilesFunction);
  EXPECT_EQ(counter, 2);
}

TEST(ProjectManager, AddFilesOneLibraryNoGroup) {
  filedata data0{false,      "v",    "filename0", Design::VERILOG_2001,
                 "filepath", "lib0", ""};
  filedata data1{false,      "v",    "filename1", Design::VERILOG_2001,
                 "filepath", "lib0", ""};
  ProjectOptions::FileData data{{data0, data1}, true};

  uint counter{0};
  auto addFilesFunction =
      [&counter](const QString& commands, const QString& libs,
                 const QStringList& fileNames, int lang, const QString& grName,
                 bool isFileCopy, bool localToProject, QStringList) {
        if (counter == 0) {
          auto expectedFiles =
              QtUtils::CreatePath(QString{"filepath"}, QString{"filename0"});
          EXPECT_EQ(commands, "-work");
          EXPECT_EQ(libs, "lib0");
          EXPECT_EQ(fileNames, QStringList{expectedFiles});
          EXPECT_EQ(lang, Design::VERILOG_2001);
          EXPECT_EQ(grName, QString{});
          EXPECT_EQ(isFileCopy, true);
          EXPECT_EQ(localToProject, false);
        } else {
          auto expectedFiles =
              QtUtils::CreatePath(QString{"filepath"}, QString{"filename1"});
          EXPECT_EQ(commands, "-work");
          EXPECT_EQ(libs, "lib0");
          EXPECT_EQ(fileNames, QStringList{expectedFiles});
          EXPECT_EQ(lang, Design::VERILOG_2001);
          EXPECT_EQ(grName, QString{});
          EXPECT_EQ(isFileCopy, true);
          EXPECT_EQ(localToProject, false);
        }
        counter++;
      };
  ProjectManager::AddFiles(data, addFilesFunction);
  EXPECT_EQ(counter, 2);
}

TEST(ProjectManager, AddFilesLocalToProject) {
  filedata data0{false,          "v",    "filename0", Design::VERILOG_2001,
                 LocalToProject, "lib0", ""};
  filedata data1{false,          "v",    "filename1", Design::VERILOG_2001,
                 LocalToProject, "lib0", ""};
  ProjectOptions::FileData data{{data0, data1}, true};

  uint counter{0};
  auto addFilesFunction =
      [&counter](const QString& commands, const QString& libs,
                 const QStringList& fileNames, int lang, const QString& grName,
                 bool isFileCopy, bool localToProject, QStringList) {
        if (counter == 0) {
          EXPECT_EQ(commands, "-work");
          EXPECT_EQ(libs, "lib0");
          EXPECT_EQ(fileNames, QStringList{"filename0"});
          EXPECT_EQ(lang, Design::VERILOG_2001);
          EXPECT_EQ(grName, QString{});
          EXPECT_EQ(isFileCopy, false);
          EXPECT_EQ(localToProject, true);
        } else {
          EXPECT_EQ(commands, "-work");
          EXPECT_EQ(libs, "lib0");
          EXPECT_EQ(fileNames, QStringList{"filename1"});
          EXPECT_EQ(lang, Design::VERILOG_2001);
          EXPECT_EQ(grName, QString{});
          EXPECT_EQ(isFileCopy, false);
          EXPECT_EQ(localToProject, true);
        }
        counter++;
      };
  ProjectManager::AddFiles(data, addFilesFunction);
  EXPECT_EQ(counter, 2);
}

TEST(ProjectManager, setDesignFilesDefault) {
  ProjectManager pManager{};
  auto res = pManager.setDesignFiles({}, {}, {}, 0, {}, true, true);
  EXPECT_EQ(res, ProjectManager::EC_FileSetNotExist);
}

TEST(ProjectManager, setSimulationFilesDefault) {
  ProjectManager pManager{};
  auto res = pManager.setSimulationFiles({}, {}, {}, 0, {}, true, true);
  EXPECT_EQ(res, ProjectManager::EC_FileSetNotExist);
}
