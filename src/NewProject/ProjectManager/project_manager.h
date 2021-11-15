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
#define PROJECT_CONFIG_SIMTOPMODULE "SimulationTopModule"

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

#define RUN_STATE_CURRENT "current"

#define RUN_TYPE_SYNTHESIS "Synthesis"
#define RUN_TYPE_IMPLEMENT "Implementation"

#define DEFAULT_FOLDER_CONSTRS "constrs_1"
#define DEFAULT_FOLDER_SIM "sim_1"
#define DEFAULT_FOLDER_SOURCE "sources_1"
#define DEFAULT_FOLDER_IMPLE "imple_1"
#define DEFAULT_FOLDER_SYNTH "synth_1"

#define PROJECT_FILE_FORMAT ".ospr"

class ProjectManager : public QObject {
  Q_OBJECT
 public:
  explicit ProjectManager(QObject *parent = nullptr);

  int CreateProject(const QString &strName, const QString &strPath);
  int setProjectType(const QString &strType);
  int setDesignFile(const QString &strFileName, bool isFileCopy = true);
  int setSimulationFile(const QString &strFileName, bool isFolder = false,
                        bool isFileCopy = true);
  int setConstrsFile(const QString &strFileName, bool isFolder = false,
                     bool isFileCopy = true);
  int setRunSet(QList<QString> listParam);
  int StartProject(const QString &strOspro);
  void FinishedProject();

  QString currentFileSet() const;
  void setCurrentFileSet(const QString &currentFileSet);

 private:
  int ImportProjectData(QString strOspro);
  int ExportProjectData();

  int CreateProjectDir();
  int CreateFolder(QString strPath);
  int CreateFile(QString strFile);

  QStringList getAllChildFiles(QString path);
  bool CopyFileToPath(QString sourceDir, QString destinDir,
                      bool iscover = true);

 private:
  QString m_currentFileSet;
};

#endif  // PROJECTMANAGER_H
