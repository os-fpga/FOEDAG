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

  int CreateProject(const QString &strName, const QString &strPath);
  int setProjectType(const QString &strType);

  int setDesignFile(const QString &strFileName, bool isFileCopy = true);
  int setSimulationFile(const QString &strFileName, bool isFileCopy = true);
  int setConstrsFile(const QString &strFileName, bool isFileCopy = true);

  int setSynthesisOption(const QList<QPair<QString, QString>> &listParam);

  int setTopModule(const QString &strFileName);
  int setTargetConstrs(const QString &strFileName);

  int setDesignFileSet(const QString &strSetName);
  QStringList getDesignFileSets() const;
  QString getDesignActiveFileSet() const;
  QStringList getDesignFiles(const QString &strFileSet) const;
  QString getDesignTopModule(const QString &strFileSet) const;

  int setConstrFileSet(const QString &strSetName);
  QStringList getConstrFileSets() const;
  QString getConstrActiveFileSet() const;
  QStringList getConstrFiles(const QString &strFileSet) const;
  QString getConstrTargetFile(const QString &strFileSet) const;

  int setSimulationFileSet(const QString &strSetName);
  QStringList getSimulationFileSets() const;
  QString getSimulationActiveFileSet() const;
  QStringList getSimulationFiles(const QString &strFileSet) const;
  QString getSimulationTopModule(const QString &strFileSet) const;

  int deleteFileSet(const QString &strSetName);
  int deleteRun(const QString &strRun);

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

  int setFileSet(const QString &strFileName, bool isFileCopy = true);

  QStringList getAllChildFiles(QString path);
  bool CopyFileToPath(QString sourceDir, QString destinDir,
                      bool iscover = true);

 private:
  QString m_currentFileSet;
  QString m_currentRun;
};
}  // namespace FOEDAG
#endif  // PROJECTMANAGER_H
