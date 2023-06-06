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

#include <QFileSystemWatcher>
#include <QObject>
#include <filesystem>

#include "../source_grid.h"
#include "project.h"

#define PROJECT_PROJECT "Project"
#define PROJECT_PATH "Path"
#define PROJECT_VERSION "Version"
#define PROJECT_CONFIGURATION "Configuration"
#define PROJECT_CONFIG_ID "ID"
#define PROJECT_CONFIG_ACTIVESIMSET "ActiveSimSet"
#define PROJECT_CONFIG_TYPE "Project Type"

#define PROJECT_OPTION "Option"
#define PROJECT_NAME "Name"
#define PROJECT_VAL "Val"

#define PROJECT_GROUP "Group"
#define PROJECT_GROUP_ID "Id"
#define PROJECT_GROUP_NAME "Name"
#define PROJECT_GROUP_FILES "Files"

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
#define PROJECT_FILE_CONFIG_TOP_LIB "TopModuleLib"
#define PROJECT_FILE_CONFIG_TARGET "TargetConstrsFile"

#define PROJECT_RUN_OPTION_FLOW "Compilation Flow"
#define PROJECT_RUN_OPTION_LANGUAGEVER "LanguageVersion"
#define PROJECT_RUN_OPTION_TARGETLANG "TargetLanguage"

#define RUN_STATE_CURRENT "current"

#define RUN_TYPE_SYNTHESIS "Synthesis"
#define RUN_TYPE_IMPLEMENT "Implementation"

#define DEFAULT_FOLDER_CONSTRS "constrs_1"
#define DEFAULT_FOLDER_SIM "sim_1"
#define DEFAULT_FOLDER_SOURCE "sources_1"
#define DEFAULT_FOLDER_IMPLE "imple_1"
#define DEFAULT_FOLDER_SYNTH "synth_1"

#define PROJECT_FILE_FORMAT ".ospr"

#define PROJECT_OSRCDIR "$OSRCDIR"
constexpr auto LocalToProject{"<Local to Project>"};

namespace FOEDAG {

struct ProjectOptions {
  struct FileData {
    QList<filedata> fileData;
    bool isCopySource;
  };
  struct Options {
    QString topModule;
    QString topModuleLib;
    QString includePathList;
    QString libraryPathList;
    QString libraryExtList;
    QString macroList;
  };
  QString projectName;
  QString projectPath;
  int projectType;
  FileData sourceFileData;
  FileData constrFileData;
  FileData simFileData;
  QStringList device;
  bool rewriteProject;
  QString currentFileSet;
  Options designOptions;
  Options simulationOptions;
};

struct Suffixes {
  const std::vector<QString> suffixes;
  Suffixes(const std::vector<QString> &s) : suffixes(s) {}
  bool TestSuffix(const QString &s) const;
};

enum ProjectType {
  RTL = 0,
  PostSynth = 1,
};

class ProjectManager : public QObject {
  Q_OBJECT
 public:
  using Libraries = std::vector<
      std::pair<std::vector<std::string>, std::vector<std::string>>>;
  using CompilationUnits = std::vector<std::pair<CompilationUnit, std::string>>;
  enum ErrorCode : int {
    EC_Success = 0,
    EC_FileSetNotExist = -1,
    EC_ProjRunNotExist = -2,
    EC_FileNotExist = -3,
  };
  struct ErrorInfo {
    ErrorCode code;
    QString message = QString();
  };
  explicit ProjectManager(QObject *parent = nullptr);
  void CreateProject(const ProjectOptions &opt);
  void UpdateProject(const ProjectOptions &opt);
  static QString ProjectFilesPath(const QString &projPath,
                                  const QString &projName,
                                  const QString &fileSet,
                                  const QString &file = QString());

  static std::filesystem::path ProjectFilesPath(
      const std::string &projPath, const std::string &projName,
      const std::string &fileSet, const std::string &file = std::string());

  void Tcl_CreateProject(int argc, const char *argv[]);
  int CreateProjectbyXml(const QString &strProXMl);

  // e.g strPath:/root/Desktop/project_1   strName:project_1
  // project_1.ospr file, project_1.runs folder and project_1.srcs folder will
  // be created under the /root/Desktop/project_1 path
  int CreateProject(const QString &strName, const QString &strPath,
                    const QString &designSource = DEFAULT_FOLDER_SOURCE);
  QString getProjectName() const;
  std::string projectName() const;
  QString getProjectPath() const;
  std::string projectPath() const;
  bool HasDesign() const;

  int setProjectType(int strType);
  ProjectType projectType() const;

  ErrorInfo addFiles(const QString &commands, const QString &libs,
                     const QString &fileNames, int lang, const QString &grName,
                     bool isFileCopy = true, bool localToProject = true);

  ErrorInfo addDesignFiles(const QString &commands, const QString &libs,
                           const QString &fileNames, int lang,
                           const QString &grName, bool isFileCopy = true,
                           bool localToProject = true);

