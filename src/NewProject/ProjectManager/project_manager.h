/*Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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

Note: At present, the ProjectManager manages only one project ,in addition,the
project object is singleton mode.
*/

#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QObject>

#include "project.h"

#define PROJECT_PROJECT "Project"
#define PROJECT_PATH "Path"
#define PROJECT_CONFIGURATION "Configuration"
#define PROJECT_CONFIG_ID "ID"
#define PROJECT_CONFIG_ACTIVESIMSET "ActiveSimSet"
#define PROJECT_CONFIG_TYPE "Project Type"

#define PROJECT_OPTION "Option"
#define PROJECT_NAME "Name"
#define PROJECT_VAL "Val"

#define PROJECT_FILESETS "FileSets"
#define PROJECT_FILESET "FileSet"
#define PROJECT_FILESET_NAME "Name"
#define PROJECT_FILESET_FILE "File"
#define PROJECT_FILESET_TYPE "Type"
#define PROJECT_FILESET_RELSRCDIR "RelSrcDir"
#define PROJECT_FILESET_CONFIG "Config"

#define PROJECT_RUNS "Runs"
#define PROJECT_RUN "Run"
#define PROJECT_RUN_NAME "Name"
#define PROJECT_RUN_TYPE "Type"
#define PROJECT_RUN_SRCSET "SrcSet"
#define PROJECT_RUN_CONSTRSSET "ConstrsSet"
#define PROJECT_RUN_STATE "State"
#define PROJECT_RUN_SYNTHRUN "SynthRun"

#define PROJECT_PART_SERIES "Series"
#define PROJECT_PART_FAMILY "Family"
#define PROJECT_PART_PACKAGE "Package"
#define PROJECT_PART_DEVICE "Device"

#define PROJECT_FILE_TYPE_DS "DesignSrcs"
#define PROJECT_FILE_TYPE_CS "Constrs"
#define PROJECT_FILE_TYPE_SS "SimulationSrcs"

#define PROJECT_FILE_CONFIG_TOP "TopModule"
#define PROJECT_FILE_CONFIG_TARGET "TargetConstrsFile"

#define RUN_STATE_CURRENT "current"

#define RUN_TYPE_SYNTHESIS "Synthesis"
#define RUN_TYPE_IMPLEMENT "Implementation"

#define DEFAULT_FOLDER_CONSTRS "constrs_1"
#define DEFAULT_FOLDER_SIM "sim_1"
#define DEFAULT_FOLDER_SOURCE "sources_1"
#define DEFAULT_FOLDER_IMPLE "imple_1"
#define DEFAULT_FOLDER_SYNTH "synth_1"

#define PROJECT_FILE_FORMAT ".ospr"

namespace FOEDAG {

class ProjectManager : public QObject {
  Q_OBJECT
 public:
  explicit ProjectManager(QObject *parent = nullptr);

  void Tcl_CreateProject(int argc, const char *argv[]);
  int CreateProjectbyXml(const QString &strProXMl);

  // e.g strPath:/root/Desktop/project_1   strName:project_1
  // project_1.ospr file, project_1.runs folder and project_1.srcs folder will
  // be created under the /root/Desktop/project_1 path
  int CreateProject(const QString &strName, const QString &strPath);
  QString getProjectName() const;
  QString getProjectPath() const;

  int setProjectType(const QString &strType);

  // Please set currentfileset before using this function
  int setDesignFile(const QString &strFileName, bool isFileCopy = true);
  // Please set currentfileset before using this function
  int setSimulationFile(const QString &strFileName, bool isFileCopy = true);
  // Please set currentfileset before using this function
  int setConstrsFile(const QString &strFileName, bool isFileCopy = true);
  // Please set currentfileset before using this function
  int deleteFile(const QString &strFileName);

  // Please set currentfileset before using this function
  int setTopModule(const QString &strFileName);
  // Please set currentfileset before using this function
  int setTargetConstrs(const QString &strFileName);

  int setDesignFileSet(const QString &strSetName);
  QStringList getDesignFileSets() const;
  QString getDesignActiveFileSet() const;
  int setDesignActive(const QString &strSetName);
  QStringList getDesignFiles(const QString &strFileSet) const;
  QString getDesignTopModule(const QString &strFileSet) const;

  int setConstrFileSet(const QString &strSetName);
  QStringList getConstrFileSets() const;
  QString getConstrActiveFileSet() const;
  int setConstrActive(const QString &strSetName);
  QStringList getConstrFiles(const QString &strFileSet) const;
  QString getConstrTargetFile(const QString &strFileSet) const;

  int setSimulationFileSet(const QString &strSetName);
  QStringList getSimulationFileSets() const;
  QString getSimulationActiveFileSet() const;
  int setSimulationActive(const QString &strSetName);
  QStringList getSimulationFiles(const QString &strFileSet) const;
  QString getSimulationTopModule(const QString &strFileSet) const;

  QStringList getSynthRunsNames() const;
  QStringList getImpleRunsNames() const;
  QString SynthUsedByImple(const QString &strSynthName) const;
  QList<QPair<QString, QString>> getRunsProperties(const QString &strRunName);

  int setSynthRun(const QString &strRunName);
  int setImpleRun(const QString &strRunName);

  // Please set currentrun before using this function
  int setRunSrcSet(const QString &strSrcSet);
  // Please set currentrun before using this function
  int setRunConstrSet(const QString &strConstrSet);
  // Please set currentrun before using this function
  int setRunSynthRun(const QString &strSynthRunName);
  // Please set currentrun before using this function
  int setSynthesisOption(const QList<QPair<QString, QString>> &listParam);

  int setRunActive(const QString &strRunName);

  QString getActiveRunDevice() const;
  QString getActiveSynthRunName() const;

  int deleteFileSet(const QString &strSetName);
  int deleteRun(const QString &strRunName);

  int StartProject(const QString &strOspro);
  int FinishedProject();

  QString currentFileSet() const;
  void setCurrentFileSet(const QString &currentFileSet);

  QString getCurrentRun() const;
  void setCurrentRun(const QString &currentRun);

 private:
  int ImportProjectData(QString strOspro);
  int ExportProjectData();

  int CreateProjectDir();
  int CreateSrcsFolder(QString strFolderName);
  int CreateRunsFolder(QString strFolderName);

  int CreateVerilogFile(QString strFile);
  int CreateVHDLFile(QString strFile);
  int CreateSDCFile(QString strFile);

  int AddOrCreateFileToFileSet(const QString &strFileName,
                               bool isFileCopy = true);

  QStringList getAllChildFiles(QString path);
  bool CopyFileToPath(QString sourceDir, QString destinDir,
                      bool iscover = true);

 private:
  QString m_currentFileSet;
  QString m_currentRun;
};
}  // namespace FOEDAG
#endif  // PROJECTMANAGER_H
