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
  if (argc < 3 || TclCheckType(QString(argv[1]))) {
    TclHelper(out);
    return false;
  }

  QString strType = QString(argv[1]);
  QString strSetName;
  if (!strType.compare("ds", Qt::CaseInsensitive)) {
    strSetName = m_projManager->getDesignActiveFileSet();
  } else if (!strType.compare("cs", Qt::CaseInsensitive)) {
    strSetName = m_projManager->getConstrActiveFileSet();
  } else if (!strType.compare("ss", Qt::CaseInsensitive)) {
    strSetName = m_projManager->getSimulationActiveFileSet();
  } else {
    return false;
  }

  QString strFileName = QString(argv[2]);
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
  if (argc < 3 || TclCheckType(QString(argv[1]))) {
    TclHelper(out);
    return false;
  }

  QString strType = QString(argv[1]);
  QString strName = QString(argv[2]);
  int ret = 0;
  if (!strType.compare("ds", Qt::CaseInsensitive)) {
    ret = m_projManager->setDesignFileSet(strName);
  } else if (!strType.compare("cs", Qt::CaseInsensitive)) {
    ret = m_projManager->setConstrFileSet(strName);
  } else if (!strType.compare("ss", Qt::CaseInsensitive)) {
    ret = m_projManager->setSimulationFileSet(strName);
  }

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
  if (argc < 3 || TclCheckType(QString(argv[1]))) {
    TclHelper(out);
    return false;
  }

  QString strType = QString(argv[1]);
  QString strSetName;
  if (!strType.compare("ds", Qt::CaseInsensitive)) {
    strSetName = m_projManager->getDesignActiveFileSet();
  } else if (!strType.compare("cs", Qt::CaseInsensitive)) {
    strSetName = m_projManager->getConstrActiveFileSet();
  } else if (!strType.compare("ss", Qt::CaseInsensitive)) {
    strSetName = m_projManager->getSimulationActiveFileSet();
  }

  int ret = 0;
  m_projManager->setCurrentFileSet(strSetName);
  for (int i = 2; i < argc; i++) {
    QString strFileName = argv[i];
    if (!strType.compare("ds", Qt::CaseInsensitive)) {
      ret = m_projManager->setDesignFile(strFileName, false);
    } else if (!strType.compare("cs", Qt::CaseInsensitive)) {
      ret = m_projManager->setConstrsFile(strFileName, false);
    } else if (!strType.compare("ss", Qt::CaseInsensitive)) {
      ret = m_projManager->setSimulationFile(strFileName, false);
    }

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
  if (argc < 3 || TclCheckType(QString(argv[1]))) {
    TclHelper(out);
    return false;
  }

  QString strType = QString(argv[1]);
  QString strName = QString(argv[2]);
  int ret = 0;
  if (!strType.compare("ds", Qt::CaseInsensitive)) {
    ret = m_projManager->setDesignActive(strName);
  } else if (!strType.compare("cs", Qt::CaseInsensitive)) {
    ret = m_projManager->setConstrActive(strName);
  } else if (!strType.compare("ss", Qt::CaseInsensitive)) {
    ret = m_projManager->setSimulationActive(strName);
  } else {
    return false;
  }

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
  if (argc < 2) {
    TclHelper(out);
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

bool TclCommandIntegration::validate() const { return m_projManager && m_form; }

void TclCommandIntegration::TclHelper(std::ostream &out) {
  out << " \n";
  out << " create_design [<type>][<setname>] \n";
  out << " set_active_design [<type>] [<setname>] \n";
  out << "\n";
  out << " add_files [<type>][<filename>] [...] \n";
  out << " set_top_module [<type>][<filename>] \n";
  out << " set_as_target [<filename>] \n";
  out << " These three commands are valid only for active design. \n";
  out << " \n";
  out << " Type has three options:ds,cs and ss . \n";
  out << " ds : design source . \n";
  out << " cs : constraint source . \n";
  out << " ss : simulation source . \n";
}

bool TclCommandIntegration::TclCheckType(QString strType) {
  if (!strType.compare("ds", Qt::CaseInsensitive) ||
      !strType.compare("cs", Qt::CaseInsensitive) ||
      !strType.compare("ss", Qt::CaseInsensitive)) {
    return false;
  } else {
    return true;
  }
}

}  // namespace FOEDAG
