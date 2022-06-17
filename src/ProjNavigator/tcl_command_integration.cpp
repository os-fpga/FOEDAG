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

  QString strFileName = QString(argv[1]);
  m_projManager->setCurrentFileSet(m_projManager->getDesignActiveFileSet());
  QString module = strFileName.left(strFileName.lastIndexOf("."));
  int ret = m_projManager->setTopModule(module);
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
        strFileName.fileName(), FromFileType(strFileName.suffix()), false);

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
  int ret = m_projManager->setDesignFiles(files, lang, false);
  if (0 != ret) {
    out << "Failed to add files: " << files.toStdString() << std::endl;
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
  return TclCreateProject(projName, out);
}

bool TclCommandIntegration::TclCreateProject(const QString &name,
                                             std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }
  QDir dir(name);
  if (dir.exists()) {
    out << "Project \"" << name.toStdString() << "\" was rewritten.";
  }

  createNewDesign(name);
  return true;
}

ProjectManager *TclCommandIntegration::GetProjectManager() {
  return m_projManager;
}

void TclCommandIntegration::createNewDesign(const QString &projName) {
  ProjectOptions opt{projName,
                     QString("%1/%2").arg(QDir::currentPath(), projName),
                     "RTL",
                     {{}, false},
                     {{}, false},
                     {"series1", "familyone", "SBG484", "fpga100t"},
                     true /*rewrite*/,
                     DEFAULT_FOLDER_SOURCE};
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

}  // namespace FOEDAG