  ErrorInfo addSimulationFiles(const QString &commands, const QString &libs,
                               const QString &fileNames, int lang,
                               const QString &grName, bool isFileCopy = true,
                               bool localToProject = true);

  QString getDefaulUnitName() const;
  int setDesignFiles(const QString &fileNames, int lang, const QString &grName,
                     bool isFileCopy = true, bool localToProject = true);
  int setDesignFiles(const QString &commands, const QString &libs,
                     const QString &fileNames, int lang, const QString &grName,
                     bool isFileCopy = true, bool localToProject = true);
  // Please set currentfileset before using this function
  int setSimulationFile(const QString &strFileName, bool isFileCopy = true,
                        bool localToProject = true);
  int setSimulationFiles(const QString &commands, const QString &libs,
                         const QString &fileNames, int lang,
                         const QString &grName, bool isFileCopy,
                         bool localToProject);
  int addConstrsFile(const QString &strFileName, bool isFileCopy = true,
                     bool localToProject = true);
  // Please set currentfileset before using this function
  int setConstrsFile(const QString &strFileName, bool isFileCopy = true,
                     bool localToProject = true);
  // Please set currentfileset before using this function
  int deleteFile(const QString &strFileName);

  // Please set currentfileset before using this function
  int setTopModule(const QString &strModuleName);
  ErrorCode setTopModuleSim(const QString &strModuleName);
  int setTopModuleLibrary(const QString &strModuleNameLib);
  // Please set currentfileset before using this function
  int setTargetConstrs(const QString &strFileName);

  int setDesignFileSet(const QString &strSetName);
  QStringList getDesignFileSets() const;
  QString getDesignActiveFileSet() const;
  int setDesignActive(const QString &strSetName);
  QStringList getDesignFiles(const QString &strFileSet) const;
  QStringList getDesignFiles() const;
  // Compiler interface
  // design
  Libraries DesignLibraries() const;
  CompilationUnits DesignFiles() const;
  std::vector<std::pair<CompilationUnit, std::vector<std::string>>>
  DesignFileList() const;
  // simulation
  Libraries SimulationLibraries() const;
  CompilationUnits SimulationFiles() const;
  std::vector<std::pair<CompilationUnit, std::vector<std::string>>>
  SimulationFileList() const;
  // --------------------
  QString getDesignTopModule(const QString &strFileSet) const;
  QString getDesignTopModule() const;
  std::string DesignTopModule() const;

  QString getSimulationTopModule() const;
  std::string SimulationTopModule() const;

  QString getDesignTopModuleLib(const QString &strFileSet) const;
  QString getDesignTopModuleLib() const;
  std::string DesignTopModuleLib() const;

  QString getSimulationTopModuleLib(const QString &strFileSet) const;
  QString getSimulationTopModuleLib() const;
  std::string SimulationTopModuleLib() const;

  int setConstrFileSet(const QString &strSetName);
  QStringList getConstrFileSets() const;
  QString getConstrActiveFileSet() const;
  int setConstrActive(const QString &strSetName);
  QStringList getConstrFiles(const QString &strFileSet) const;
  QString getConstrTargetFile(const QString &strFileSet) const;
  std::vector<std::string> getConstrFiles() const;
  std::string getConstrPinFile() const;

  int setSimulationFileSet(const QString &strSetName);
  QStringList getSimulationFileSets() const;
  QString getSimulationActiveFileSet() const;
  int setSimulationActive(const QString &strSetName);
  QStringList getSimulationFiles(const QString &strFileSet) const;
  QString getSimulationTopModule(const QString &strFileSet) const;

  QStringList getSynthRunsNames() const;
  QStringList getImpleRunsNames() const;
  QStringList ImpleUsedSynth(const QString &strSynthName) const;
  QList<QPair<QString, QString>> getRunsProperties(
      const QString &strRunName) const;

  int setSynthRun(const QString &strRunName);
  int setImpleRun(const QString &strRunName);

  // Please set currentrun before using this function, Unless you used
  // setSynthRun/setImpleRun before
  int setRunSrcSet(const QString &strSrcSet);
  QString getRunSrcSet(const QString &strRunName) const;
  // Please set currentrun before using this function, Unless you used
  // setSynthRun/setImpleRun before
  int setRunConstrSet(const QString &strConstrSet);
  QString getRunConstrSet(const QString &strRunName) const;
  // Please set currentrun before using this function, Unless you used
  // setSynthRun/setImpleRun before
  int setRunSynthRun(const QString &strSynthRunName);
  QString getRunSynthRun(const QString &strRunName) const;
  // Please set currentrun before using this function, Unless you used
  // setSynthRun/setImpleRun before
  int setSynthesisOption(const QList<QPair<QString, QString>> &listParam);
  QString getSynthOption(const QString &optionName) const;

  int setRunActive(const QString &strRunName);

  QString getActiveRunDevice() const;
  QString getActiveSynthRunName() const;
  QString getActiveImpleRunName() const;
  QString getRunType(const QString &strRunName) const;

  int deleteFileSet(const QString &strSetName);
  int deleteRun(const QString &strRunName);

  void FinishedProject();

