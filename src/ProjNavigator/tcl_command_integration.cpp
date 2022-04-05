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
#include <QMetaMethod>
#include <QString>

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

  QString strSetName{"ds"};

  QString strFileName = QString(argv[1]);
  m_projManager->setCurrentFileSet(strSetName);
  QString module = strFileName.left(strFileName.lastIndexOf("."));
  int ret = m_projManager->setTopModule(module);
  if (0 == ret) {
    m_projManager->FinishedProject();
    m_form->UpdateSrcHierachyTree();
  }
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
  } else if (0 == ret) {
    m_projManager->FinishedProject();
    m_form->UpdateSrcHierachyTree();
  }
  return true;
}

bool TclCommandIntegration::TclAddOrCreateFiles(int argc, const char *argv[],
                                                std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }

  QString strSetName = m_projManager->getDesignActiveFileSet();

  int ret = 0;
  m_projManager->setCurrentFileSet(strSetName);
  for (int i = 1; i < argc; i++) {
    QString strFileName = argv[i];
    ret = m_projManager->setDesignFile(strFileName, false);

    if (0 != ret) {
      out << "Failed to add file: " << strFileName.data() << std::endl;
      return false;
    }
  }

  if (0 == ret) {
    m_projManager->FinishedProject();
    m_form->UpdateSrcHierachyTree();
  }
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

  if (0 == ret) {
    m_projManager->FinishedProject();
    m_form->UpdateSrcHierachyTree();
  }
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
  if (0 == ret) {
    m_projManager->FinishedProject();
    m_form->UpdateSrcHierachyTree();
  }
  return true;
}

bool TclCommandIntegration::TclCreateProject(int argc, const char *argv[],
                                             std::ostream &out) {
  if (!validate()) {
    out << "Command validation fail: internal error" << std::endl;
    return false;
  }
  const QString projName{argv[1]};
  QString tmpPath = projName;
  QDir dir(tmpPath);
  if (dir.exists()) {
    out << "Project \"" << projName.toStdString() << "\" was rewritten.";
  }

  // make indirect call since different threads. QObject throws warnings
  // otherwise.
  int methodIndex = metaObject()->indexOfMethod("createNewDesign(QString)");
  QMetaMethod method = metaObject()->method(methodIndex);
  method.invoke(this, Qt::QueuedConnection, Q_ARG(QString, projName));
  return true;
}

QString TclCommandIntegration::getActiveDesign() const {
  return m_projManager->getProjectName();
}

void TclCommandIntegration::createNewDesign(const QString &projName) {
  ProjectOptions opt{projName,
                     QString("%1/%2").arg(QDir::currentPath(), projName),
                     "RTL",
                     {{}, false},
                     {{}, false},
                     {"series1", "familyone", "SBG484", "fpga100t"},
                     true /*rewrite*/,
                     projName};
  m_projManager->CreateProject(opt);
  QString newDesignStr{m_projManager->getProjectPath() + "/" +
                       m_projManager->getProjectName() + PROJECT_FILE_FORMAT};
  emit newDesign(newDesignStr);
  m_form->UpdateSrcHierachyTree();
}

bool TclCommandIntegration::validate() const { return m_projManager && m_form; }

}  // namespace FOEDAG
