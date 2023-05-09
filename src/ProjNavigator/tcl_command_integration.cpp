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

  QString strName = QString(argv[1]);
  return TclCreateFileSet(strName, out);
}

bool TclCommandIntegration::TclCreateFileSet(const QString &name,
                                             std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  int ret = m_projManager->setDesignFileSet(name);

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
    QFileInfo strFileName = QString{argv[i]};
    ret = m_projManager->setDesignFiles(
        strFileName.fileName(), FromFileType(strFileName.suffix()),
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

bool TclCommandIntegration::TclAddOrCreateDesignFiles(const QString &files,
                                                      int lang,
                                                      std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  QString strSetName = m_projManager->getDesignActiveFileSet();

  m_projManager->setCurrentFileSet(strSetName);
  int ret = m_projManager->setDesignFiles(
      files, lang, m_projManager->getDefaulUnitName(), false);
  if (0 != ret) {
    out << "Failed to add files: " << files.toStdString() << std::endl;
    return false;
  }

  update();
  return true;
}

bool TclCommandIntegration::TclAddDesignFiles(const QString &commands,
                                              const QString &libs,
                                              const QString &files, int lang,
                                              std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  const QString strSetName = m_projManager->getDesignActiveFileSet();
  m_projManager->setCurrentFileSet(strSetName);
  const auto ret = m_projManager->addDesignFiles(
      commands, libs, files, lang, m_projManager->getDefaulUnitName(), false,
      false);
  if (ProjectManager::EC_Success != ret.code) {
    error(ret.code, ret.message, out);
    return false;
  }

  update();
  return true;
}

bool TclCommandIntegration::TclAddSimulationFiles(const QString &commands,
                                                  const QString &libs,
                                                  const QString &files,
                                                  int lang, std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }
  const QString strSetName = m_projManager->getSimulationActiveFileSet();
  m_projManager->setCurrentFileSet(strSetName);
  const auto ret = m_projManager->addSimulationFiles(
      commands, libs, files, lang, m_projManager->getDefaulUnitName(), false,
      false);
  if (ProjectManager::EC_Success != ret.code) {
    error(ret.code, ret.message, out);
    return false;
  }
  update();
  return true;
}

bool TclCommandIntegration::TclAddOrCreateConstrFiles(const QString &file,
                                                      std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  QString strSetName = m_projManager->getConstrActiveFileSet();

  m_projManager->setCurrentFileSet(strSetName);
  int ret = m_projManager->setConstrsFile(file, false);

  if (0 != ret) {
    out << "Failed to add file: " << file.toStdString() << std::endl;
    return false;
  }

  update();
  return true;
}

bool TclCommandIntegration::TclAddConstrFiles(const QString &file,
                                              std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  const QString strSetName = m_projManager->getConstrActiveFileSet();
  m_projManager->setCurrentFileSet(strSetName);

  const QFileInfo info{file};
  if ((info.suffix().compare("pin", Qt::CaseInsensitive) == 0) &&
      !m_projManager->getConstrPinFile().empty()) {
    out << "*.pin constraint file has already added. Only one *.pin file "
           "supported"
        << std::endl;
    return false;
  }

  const int ret = m_projManager->addConstrsFile(file, false, false);

  if (ProjectManager::EC_Success != ret) {
    error(ret, file, out);
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
  const QString projName{argv[1]};
  QString type{};
  if ((argc > 3) && QtUtils::IsEqual(QString{argv[3]}, "-type"))
    type = QString{argv[3]};
  return TclCreateProject(projName, type, out);
}

bool TclCommandIntegration::TclCreateProject(const QString &name,
                                             const QString &type,
                                             std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  int projectType{RTL};
  if (!type.isEmpty()) {
    if (QtUtils::IsEqual(type, "rtl")) {
      projectType = RTL;
    } else if (QtUtils::IsEqual(type, "gate-level")) {
      projectType = PostSynth;
    } else {
      out << "Wrong project type. Values are rtl, gate-level";
      return false;
    }
  }

  QDir dir(name);
  if (dir.exists()) {
    out << "Project \"" << name.toStdString() << "\" was rewritten.";
  }

  createNewDesign(name, projectType);
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

bool TclCommandIntegration::TclshowChatGpt(const std::string &request,
                                           const std::string &content) {
  emit showChatGpt(QString::fromStdString(request),
                   QString::fromStdString(content));
  return true;
}

ProjectManager *TclCommandIntegration::GetProjectManager() {
  return m_projManager;
}

void TclCommandIntegration::saveSettings() { emit saveSettingsSignal(); }

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
