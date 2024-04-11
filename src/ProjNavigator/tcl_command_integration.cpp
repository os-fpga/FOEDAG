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
#include "tcl_command_integration.h"

#include <QDebug>
#include <QDir>
#include <QString>

#include "Compiler/CompilerDefines.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/sources_form.h"
#include "Utils/QtUtils.h"
#include "Utils/StringUtils.h"
#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

namespace FOEDAG {

TclCommandIntegration::TclCommandIntegration(ProjectManager *projManager,
                                             SourcesForm *form)
    : m_projManager(projManager), m_form(form) {}

bool TclCommandIntegration::TclSetTopModule(int argc, const char *argv[],
                                            std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  auto topModule = QString{};
  auto topModuleLib = QString{};

  for (auto i = 1; i < argc; i++) {
    auto argStr = QString(argv[i]);
    if (argStr == "-work") {
      if (i + 1 >= argc) {
        out << "Incorrect syntax for set_top_module <top> ?-work <libName>?. "
               "Library name should follow '-work' tag"
            << std::endl;
        return false;
      }
      topModuleLib = QString(argv[++i]);  // library name follows -work tag
    } else {
      topModule = argStr;
    }
  }
  m_projManager->setCurrentFileSet(m_projManager->getDesignActiveFileSet());
  auto ret = m_projManager->setTopModule(topModule);
  if (!topModuleLib.isEmpty()) m_projManager->setTopModuleLibrary(topModuleLib);
  if (0 == ret) update();

  return true;
}

bool TclCommandIntegration::TclCreateFileSet(int argc, const char *argv[],
                                             std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  std::string strName = std::string(argv[1]);
  return TclCreateFileSet(strName, out);
}

bool TclCommandIntegration::TclCreateFileSet(const std::string &name,
                                             std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  int ret = m_projManager->setDesignFileSet(QString::fromStdString(name));

  if (1 == ret) {
    out << "The set name is already exists!" << std::endl;
    return false;
  }
  if (0 == ret) update();
  return true;
}

bool TclCommandIntegration::TclAddOrCreateDesignFiles(int argc,
                                                      const char *argv[],
                                                      std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  QString strSetName = m_projManager->getDesignActiveFileSet();

  int ret = 0;
  m_projManager->setCurrentFileSet(strSetName);
  for (int i = 1; i < argc; i++) {
    QFileInfo strFileName{QString{argv[i]}};
    ret = m_projManager->setDesignFiles(
        {}, {}, {strFileName.fileName()}, FromFileType(strFileName.suffix()),
        m_projManager->getDefaulUnitName(), false);

    if (0 != ret) {
      out << "Failed to add file: " << strFileName.fileName().toStdString()
          << std::endl;
      return false;
    }
  }

  if (0 == ret) update();
  return true;
}

bool TclCommandIntegration::TclAddDesignFiles(
    const std::string &commands, const std::string &libs,
    const std::vector<std::string> &files, int lang, std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  const QString strSetName = m_projManager->getDesignActiveFileSet();
  m_projManager->setCurrentFileSet(strSetName);
  const auto ret = m_projManager->addDesignFiles(
      QString::fromStdString(commands), QString::fromStdString(libs),
      QtUtils::ToQStringList(files), lang, m_projManager->getDefaulUnitName(),
      false, false);
  if (ProjectManager::EC_Success != ret.code) {
    error(ret.code, ret.message, out);
    return false;
  }

  update();
  return true;
}

bool TclCommandIntegration::TclAddSimulationFiles(
    const std::string &commands, const std::string &libs,
    const std::vector<std::string> &files, int lang, std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }
  const QString strSetName = m_projManager->getSimulationActiveFileSet();
  m_projManager->setCurrentFileSet(strSetName);
  const auto ret = m_projManager->addSimulationFiles(
      QString::fromStdString(commands), QString::fromStdString(libs),
      QtUtils::ToQStringList(files), lang, m_projManager->getDefaulUnitName(),
      false, false);
  if (ProjectManager::EC_Success != ret.code) {
    error(ret.code, ret.message, out);
    return false;
  }
  update();
  return true;
}

bool TclCommandIntegration::TclVerifySynthPorts(std::ostream &out) {
  if (!validate()) {
    out << "Command validation failed for verify_synth_ports: internal error"
        << std::endl;
    return false;
  }
  update();
  return true;
}

bool TclCommandIntegration::TclAddConstrFiles(const std::string &file,
                                              std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  const QString strSetName = m_projManager->getConstrActiveFileSet();
  m_projManager->setCurrentFileSet(strSetName);

  auto fileStr = QString::fromStdString(file);
  const QFileInfo info{fileStr};
  if ((info.suffix().compare("pin", Qt::CaseInsensitive) == 0) &&
      !m_projManager->getConstrPinFile().empty()) {
    out << "*.pin constraint file has already added. Only one *.pin file "
           "supported"
        << std::endl;
    return false;
  }

  const int ret = m_projManager->addConstrsFile(fileStr, false, false);

  if (ProjectManager::EC_Success != ret) {
    error(ret, fileStr, out);
    return false;
  }

  update();
  return true;
}

bool TclCommandIntegration::TclSetActive(int argc, const char *argv[],
                                         std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  QString strName = QString(argv[1]);
  int ret = m_projManager->setDesignActive(strName);

  if (0 == ret) update();
  return true;
}

bool TclCommandIntegration::TclSetAsTarget(int argc, const char *argv[],
                                           std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  QString strFileName = QString(argv[1]);
  m_projManager->setCurrentFileSet(m_projManager->getConstrActiveFileSet());
  int ret = m_projManager->setTargetConstrs(strFileName);
  if (0 == ret) update();
  return true;
}