  QString currentFileSet() const;
  void setCurrentFileSet(const QString &currentFileSet);

  QString getCurrentRun() const;
  void setCurrentRun(const QString &currentRun);

  const std::vector<std::string> &includePathList() const;
  QString includePath() const;
  void setIncludePathList(const std::vector<std::string> &newIncludePathList);
  void addIncludePath(const std::string &includePath);

  const std::vector<std::string> &libraryPathList() const;
  QString libraryPath() const;
  void setLibraryPathList(const std::vector<std::string> &newLibraryPathList);
  void addLibraryPath(const std::string &libraryPath);

  const std::vector<std::string> &libraryExtensionList() const;
  QString libraryExtension() const;
  void setLibraryExtensionList(
      const std::vector<std::string> &newLibraryExtensionList);
  void addLibraryExtension(const std::string &libraryExt);

  void setMacroList(
      const std::vector<std::pair<std::string, std::string>> &newMacroList);
  void addMacro(const std::string &macroName, const std::string &macroValue);
  const std::vector<std::pair<std::string, std::string>> &macroList() const;
  QString macros() const;

  const std::vector<std::string> &simLibraryExtensionList() const;
  QString simLibraryExtension() const;
  void setSimLibraryExtensionList(
      const std::vector<std::string> &newLibraryExtensionList);
  void addSimLibraryExtension(const std::string &libraryExt);

  void setSimMacroList(
      const std::vector<std::pair<std::string, std::string>> &newMacroList);
  void addSimMacro(const std::string &macroName, const std::string &macroValue);
  const std::vector<std::pair<std::string, std::string>> &macroListSim() const;
  QString macrosSim() const;

  const std::vector<std::string> &includePathListSim() const;
  QString includePathSim() const;
  void setIncludePathListSim(
      const std::vector<std::string> &newIncludePathList);
  void addIncludePathSim(const std::string &includePath);

  const std::vector<std::string> &libraryPathListSim() const;
  QString libraryPathSim() const;
  void setLibraryPathListSim(
      const std::vector<std::string> &newLibraryPathList);
  void addLibraryPathSim(const std::string &libraryPath);

  void setTargetDevice(const std::string &deviceName);
  void setTargetDeviceData(const std::string &family, const std::string &series,
                           const std::string &package);
  std::string getTargetDevice();
  static std::vector<std::pair<std::string, std::string>> ParseMacro(
      const QString &macro);

  const std::vector<std::string> &ipCatalogPathList() const;
  QString ipCatalogPaths() const;
  void setIpCatalogPathList(
      const std::vector<std::string> &newIpCatalogPathList);
  void addIpCatalogPath(const std::string &ipCatalogPath);

  const std::vector<std::string> &ipInstancePathList() const;
  QString ipInstancePaths() const;
  void setIpInstancePathList(
      const std::vector<std::string> &newIpInstancePathList);
  void addIpInstancePath(const std::string &ipInstancePath);

  const std::vector<std::string> &ipInstanceCmdList() const;
  QString ipInstanceCmds() const;
  void setIpInstanceCmdList(
      const std::vector<std::string> &newIpInstanceCmdList);
  void addIpInstanceCmd(const std::string &ipInstanceCmd);

  using AddFileFunction =
      std::function<void(const QString &, const QString &, const QString &, int,
                         const QString &, bool, bool)>;
  static void AddFiles(const ProjectOptions::FileData &fileData,
                       const AddFileFunction &addFileFunction);

 private:
  // Please set currentfileset before using this function
  int setDesignFile(const QString &strFileName, bool isFileCopy = true,
                    bool localToProject = true);

  int CreateProjectDir();
  int CreateSrcsFolder(QString strFolderName);
  int CreateRunsFolder(QString strFolderName);

  int CreateVerilogFile(QString strFile);
  int CreateSystemVerilogFile(QString strFile);
  int CreateVHDLFile(QString strFile);
  int CreateSDCFile(QString strFile);
  int CreateCFile(const QString &strFile);

  int AddOrCreateFileToFileSet(const QString &strFileName,
                               bool isFileCopy = true);

  QStringList getAllChildFiles(QString path);
  bool CopyFileToPath(QString sourceDir, QString destinDir,
                      bool iscover = true);
  int CreateAndAddFile(const QString &suffix, const QString &filename,
                       const QString &filenameAdd, bool copyFile);
  void UpdateProjectInternal(const ProjectOptions &opt, bool setTargetConstr);

 private:
  QString m_currentFileSet;
  QString m_currentRun;
  inline static const Suffixes m_designSuffixes{
      {"v", "sv", "vh", "svh", "vhd", "blif", "eblif"}};
  inline static const Suffixes m_constrSuffixes{{"sdc", "pin"}};
  inline static const Suffixes m_simSuffixes{
      {"v", "sv", "cpp", "c", "cc", "vhd", "vhdl"}};

 signals:
  void projectPathChanged();
  void saveFile();
};

}  // namespace FOEDAG
std::ostream &operator<<(std::ostream &out, const QString &text);
#endif  // PROJECTMANAGER_H