bool TclCommandIntegration::TclCreateProject(int argc, const char *argv[],
                                             std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }
  bool cleanup{false};
  const std::string projName{argv[1]};
  std::string type{};
  for (int i = 2; i < argc; i++) {
    std::string arg{argv[i]};
    if (arg == "clean")
      cleanup = true;
    else if (arg == "-type" && (i < argc - 1))
      type = argv[i + 1];
  }
  return TclCreateProject(projName, type, cleanup, out);
}

bool TclCommandIntegration::TclCreateProject(const std::string &name,
                                             const std::string &type,
                                             bool cleanup, std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  int projectType{RTL};
  QString typeqstr = QString::fromStdString(type);
  if (!typeqstr.isEmpty()) {
    if (QtUtils::IsEqual(typeqstr, "rtl")) {
      projectType = RTL;
    } else if (QtUtils::IsEqual(typeqstr, "gate-level")) {
      projectType = GateLevel;
    } else {
      out << "Wrong project type. Values are rtl, gate-level";
      return false;
    }
  }

  QString nameqstr = QString::fromStdString(name);
  QDir dir(nameqstr);
  if (dir.exists()) {
    if (cleanup) dir.removeRecursively();
    out << "Project \"" << name << "\" was rewritten.";
  }

  createNewDesign(nameqstr, projectType);
  return true;
}

bool TclCommandIntegration::TclCloseProject() {
  if (m_form) {  // GUI mode
    emit closeDesign();
  } else {  // batch mode
    Project::Instance()->InitProject();
  }
  return true;
}

bool TclCommandIntegration::TclClearSimulationFiles(std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }
  auto simFiles = m_projManager->getSimulationFiles(
      m_projManager->getSimulationActiveFileSet());
  for (const auto &strfile : simFiles) {
    const QFileInfo info{strfile};
    m_projManager->deleteFile(info.fileName());
  }
  update();
  return true;
}

bool TclCommandIntegration::TclSetTopTestBench(int argc, const char *argv[],
                                               std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  if (argc != 2) {
    out << "Wrong arguments count. See help for `set_top_testbench` command"
        << std::endl;
    return false;
  }
  const QString strSetName = m_projManager->getSimulationActiveFileSet();
  m_projManager->setCurrentFileSet(strSetName);
  m_projManager->setTopModuleSim(QString::fromLatin1(argv[1]));
  update();
  return true;
}

ProjectManager *TclCommandIntegration::GetProjectManager() {
  return m_projManager;
}

void TclCommandIntegration::saveSettings() { emit saveSettingsSignal(); }

std::vector<std::string> TclCommandIntegration::GetClockList(
    const std::filesystem::path &path, bool &vhdl) {
  QFile jsonFile{QString::fromStdString(path.string())};
  if (jsonFile.exists() &&
      jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Read/parse json from file and update the passed jsonObject w/ new vals
    QString jsonStr = jsonFile.readAll();
    json jsonObject;
    try {
      jsonObject = json::parse(jsonStr.toStdString());
    } catch (...) {
      return {};
    }
    std::vector<std::string> ports;
    auto hierTree = jsonObject.at("hierTree");
    for (auto it = hierTree.begin(); it != hierTree.end(); it++) {
      auto portsArr = it->at("ports");
      auto language = it->at("language");
      if (isVHDL(language)) vhdl = true;
      for (auto it{portsArr.cbegin()}; it != portsArr.cend(); ++it) {
        const auto range = it->at("range");
        const int msb = range["msb"];
        const int lsb = range["lsb"];
        if (msb == 0 && lsb == 0) ports.push_back(it->at("name"));
      }
    }
    return ports;
  }
  return {};
}

void TclCommandIntegration::updateHierarchyView() { emit updateHierarchy(); }

void TclCommandIntegration::updateReportsView() { emit updateReports(); }

void TclCommandIntegration::createNewDesign(const QString &projName,
                                            int projectType) {
  ProjectOptions opt{projName,
                     QString("%1/%2").arg(QDir::currentPath(), projName),
                     projectType,
                     {{}, false},
                     {{}, false},
                     {{}, false},
                     {},
                     true /*rewrite*/,
                     DEFAULT_FOLDER_SOURCE,
                     ProjectOptions::Options{},
                     ProjectOptions::Options{}};
  m_projManager->CreateProject(opt);
  QString newDesignStr{m_projManager->getProjectPath() + "/" +
                       m_projManager->getProjectName() + PROJECT_FILE_FORMAT};
  emit newDesign(newDesignStr);
  update();
}

bool TclCommandIntegration::isVHDL(const std::string &str) {
  static const StringVector vhdl{"VHDL_87",   "VHDL_93",   "VHDL_2K",
                                 "VHDL_2008", "VHDL_2019", "VHDL_PSL"};
  return std::find(vhdl.begin(), vhdl.end(), str) != vhdl.end();
}

bool TclCommandIntegration::validate() const { return m_projManager; }

void TclCommandIntegration::update() {
  if (m_projManager) m_projManager->FinishedProject();
  if (m_form) m_form->UpdateSrcHierachyTree();
}

void TclCommandIntegration::error(int res, const QString &filename,
                                  std::ostream &out) {
  switch (res) {
    case ProjectManager::EC_Success:
      break;
    case ProjectManager::EC_FileNotExist:
      out << "File(s) do not exist: " << filename.toStdString() << std::endl;
      break;
    default:
      out << "Failed to add files: " << filename.toStdString() << std::endl;
      break;
  }
}

}  // namespace FOEDAG
