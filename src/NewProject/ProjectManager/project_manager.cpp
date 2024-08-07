#include "project_manager.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QXmlStreamWriter>
#include <filesystem>
#include <iostream>

#include "Compiler/CompilerDefines.h"
#include "DesignFileWatcher.h"
#include "MainWindow/Session.h"
#include "Utils/FileUtils.h"
#include "Utils/QtUtils.h"
#include "Utils/StringUtils.h"
#include "Utils/sequential_map.h"

extern FOEDAG::Session* GlobalSession;

using namespace FOEDAG;

ProjectManager::ProjectManager(QObject* parent) : QObject(parent) {
  // Re-emit projectPathChanged signals
  QObject::connect(Project::Instance(), &Project::projectPathChanged, this,
                   &ProjectManager::projectPathChanged);
  QObject::connect(this, &ProjectManager::saveFile, Project::Instance(),
                   &Project::saveFile);
}

void ProjectManager::CreateProject(const ProjectOptions& opt) {
  if (opt.rewriteProject) {
    std::filesystem::path tmpPath = opt.projectPath.toStdString();
    std::string projectName = opt.projectName.toStdString();
    std::filesystem::path osprFile =
        tmpPath / std::string(projectName + ".ospr");
    std::filesystem::remove(osprFile);
    std::filesystem::path srcsDir = projectSrcsPath(projectName);
    std::filesystem::remove_all(srcsDir);
    // DO NOT REMOVE FILES BLINDLY, COMPILATION RESULTS GET SAVED IN THE SAME
    // PROJECT DIR
    // if (dir.exists()) dir.removeRecursively();
  }
  CreateProject(opt.projectName, opt.projectPath, opt.currentFileSet);

  setProjectType(opt.projectType);
  UpdateProjectInternal(opt, true);

  DesignFileWatcher::Instance()->emitDesignCreated();
}

void ProjectManager::UpdateProject(const ProjectOptions& opt) {
  deleteFileSet(DEFAULT_FOLDER_SOURCE);
  setDesignFileSet(DEFAULT_FOLDER_SOURCE);

  deleteFileSet(DEFAULT_FOLDER_CONSTRS);
  setConstrFileSet(DEFAULT_FOLDER_CONSTRS);

  deleteFileSet(DEFAULT_FOLDER_SIM);
  setSimulationFileSet(DEFAULT_FOLDER_SIM);

  UpdateProjectInternal(opt, false);
  DesignFileWatcher::Instance()->updateDesignFileWatchers(this);
}

QString ProjectManager::ProjectFilesPath(const QString& projPath,
                                         const QString& projName,
                                         const QString& fileSet,
                                         const QString& file) {
  return QString::fromStdString(
      ProjectFilesPath(projPath.toStdString(), projName.toStdString(),
                       fileSet.toStdString(), file.toStdString())
          .string());
}

std::filesystem::path ProjectManager::ProjectFilesPath(
    const std::string& projPath, const std::string& projName,
    const std::string& fileSet, const std::string& file) {
  std::filesystem::path p = projectSrcsPath(projPath, projName);
  p = p / fileSet;
  if (!file.empty()) p = p / file;
  return p;
}

int ProjectManager::CreateProject(const QString& strName,
                                  const QString& strPath,
                                  const QString& designSource) {
  int ret = 0;
  if ("" == strName || "" == strPath) {
    return -1;
  }
  Project::Instance()->InitProject();
  Project::Instance()->setProjectName(strName);
  Project::Instance()->setProjectPath(strPath);
  ret = CreateProjectDir();
  if (0 != ret) {
    return ret;
  }

  setDesignFileSet(designSource);

  setConstrFileSet(DEFAULT_FOLDER_CONSTRS);

  ret = setSimulationFileSet(DEFAULT_FOLDER_SIM);

  ProjectRun proImpRun;
  proImpRun.setRunName(DEFAULT_FOLDER_IMPLE);
  proImpRun.setRunType(RUN_TYPE_IMPLEMENT);
  proImpRun.setSrcSet(designSource);
  proImpRun.setConstrsSet(DEFAULT_FOLDER_CONSTRS);
  proImpRun.setRunState(RUN_STATE_CURRENT);
  proImpRun.setSynthRun(DEFAULT_FOLDER_SYNTH);
  Project::Instance()->setProjectRun(proImpRun);

  ProjectRun proSynRun;
  proSynRun.setRunName(DEFAULT_FOLDER_SYNTH);
  proSynRun.setRunType(RUN_TYPE_SYNTHESIS);
  proSynRun.setSrcSet(designSource);
  proSynRun.setConstrsSet(DEFAULT_FOLDER_CONSTRS);
  proSynRun.setRunState(RUN_STATE_CURRENT);
  proSynRun.setOption(PROJECT_RUN_OPTION_FLOW, "Classic Flow");
  proSynRun.setOption(PROJECT_RUN_OPTION_LANGUAGEVER, "SYSTEMVERILOG_2005");
  proSynRun.setOption(PROJECT_RUN_OPTION_TARGETLANG, "VERILOG");

  Project::Instance()->setProjectRun(proSynRun);

  ProjectConfiguration* projectConfig = Project::Instance()->projectConfig();
  projectConfig->setActiveSimSet(DEFAULT_FOLDER_SIM);

  return ret;
}

QString ProjectManager::getProjectName() const {
  return Project::Instance()->projectName();
}

std::string ProjectManager::projectName() const {
  return Project::Instance()->projectName().toStdString();
}

QString ProjectManager::getProjectPath() const {
  return Project::Instance()->projectPath();
}

std::string ProjectManager::projectPath() const {
  return getProjectPath().toStdString();
}

std::filesystem::path ProjectManager::projectBasePath(
    const std::string& projectPath) {
  fs::path base{fs::path{projectPath}};
  base /= "run_1";
  return base;
}

std::filesystem::path ProjectManager::projectSrcsPath(
    const std::string& projectPath, const std::string& projectName) {
  fs::path base{projectPath};
  base = base / projectSrcsPath(projectName);
  return base;
}

std::filesystem::path ProjectManager::projectSrcsPath(
    const QString& projectPath, const QString& projectName) {
  return projectSrcsPath(projectPath.toStdString(), projectName.toStdString());
}

std::filesystem::path ProjectManager::projectSrcsPath(
    const std::string& projectName) {
  fs::path base{projectBasePath({})};
  base /= StringUtils::format("%.srcs", projectName);
  return base;
}

std::filesystem::path ProjectManager::projectSrcsPath(
    const QString& projectName) {
  return projectSrcsPath(projectName.toStdString());
}

std::filesystem::path ProjectManager::projectSynthSettingsPath(
    const std::string& projectPath) {
  auto synth = synthPath(projectPath);
  return synth / StringUtils::format("%_settings", synth.filename().string());
}

std::filesystem::path ProjectManager::projectImplSettingsPath(
    const std::string& projectPath) {
  auto impl = implPath(projectPath);
  return impl / StringUtils::format("%_settings", impl.filename().string());
}

std::filesystem::path ProjectManager::projectSettingsPath(
    const std::string& projectPath) {
  return projectBasePath(projectPath) / "settings";
}

std::filesystem::path ProjectManager::synthPath(
    const std::string& projectPath) {
  return projectBasePath(projectPath) / "synth_1_1";
}

std::filesystem::path ProjectManager::implPath(const std::string& projectPath) {
  return synthPath(projectPath) / "impl_1_1_1";
}

std::filesystem::path ProjectManager::projectIPsPath(
    const std::string& projectPath) {
  return projectBasePath(projectPath) / "IPs";
}

bool ProjectManager::HasDesign() const { return !getProjectName().isEmpty(); }

int ProjectManager::setProjectType(int strType) {
  int ret = 0;
  ProjectConfiguration* projectConfig = Project::Instance()->projectConfig();
  projectConfig->setProjectType(strType);
  return ret;
}

ProjectType ProjectManager::projectType() const {
  ProjectConfiguration* projectConfig = Project::Instance()->projectConfig();
  return projectConfig ? static_cast<ProjectType>(projectConfig->projectType())
                       : RTL;
}

ProjectManager::ErrorInfo ProjectManager::addFiles(
    const QString& commands, const QString& libs, const QStringList& fileNames,
    int lang, const QString& grName, bool isFileCopy, bool localToProject) {
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) return {EC_FileSetNotExist};

  const QStringList commandsList = QtUtils::StringSplit(commands, ' ');
  const QStringList libsList = QtUtils::StringSplit(libs, ' ');

  // check file exists
  QStringList notExistingFiles;
  for (const auto& file : fileNames) {
    if (const QFileInfo fileInfo{file}; !fileInfo.exists())
      notExistingFiles.append(file);
  }
  if (!notExistingFiles.isEmpty())
    return {EC_FileNotExist, notExistingFiles.join(", ")};
  proFileSet->addFiles(commandsList, libsList, fileNames, lang, grName);

  auto result{EC_Success};
  for (const auto& file : fileNames) {
    const int res = setDesignFile(file, isFileCopy, false);
    if (res != EC_Success) result = static_cast<ErrorCode>(res);
  }
  return {result};
}

ProjectManager::ErrorInfo ProjectManager::addDesignFiles(
    const QString& commands, const QString& libs, const QStringList& fileNames,
    int lang, const QString& grName, bool isFileCopy, bool localToProject) {
  setCurrentFileSet(getDesignActiveFileSet());
  return addFiles(commands, libs, fileNames, lang, grName, isFileCopy,
                  localToProject);
}

ProjectManager::ErrorInfo ProjectManager::addSimulationFiles(
    const QString& commands, const QString& libs, const QStringList& fileNames,
    int lang, const QString& grName, bool isFileCopy, bool localToProject) {
  setCurrentFileSet(getSimulationActiveFileSet());
  return addFiles(commands, libs, fileNames, lang, grName, isFileCopy,
                  localToProject);
}

QString ProjectManager::getDefaulUnitName() const {
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) return QString{};
  return proFileSet->getDefaultUnitName();
}

int ProjectManager::setDesignFiles(const QString& commands, const QString& libs,
                                   const QStringList& fileNames, int lang,
                                   const QString& grName, bool isFileCopy,
                                   bool localToProject,
                                   const QStringList& override) {
  setCurrentFileSet(getDesignActiveFileSet());
  auto res = setFiles(commands, libs, fileNames, lang, grName, isFileCopy,
                      localToProject);
  if (res != EC_Success) return res;

  int result{EC_Success};
  for (const auto& file : fileNames) {
    int res = setDesignFile(file, isFileCopy, localToProject,
                            override.contains(file));
    if (res != EC_Success) result = res;
  }
  return result;
}

int ProjectManager::setFiles(const QString& commands, const QString& libs,
                             const QStringList& fileList, int lang,
                             const QString& grName, bool isFileCopy,
                             bool localToProject) {
  const QStringList commandsList = QtUtils::StringSplit(commands, ' ');
  const QStringList libsList = QtUtils::StringSplit(libs, ' ');

  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) {
    return EC_FileSetNotExist;
  }

  if (localToProject) {
    const auto path =
        ProjectFilesPath(Project::Instance()->projectPath(),
                         Project::Instance()->projectName(), m_currentFileSet);
    QStringList fullPathFileList;
    for (const auto& file : fileList) {
      fullPathFileList.append(QtUtils::CreatePath(path, file));
    }
    proFileSet->addFiles(commandsList, libsList, fullPathFileList, lang,
                         grName);
  } else {
    if (isFileCopy) {
      QStringList localFileList;
      for (const auto& file : fileList) {
        const QFileInfo info{file};
        localFileList.append(
            ProjectFilesPath(getProjectPath(), getProjectName(),
                             m_currentFileSet, info.fileName()));
      }
      proFileSet->addFiles(commandsList, libsList, localFileList, lang, grName);
    } else {
      proFileSet->addFiles(commandsList, libsList, fileList, lang, grName);
    }
  }
  return EC_Success;
}

int ProjectManager::setSimulationFiles(const QString& commands,
                                       const QString& libs,
                                       const QStringList& fileNames, int lang,
                                       const QString& grName, bool isFileCopy,
                                       bool localToProject,
                                       const QStringList& override) {
  setCurrentFileSet(getSimulationActiveFileSet());
  auto res = setFiles(commands, libs, fileNames, lang, grName, isFileCopy,
                      localToProject);
  if (res != EC_Success) return res;

  int result{0};
  for (const auto& file : fileNames) {
    int res = setSimulationFile(file, isFileCopy, localToProject,
                                override.contains(file));
    if (res != 0) result = res;
  }
  return result;
}

int ProjectManager::setDesignFile(const QString& strFileName, bool isFileCopy,
                                  bool localToProject, bool override) {
  int ret = 0;
  QFileInfo fileInfo(strFileName);
  const auto localPath =
      ProjectFilesPath(Project::Instance()->projectPath(),
                       Project::Instance()->projectName(), m_currentFileSet);
  if (localToProject) {
    fileInfo.setFile(localPath, strFileName);
  }
  if (override) {
    QDir localDir{localPath};
    if (localDir.entryList().contains(fileInfo.fileName())) {
      if (FileUtils::removeFile(fileInfo.absoluteFilePath().toStdString())) {
        if (localToProject)
          fileInfo.setFile(localPath, strFileName);
        else
          fileInfo.setFile(strFileName);
      }
    }
  }
  QString suffix = fileInfo.suffix();
  if (fileInfo.isDir()) {
    QStringList fileList = getAllChildFiles(strFileName);
    foreach (QString strfile, fileList) {
      ret = AddOrCreateFileToFileSet(strfile, isFileCopy);
    }
  } else if (fileInfo.exists()) {
    ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
  } else {
    if (strFileName.contains("/")) {
      if (m_designSuffixes.TestSuffix(suffix)) {
        ret = CreateAndAddFile(suffix, strFileName, strFileName, isFileCopy);
      }
    } else {
      QString filePath = ProjectFilesPath(Project::Instance()->projectPath(),
                                          Project::Instance()->projectName(),
                                          m_currentFileSet) +
                         "/" + strFileName;
      QString fileSetPath =
          ProjectFilesPath(PROJECT_OSRCDIR, Project::Instance()->projectName(),
                           m_currentFileSet);
      fileSetPath += "/" + strFileName;
      if (m_designSuffixes.TestSuffix(suffix)) {
        ret = CreateAndAddFile(suffix, filePath, fileSetPath, false);
      }
    }
  }
  return ret;
}

int ProjectManager::setSimulationFile(const QString& strFileName,
                                      bool isFileCopy, bool localToProject,
                                      bool override) {
  int ret = 0;
  QFileInfo fileInfo(strFileName);
  const auto localPath =
      ProjectFilesPath(Project::Instance()->projectPath(),
                       Project::Instance()->projectName(), m_currentFileSet);
  if (localToProject) {
    fileInfo.setFile(localPath, strFileName);
  }
  if (override) {
    QDir localDir{localPath};
    if (localDir.entryList().contains(fileInfo.fileName())) {
      if (FileUtils::removeFile(fileInfo.absoluteFilePath().toStdString())) {
        if (localToProject)
          fileInfo.setFile(localPath, strFileName);
        else
          fileInfo.setFile(strFileName);
      }
    }
  }
  QString suffix = fileInfo.suffix();
  if (fileInfo.isDir()) {
    QStringList fileList = getAllChildFiles(strFileName);
    foreach (QString strfile, fileList) {
      suffix = QFileInfo(strfile).suffix();
      if (m_simSuffixes.TestSuffix(suffix)) {
        ret = AddOrCreateFileToFileSet(strfile, isFileCopy);
      }
    }
  } else if (fileInfo.exists()) {
    if (m_simSuffixes.TestSuffix(suffix)) {
      ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
    }
  } else {
    if (strFileName.contains("/")) {
      if (m_simSuffixes.TestSuffix(suffix)) {
        ret = CreateAndAddFile(suffix, strFileName, strFileName, isFileCopy);
      }
    } else {
      QString filePath = ProjectFilesPath(Project::Instance()->projectPath(),
                                          Project::Instance()->projectName(),
                                          m_currentFileSet) +
                         "/" + strFileName;
      QString fileSetPath =
          ProjectFilesPath(PROJECT_OSRCDIR, Project::Instance()->projectName(),
                           m_currentFileSet);
      fileSetPath += "/" + strFileName;
      if (m_simSuffixes.TestSuffix(suffix)) {
        ret = CreateAndAddFile(suffix, filePath, fileSetPath, false);
      }
    }
  }
  return ret;
}

int ProjectManager::addConstrsFile(const QString& strFileName, bool isFileCopy,
                                   bool localToProject, bool override) {
  // check file exists
  if (const QFileInfo fileInfo{strFileName}; !fileInfo.exists())
    return EC_FileNotExist;
  return setConstrsFile(strFileName, isFileCopy, localToProject, override);
}

int ProjectManager::setConstrsFile(const QString& strFileName, bool isFileCopy,
                                   bool localToProject, bool override) {
  int ret = 0;
  QFileInfo fileInfo(strFileName);
  const auto localPath =
      ProjectFilesPath(Project::Instance()->projectPath(),
                       Project::Instance()->projectName(), m_currentFileSet);
  if (localToProject) {
    fileInfo.setFile(localPath, strFileName);
  }
  if (override) {
    QDir localDir{localPath};
    if (localDir.entryList().contains(fileInfo.fileName())) {
      if (FileUtils::removeFile(fileInfo.absoluteFilePath().toStdString())) {
        if (localToProject)
          fileInfo.setFile(localPath, strFileName);
        else
          fileInfo.setFile(strFileName);
      }
    }
  }
  QString suffix = fileInfo.suffix();
  if (fileInfo.isDir()) {
    QStringList fileList = getAllChildFiles(strFileName);
    foreach (QString strfile, fileList) {
      suffix = QFileInfo(strfile).suffix();
      if (m_constrSuffixes.TestSuffix(suffix)) {
        ret = AddOrCreateFileToFileSet(strfile, isFileCopy);
      }
    }
  } else if (fileInfo.exists()) {
    if (m_constrSuffixes.TestSuffix(suffix)) {
      ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
    }
  } else {
    if (strFileName.contains("/")) {
      if (m_constrSuffixes.TestSuffix(suffix)) {
        ret = CreateAndAddFile(suffix, strFileName, strFileName, false);
      }
    } else {
      QString filePath = ProjectFilesPath(Project::Instance()->projectPath(),
                                          Project::Instance()->projectName(),
                                          m_currentFileSet) +
                         "/" + strFileName;
      QString fileSetPath =
          ProjectFilesPath(PROJECT_OSRCDIR, Project::Instance()->projectName(),
                           m_currentFileSet);
      fileSetPath += "/" + strFileName;
      if (m_constrSuffixes.TestSuffix(suffix)) {
        ret = CreateAndAddFile(suffix, filePath, fileSetPath, false);
      }
    }
  }
  return ret;
}

int ProjectManager::deleteFile(const QString& strFileName) {
  int ret = 0;
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) {
    return -1;
  }
  // target file cannot be deleted
  if (strFileName == proFileSet->getOption(PROJECT_FILE_CONFIG_TARGET)) {
    return -1;
  }

  proFileSet->deleteFile(strFileName);
  DesignFileWatcher::Instance()->updateDesignFileWatchers(this);
  return ret;
}

int ProjectManager::setTopModule(const QString& strModuleName) {
  int ret = 0;
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) {
    return -1;
  }

  proFileSet->setOption(PROJECT_FILE_CONFIG_TOP, strModuleName);
  return ret;
}

ProjectManager::ErrorCode ProjectManager::setTopModuleSim(
    const QString& strModuleName) {
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) {
    return EC_FileSetNotExist;
  }

  proFileSet->setOption(PROJECT_FILE_CONFIG_TOP, strModuleName);
  return EC_Success;
}

int ProjectManager::setTopModuleLibrary(const QString& strModuleNameLib) {
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) {
    return EC_FileSetNotExist;
  }

  proFileSet->setOption(PROJECT_FILE_CONFIG_TOP_LIB, strModuleNameLib);
  return EC_Success;
}

int ProjectManager::setTargetConstrs(const QString& strFileName) {
  int ret = 0;
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) {
    return -1;
  }

  QString strFilePath = proFileSet->getFilePath(strFileName);
  if ("" == strFilePath) {
    // the file is not exist.
    return -1;
  }

  proFileSet->setOption(PROJECT_FILE_CONFIG_TARGET, strFileName);
  return ret;
}

int ProjectManager::setDesignFileSet(const QString& strSetName) {
  int ret = 0;
  ret = CreateSrcsFolder(strSetName);
  if (0 != ret) {
    return ret;
  }

  ProjectFileSet proFileSet;
  proFileSet.setSetName(strSetName);
  proFileSet.setSetType(PROJECT_FILE_TYPE_DS);
  proFileSet.setRelSrcDir(QU::ToQString(StringUtils::buildPath(
      projectSrcsPath(Project::Instance()->projectName()),
      strSetName.toStdString())));
  ret = Project::Instance()->setProjectFileset(proFileSet);
  return ret;
}

QStringList ProjectManager::getDesignFileSets() const {
  QStringList retList;

  QMap<QString, ProjectFileSet*> tmpFileSetMap =
      Project::Instance()->getMapProjectFileset();

  for (auto iter = tmpFileSetMap.begin(); iter != tmpFileSetMap.end(); ++iter) {
    ProjectFileSet* tmpFileSet = iter.value();
    if (tmpFileSet && PROJECT_FILE_TYPE_DS == tmpFileSet->getSetType()) {
      retList.append(tmpFileSet->getSetName());
    }
  }
  return retList;
}

QString ProjectManager::getDesignActiveFileSet() const {
  QString strActive = "";

  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();

  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun && RUN_STATE_CURRENT == tmpRun->runState() &&
        RUN_TYPE_SYNTHESIS == tmpRun->runType()) {
      strActive = tmpRun->srcSet();
      break;
    }
  }
  return strActive;
}

int ProjectManager::setDesignActive(const QString& strSetName) {
  int ret = -1;
  if ("" == strSetName) {
    return ret;
  }

  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(strSetName);
  if (nullptr == proFileSet) {
    // the set is not exist.
    return ret;
  }

  if (proFileSet->getSetType() != PROJECT_FILE_TYPE_DS) {
    // the set is not design file set.
    return ret;
  }

  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();

  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun && RUN_STATE_CURRENT == tmpRun->runState()) {
      tmpRun->setSrcSet(strSetName);
      ret = 0;
    }
  }
  return ret;
}

QStringList ProjectManager::getDesignFiles(const QString& strFileSet) const {
  QStringList strList;

  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(strFileSet);

  if (tmpFileSet && PROJECT_FILE_TYPE_DS == tmpFileSet->getSetType()) {
    auto tmpMapFiles = tmpFileSet->getMapFiles();
    for (auto iter = tmpMapFiles.begin(); iter != tmpMapFiles.end(); ++iter) {
      strList.append(iter->second);
    }
  }
  return strList;
}

QStringList ProjectManager::getDesignFiles() const {
  return getDesignFiles(getDesignActiveFileSet());
}

ProjectManager::CompilationUnits ProjectManager::DesignFiles() const {
  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(getDesignActiveFileSet());

  CompilationUnits vec;
  if (tmpFileSet && PROJECT_FILE_TYPE_DS == tmpFileSet->getSetType()) {
    auto tmpMapFiles = tmpFileSet->Files();
    for (auto iter = tmpMapFiles.begin(); iter != tmpMapFiles.end(); ++iter) {
      vec.push_back(
          std::make_pair(iter->first, iter->second.join(" ").toStdString()));
    }
  }
  return vec;
}

ProjectManager::Libraries ProjectManager::DesignLibraries() const {
  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(getDesignActiveFileSet());

  std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>>
      result;
  for (const auto& commandsLibs : tmpFileSet->getLibraries()) {
    std::vector<std::string> commands;
    std::vector<std::string> libs;
    for (auto i = 0; i < commandsLibs.first.size(); ++i) {
      commands.push_back(commandsLibs.first[i].toStdString());
      libs.push_back(commandsLibs.second[i].toStdString());
    }
    result.emplace_back(std::move(commands), std::move(libs));
  }

  return result;
}

ProjectManager::Libraries ProjectManager::SimulationLibraries() const {
  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(getSimulationActiveFileSet());

  std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>>
      result;
  for (const auto& commandsLibs : tmpFileSet->getLibraries()) {
    std::vector<std::string> commands;
    std::vector<std::string> libs;
    for (auto i = 0; i < commandsLibs.first.size(); ++i) {
      commands.push_back(commandsLibs.first[i].toStdString());
      libs.push_back(commandsLibs.second[i].toStdString());
    }
    result.emplace_back(std::move(commands), std::move(libs));
  }

  return result;
}

ProjectManager::CompilationUnits ProjectManager::SimulationFiles() const {
  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(getSimulationActiveFileSet());

  CompilationUnits vec;
  if (tmpFileSet && PROJECT_FILE_TYPE_SS == tmpFileSet->getSetType()) {
    auto tmpMapFiles = tmpFileSet->Files();
    for (auto iter = tmpMapFiles.begin(); iter != tmpMapFiles.end(); ++iter) {
      vec.push_back(
          std::make_pair(iter->first, iter->second.join(" ").toStdString()));
    }
  }
  return vec;
}

std::vector<std::pair<CompilationUnit, std::vector<std::string>>>
ProjectManager::SimulationFileList() const {
  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(getSimulationActiveFileSet());

  std::vector<std::pair<CompilationUnit, std::vector<std::string>>> vec;
  if (tmpFileSet && PROJECT_FILE_TYPE_SS == tmpFileSet->getSetType()) {
    auto tmpMapFiles = tmpFileSet->Files();
    for (auto iter = tmpMapFiles.cbegin(); iter != tmpMapFiles.cend(); ++iter) {
      std::vector<std::string> files;
      for (const auto& f : iter->second) files.push_back(f.toStdString());
      vec.push_back(std::make_pair(iter->first, files));
    }
  }
  return vec;
}

QString ProjectManager::getDesignTopModule(const QString& strFileSet) const {
  QString strTopModule;

  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(strFileSet);

  if (tmpFileSet && PROJECT_FILE_TYPE_DS == tmpFileSet->getSetType()) {
    strTopModule = tmpFileSet->getOption(PROJECT_FILE_CONFIG_TOP);
  }
  return strTopModule;
}

QString ProjectManager::getDesignTopModule() const {
  return getDesignTopModule(getDesignActiveFileSet());
}

std::string ProjectManager::DesignTopModule() const {
  return getDesignTopModule().toStdString();
}

QString ProjectManager::getSimulationTopModule() const {
  return getSimulationTopModule(getSimulationActiveFileSet());
}

std::string ProjectManager::SimulationTopModule() const {
  return getSimulationTopModule().toStdString();
}

QString ProjectManager::getDesignTopModuleLib(const QString& strFileSet) const {
  QString strTopModuleLib;

  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(strFileSet);

  if (tmpFileSet && PROJECT_FILE_TYPE_DS == tmpFileSet->getSetType()) {
    strTopModuleLib = tmpFileSet->getOption(PROJECT_FILE_CONFIG_TOP_LIB);
  }
  return strTopModuleLib;
}

QString ProjectManager::getDesignTopModuleLib() const {
  return getDesignTopModuleLib(getDesignActiveFileSet());
}

std::string ProjectManager::DesignTopModuleLib() const {
  return getDesignTopModuleLib().toStdString();
}

QString ProjectManager::getSimulationTopModuleLib(
    const QString& strFileSet) const {
  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(strFileSet);

  if (tmpFileSet && PROJECT_FILE_TYPE_SS == tmpFileSet->getSetType()) {
    return tmpFileSet->getOption(PROJECT_FILE_CONFIG_TOP_LIB);
  }
  return QString{};
}

QString ProjectManager::getSimulationTopModuleLib() const {
  return getSimulationTopModuleLib(getSimulationActiveFileSet());
}

std::string ProjectManager::SimulationTopModuleLib() const {
  return getSimulationTopModuleLib().toStdString();
}

int ProjectManager::setConstrFileSet(const QString& strSetName) {
  int ret = 0;
  ret = CreateSrcsFolder(strSetName);
  if (0 != ret) {
    return ret;
  }

  ProjectFileSet proFileSet;
  proFileSet.setSetName(strSetName);
  proFileSet.setSetType(PROJECT_FILE_TYPE_CS);
  proFileSet.setRelSrcDir(QU::ToQString(StringUtils::buildPath(
      projectSrcsPath(Project::Instance()->projectName()),
      strSetName.toStdString())));
  ret = Project::Instance()->setProjectFileset(proFileSet);

  return ret;
}

QStringList ProjectManager::getConstrFileSets() const {
  QStringList retList;

  QMap<QString, ProjectFileSet*> tmpFileSetMap =
      Project::Instance()->getMapProjectFileset();

  for (auto iter = tmpFileSetMap.begin(); iter != tmpFileSetMap.end(); ++iter) {
    ProjectFileSet* tmpFileSet = iter.value();
    if (tmpFileSet && PROJECT_FILE_TYPE_CS == tmpFileSet->getSetType()) {
      retList.append(tmpFileSet->getSetName());
    }
  }
  return retList;
}

QString ProjectManager::getConstrActiveFileSet() const {
  QString strActive = "";

  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();

  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun && RUN_STATE_CURRENT == tmpRun->runState() &&
        RUN_TYPE_SYNTHESIS == tmpRun->runType()) {
      strActive = tmpRun->constrsSet();
      break;
    }
  }
  return strActive;
}

int ProjectManager::setConstrActive(const QString& strSetName) {
  int ret = -1;
  if ("" == strSetName) {
    return ret;
  }

  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(strSetName);
  if (nullptr == proFileSet) {
    // the set is not exist.
    return ret;
  }

  if (proFileSet->getSetType() != PROJECT_FILE_TYPE_CS) {
    // the set is not constraints file set.
    return ret;
  }

  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();

  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun && RUN_STATE_CURRENT == tmpRun->runState()) {
      tmpRun->setConstrsSet(strSetName);
      ret = 0;
    }
  }
  return ret;
}

QStringList ProjectManager::getConstrFiles(const QString& strFileSet) const {
  QStringList strList;

  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(strFileSet);

  if (tmpFileSet && PROJECT_FILE_TYPE_CS == tmpFileSet->getSetType()) {
    auto tmpMapFiles = tmpFileSet->getMapFiles();
    for (auto iter = tmpMapFiles.begin(); iter != tmpMapFiles.end(); ++iter) {
      strList.append(iter->second);
    }
  }
  return strList;
}

QString ProjectManager::getConstrTargetFile(const QString& strFileSet) const {
  QString strTargetFile;

  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(strFileSet);

  if (tmpFileSet && PROJECT_FILE_TYPE_CS == tmpFileSet->getSetType()) {
    strTargetFile = tmpFileSet->getOption(PROJECT_FILE_CONFIG_TARGET);
  }
  return strTargetFile;
}

std::vector<std::string> ProjectManager::getConstrFiles() const {
  std::vector<std::string> files;
  for (const auto& set : getConstrFileSets()) {
    for (const auto& file : getConstrFiles(set)) {
      QString f{file};
      f.replace(PROJECT_OSRCDIR, Project::Instance()->projectPath());
      files.push_back(f.toStdString());
    }
  }
  return files;
}

std::string ProjectManager::getConstrPinFile() const {
  const Suffixes pin{{"pin"}};
  auto constraints = getConstrFiles();
  for (const auto& c : constraints) {
    const QFileInfo info{QString::fromStdString(c)};
    if (pin.TestSuffix(info.suffix())) return c;
  }
  return std::string{};
}

int ProjectManager::setSimulationFileSet(const QString& strSetName) {
  int ret = 0;
  ret = CreateSrcsFolder(strSetName);
  if (0 != ret) {
    return ret;
  }

  ProjectFileSet proFileSet;
  proFileSet.setSetName(strSetName);
  proFileSet.setSetType(PROJECT_FILE_TYPE_SS);
  proFileSet.setRelSrcDir(QU::ToQString(StringUtils::buildPath(
      projectSrcsPath(Project::Instance()->projectName()),
      strSetName.toStdString())));
  ret = Project::Instance()->setProjectFileset(proFileSet);

  return ret;
}

QStringList ProjectManager::getSimulationFileSets() const {
  QStringList retList;

  QMap<QString, ProjectFileSet*> tmpFileSetMap =
      Project::Instance()->getMapProjectFileset();

  for (auto iter = tmpFileSetMap.begin(); iter != tmpFileSetMap.end(); ++iter) {
    ProjectFileSet* tmpFileSet = iter.value();
    if (tmpFileSet && PROJECT_FILE_TYPE_SS == tmpFileSet->getSetType()) {
      retList.append(tmpFileSet->getSetName());
    }
  }
  return retList;
}

QString ProjectManager::getSimulationActiveFileSet() const {
  QString strActive{};

  ProjectConfiguration* tmpProCfg = Project::Instance()->projectConfig();
  if (tmpProCfg) {
    strActive = tmpProCfg->activeSimSet();
  }
  return strActive;
}

int ProjectManager::setSimulationActive(const QString& strSetName) {
  int ret = 0;
  if ("" == strSetName) {
    return -1;
  }

  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(strSetName);
  if (nullptr == proFileSet) {
    // the set is not exist.
    return ret;
  }

  if (proFileSet->getSetType() != PROJECT_FILE_TYPE_SS) {
    // the set is not simulation file set.
    return ret;
  }

  ProjectConfiguration* tmpProCfg = Project::Instance()->projectConfig();
  if (tmpProCfg) {
    tmpProCfg->setActiveSimSet(strSetName);
  }
  return ret;
}

QStringList ProjectManager::getSimulationFiles(
    const QString& strFileSet) const {
  QStringList strList;

  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(strFileSet);

  if (tmpFileSet && PROJECT_FILE_TYPE_SS == tmpFileSet->getSetType()) {
    auto tmpMapFiles = tmpFileSet->getMapFiles();
    for (auto iter = tmpMapFiles.begin(); iter != tmpMapFiles.end(); ++iter) {
      strList.append(iter->second);
    }
  }
  return strList;
}

QString ProjectManager::getSimulationTopModule(
    const QString& strFileSet) const {
  QString strTopModule = "";

  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(strFileSet);

  if (tmpFileSet && PROJECT_FILE_TYPE_SS == tmpFileSet->getSetType()) {
    strTopModule = tmpFileSet->getOption(PROJECT_FILE_CONFIG_TOP);
  }
  return strTopModule;
}

QStringList ProjectManager::getSynthRunsNames() const {
  QStringList listSynthRunNames;
  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();
  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun && RUN_TYPE_SYNTHESIS == tmpRun->runType()) {
      listSynthRunNames.append(tmpRun->runName());
    }
  }
  return listSynthRunNames;
}

QStringList ProjectManager::getImpleRunsNames() const {
  QStringList listImpleRunNames;
  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();
  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun && RUN_TYPE_IMPLEMENT == tmpRun->runType()) {
      listImpleRunNames.append(tmpRun->runName());
    }
  }
  return listImpleRunNames;
}

QStringList ProjectManager::ImpleUsedSynth(const QString& strSynthName) const {
  QStringList listImpleRunNames;
  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();
  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun && RUN_TYPE_IMPLEMENT == tmpRun->runType() &&
        strSynthName == tmpRun->synthRun()) {
      listImpleRunNames.append(tmpRun->runName());
    }
  }

  return listImpleRunNames;
}

QList<QPair<QString, QString>> ProjectManager::getRunsProperties(
    const QString& strRunName) const {
  QList<QPair<QString, QString>> listProperties;
  ProjectRun* proRun = Project::Instance()->getProjectRun(strRunName);
  if (nullptr != proRun) {
    QPair<QString, QString> pair;
    pair.first = PROJECT_RUN_NAME;
    pair.second = proRun->runName();
    listProperties.append(pair);
    pair.first = PROJECT_RUN_TYPE;
    pair.second = proRun->runType();
    listProperties.append(pair);
    pair.first = PROJECT_RUN_SRCSET;
    pair.second = proRun->srcSet();
    listProperties.append(pair);
    pair.first = PROJECT_RUN_CONSTRSSET;
    pair.second = proRun->constrsSet();
    listProperties.append(pair);
    pair.first = PROJECT_RUN_STATE;
    pair.second = proRun->runState();
    listProperties.append(pair);
    pair.first = PROJECT_RUN_SYNTHRUN;
    pair.second = proRun->synthRun();
    listProperties.append(pair);
    QMap<QString, QString> mapOption = proRun->getMapOption();
    QMap<QString, QString>::iterator iter = mapOption.begin();
    while (iter != mapOption.end()) {
      pair.first = iter.key();
      pair.second = iter.value();
      listProperties.append(pair);
      iter++;
    }
  }

  return listProperties;
}

int ProjectManager::setSynthRun(const QString& strRunName) {
  int ret = 0;
  ProjectRun proRun;
  proRun.setRunName(strRunName);
  proRun.setRunType(RUN_TYPE_SYNTHESIS);
  proRun.setOption(PROJECT_RUN_OPTION_FLOW, "Classic Flow");
  proRun.setOption(PROJECT_RUN_OPTION_LANGUAGEVER, "SYSTEMVERILOG_2005");
  proRun.setOption(PROJECT_RUN_OPTION_TARGETLANG, "VERILOG");
  ret = Project::Instance()->setProjectRun(proRun);
  if (0 == ret) {
    CreateRunsFolder(strRunName);
    m_currentRun = strRunName;
  }
  return ret;
}

int ProjectManager::setImpleRun(const QString& strRunName) {
  int ret = 0;
  ProjectRun proRun;
  proRun.setRunName(strRunName);
  proRun.setRunType(RUN_TYPE_IMPLEMENT);
  ret = Project::Instance()->setProjectRun(proRun);
  if (0 == ret) {
    CreateRunsFolder(strRunName);
    m_currentRun = strRunName;
  }
  return ret;
}

int ProjectManager::setRunSrcSet(const QString& strSrcSet) {
  int ret = 0;
  ProjectRun* proRun = Project::Instance()->getProjectRun(m_currentRun);
  if (nullptr == proRun) {
    return -2;
  }
  proRun->setSrcSet(strSrcSet);
  return ret;
}

QString ProjectManager::getRunSrcSet(const QString& strRunName) const {
  QString strSrcSet = "";
  ProjectRun* proRun = Project::Instance()->getProjectRun(strRunName);
  if (nullptr != proRun) {
    strSrcSet = proRun->srcSet();
  }
  return strSrcSet;
}

int ProjectManager::setRunConstrSet(const QString& strConstrSet) {
  int ret = 0;
  ProjectRun* proRun = Project::Instance()->getProjectRun(m_currentRun);
  if (nullptr == proRun) {
    return -2;
  }
  proRun->setConstrsSet(strConstrSet);
  return ret;
}

QString ProjectManager::getRunConstrSet(const QString& strRunName) const {
  QString strConstrSet = "";
  ProjectRun* proRun = Project::Instance()->getProjectRun(strRunName);
  if (nullptr != proRun) {
    strConstrSet = proRun->constrsSet();
  }
  return strConstrSet;
}

int ProjectManager::setRunSynthRun(const QString& strSynthRunName) {
  int ret = 0;
  ProjectRun* proRun = Project::Instance()->getProjectRun(m_currentRun);
  if (nullptr == proRun) {
    return -2;
  }
  if (proRun->runType() == RUN_TYPE_IMPLEMENT) {
    proRun->setSynthRun(strSynthRunName);
  }
  return ret;
}

QString ProjectManager::getRunSynthRun(const QString& strRunName) const {
  QString strSynthRun = "";
  ProjectRun* proRun = Project::Instance()->getProjectRun(strRunName);
  if (nullptr != proRun) {
    strSynthRun = proRun->synthRun();
  }
  return strSynthRun;
}
int ProjectManager::setSynthesisOption(
    const QList<QPair<QString, QString>>& listParam) {
  int ret = 0;
  ProjectRun* proRun = Project::Instance()->getProjectRun(m_currentRun);
  if (nullptr == proRun) {
    return -2;
  }
  for (auto& pair : listParam) {
    proRun->setOption(pair.first, pair.second);
  }
  return ret;
}

QString ProjectManager::getSynthOption(const QString& optionName) const {
  ProjectRun* proRun = Project::Instance()->getProjectRun(m_currentRun);
  if (nullptr == proRun) {
    return {};
  }
  return proRun->getOption(optionName);
}

int ProjectManager::setRunActive(const QString& strRunName) {
  int ret = 0;

  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();
  // clear all runs state
  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun) {
      tmpRun->setRunState("");
    }
  }

  ProjectRun* proRun = Project::Instance()->getProjectRun(strRunName);
  if (nullptr == proRun) {
    return -2;
  }
  proRun->setRunState(RUN_STATE_CURRENT);

  // The synthesis used will become active
  if (proRun->runType() == RUN_TYPE_IMPLEMENT) {
    QString strSynthRunName = proRun->synthRun();
    ProjectRun* proSynthRun =
        Project::Instance()->getProjectRun(strSynthRunName);
    if (nullptr == proSynthRun) {
      return -2;
    }
    proSynthRun->setRunState(RUN_STATE_CURRENT);
  }
  return ret;
}

QString ProjectManager::getActiveRunDevice() const {
  QString strActive = "";

  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();

  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun && RUN_STATE_CURRENT == tmpRun->runState() &&
        RUN_TYPE_SYNTHESIS == tmpRun->runType()) {
      strActive = tmpRun->getOption(PROJECT_PART_DEVICE);
      break;
    }
  }
  return strActive;
}

QString ProjectManager::getActiveSynthRunName() const {
  QString strActive = "";

  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();

  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun && RUN_STATE_CURRENT == tmpRun->runState() &&
        RUN_TYPE_SYNTHESIS == tmpRun->runType()) {
      strActive = tmpRun->runName();
      break;
    }
  }
  return strActive;
}

QString ProjectManager::getActiveImpleRunName() const {
  QString strActive = "";

  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();

  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    if (tmpRun && RUN_STATE_CURRENT == tmpRun->runState() &&
        RUN_TYPE_IMPLEMENT == tmpRun->runType()) {
      strActive = tmpRun->runName();
      break;
    }
  }
  return strActive;
}

QString ProjectManager::getRunType(const QString& strRunName) const {
  QString strType = "";
  ProjectRun* proRun = Project::Instance()->getProjectRun(strRunName);
  if (nullptr != proRun) {
    strType = proRun->runType();
  }
  return strType;
}

int ProjectManager::deleteFileSet(const QString& strSetName) {
  Project::Instance()->deleteProjectFileset(strSetName);
  return 0;
}

int ProjectManager::deleteRun(const QString& strRunName) {
  int ret = 0;
  ProjectRun* proRun = Project::Instance()->getProjectRun(strRunName);
  if (RUN_STATE_CURRENT == proRun->runState()) {
    return -1;
  }

  // Check whether there is an implementation using synthesis's result. If so,
  // delete the implementation run
  if (RUN_TYPE_SYNTHESIS == proRun->runType()) {
    QMap<QString, ProjectRun*> tmpRunMap =
        Project::Instance()->getMapProjectRun();
    for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
      ProjectRun* tmpRun = iter.value();
      if (tmpRun && RUN_TYPE_IMPLEMENT == tmpRun->runType() &&
          strRunName == tmpRun->synthRun()) {
        Project::Instance()->deleteprojectRun(tmpRun->runName());
      }
    }
  }
  Project::Instance()->deleteprojectRun(strRunName);
  return ret;
}

void ProjectManager::FinishedProject() { emit saveFile(); }

int ProjectManager::CreateProjectDir() {
  int ret = 0;
  do {
    QString tmpName = Project::Instance()->projectName();
    QString tmpPath = Project::Instance()->projectPath();

    if (tmpName.isEmpty() || tmpPath.isEmpty()) {
      ret = -1;
      break;
    }

    QDir dir(tmpPath);
    if (!dir.exists()) {
      if (!dir.mkpath(tmpPath)) {
        ret = -2;
        break;
      }
    }

    if (!dir.mkpath(QU::ToQString(projectSrcsPath(tmpPath, tmpName)))) {
      ret = -2;
      break;
    }
  } while (false);

  return ret;
}

int ProjectManager::CreateSrcsFolder(QString strFolderName) {
  int ret = 0;
  do {
    QString tmpName = Project::Instance()->projectName();
    QString tmpPath = Project::Instance()->projectPath();

    if ("" == tmpName || "" == tmpPath || "" == strFolderName) {
      ret = -1;
      break;
    }

    auto srcPath = projectSrcsPath(tmpPath, tmpName);
    srcPath /= strFolderName.toStdString();
    QString strPath = QU::ToQString(srcPath);
    QDir dir(strPath);
    if (!dir.exists()) {
      if (!dir.mkpath(strPath)) {
        ret = -2;
        break;
      }
    }
  } while (false);
  return ret;
}

int ProjectManager::CreateRunsFolder(QString strFolderName) {
  int ret = 0;
  do {
    QString tmpName = Project::Instance()->projectName();
    QString tmpPath = Project::Instance()->projectPath();

    if ("" == tmpName || "" == tmpPath || "" == strFolderName) {
      ret = -1;
      break;
    }

    QString strPath = tmpPath + "/" + tmpName + ".runs/" + strFolderName;
    QDir dir(strPath);
    if (!dir.exists()) {
      if (!dir.mkpath(strPath)) {
        ret = -2;
        break;
      }
    }
  } while (false);
  return ret;
}

int ProjectManager::CreateVerilogFile(QString strFile) {
  int ret = 0;
  QFile file(strFile);
  if (file.exists()) return ret;
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    return -1;
  }
  file.close();
  return ret;
}

int ProjectManager::CreateSystemVerilogFile(QString strFile) {
  int ret = 0;
  QFile file(strFile);
  if (file.exists()) return ret;
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    return -1;
  }
  file.close();
  return ret;
}

int ProjectManager::CreateVHDLFile(QString strFile) {
  int ret = 0;
  QFile file(strFile);
  if (file.exists()) return ret;
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    QTextStream out(stdout);
    out << "failed:" << strFile << "\n";
    return -1;
  }
  file.close();
  return ret;
}

int ProjectManager::CreateSDCFile(QString strFile) {
  int ret = 0;
  QFile file(strFile);
  if (file.exists()) return ret;
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    return -1;
  }
  file.close();
  return ret;
}

int ProjectManager::CreateCFile(const QString& strFile) {
  int ret = 0;
  QFile file(strFile);
  if (file.exists()) return ret;
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    return -1;
  }
  file.close();
  return ret;
}

int ProjectManager::AddOrCreateFileToFileSet(const QString& strFileName,
                                             bool isFileCopy) {
  int ret = 0;
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) {
    return -1;
  }

  QFileInfo fileInfo(strFileName);
  QString fname = fileInfo.fileName();
  if (isFileCopy) {
    auto filePath =
        SU::buildPath(projectSrcsPath(Project::Instance()->projectName()),
                      m_currentFileSet.toStdString(), fname.toStdString());
    QString destinDir = QU::ToQString(SU::buildPath(projectPath(), filePath));
    if (CopyFileToPath(strFileName, destinDir)) {
      proFileSet->addFile(
          fname, QU::ToQString(SU::buildPath(PROJECT_OSRCDIR, filePath)));
    } else {
      ret = -2;
    }
  } else {
    proFileSet->addFile(fname, strFileName);
  }
  return ret;
}

QStringList ProjectManager::getAllChildFiles(QString path) {
  QStringList resultFileList;
  if (path == "") {
    return resultFileList;
  }

  QDir sourceDir(path);
  QFileInfoList fileInfoList = sourceDir.entryInfoList();
  foreach (QFileInfo fileInfo, fileInfoList) {
    if (fileInfo.fileName() == "." || fileInfo.fileName() == "..") continue;
    if (fileInfo.isDir()) continue;
    resultFileList.push_back(path + "/" + fileInfo.fileName());
  }
  return resultFileList;
}

bool ProjectManager::CopyFileToPath(QString sourceDir, QString destinDir,
                                    bool iscover) {
  destinDir.replace("\\", "/");
  if (sourceDir == destinDir) {
    return true;
  }
  if (!QFile::exists(sourceDir)) {
    return false;
  }

  QDir* createfile = new QDir;
  bool exist = createfile->exists(destinDir);
  if (exist) {
    if (iscover) {
      createfile->remove(destinDir);
    }
  }

  if (!QFile::copy(sourceDir, destinDir)) {
    return false;
  }
  return true;
}

std::vector<std::pair<std::string, std::string>> ProjectManager::ParseMacro(
    const QString& macro) {
  std::vector<std::pair<std::string, std::string>> macroList;
  const QStringList data = QtUtils::StringSplit(macro, ' ');
  for (const auto& d : data) {
    auto splitted = QtUtils::StringSplit(d, '=');
    if (!splitted.isEmpty())
      macroList.push_back(std::make_pair(
          splitted.first().toStdString(),
          splitted.count() > 1 ? splitted.at(1).toStdString() : std::string{}));
  }
  return macroList;
}

const std::vector<std::string>& ProjectManager::ipCatalogPathList() const {
  return Project::Instance()->ipConfig()->ipCatalogPathList();
}

QString ProjectManager::ipCatalogPaths() const {
  auto pathList = Project::Instance()->ipConfig()->ipCatalogPathList();
  QStringList tmpList;
  for (const auto& p : pathList) tmpList.append(QString::fromStdString(p));
  return tmpList.join(" ");
}

void ProjectManager::setIpCatalogPathList(
    const std::vector<std::string>& newIpCatalogPathList) {
  Project::Instance()->ipConfig()->setIpCatalogPathList(newIpCatalogPathList);
};

void ProjectManager::addIpCatalogPath(const std::string& ipCatalogPath) {
  Project::Instance()->ipConfig()->addIpCatalogPath(ipCatalogPath);
}

const std::vector<std::string>& ProjectManager::ipInstancePathList() const {
  return Project::Instance()->ipConfig()->instancePathList();
}

QString ProjectManager::ipInstancePaths() const {
  auto pathList = Project::Instance()->ipConfig()->instancePathList();
  QStringList tmpList;
  for (const auto& p : pathList) tmpList.append(QString::fromStdString(p));
  return tmpList.join(" ");
}

void ProjectManager::setIpInstancePathList(
    const std::vector<std::string>& newIpInstancePathList) {
  Project::Instance()->ipConfig()->setInstancePathList(newIpInstancePathList);
};

void ProjectManager::addIpInstancePath(const std::string& ipInstancePath) {
  Project::Instance()->ipConfig()->addInstancePath(ipInstancePath);
}

const std::vector<std::string>& ProjectManager::ipInstanceCmdList() const {
  return Project::Instance()->ipConfig()->instanceCmdList();
}

QString ProjectManager::ipInstanceCmds() const {
  auto cmdList = Project::Instance()->ipConfig()->instanceCmdList();
  QStringList tmpList;
  for (const auto& p : cmdList) tmpList.append(QString::fromStdString(p));
  return tmpList.join("_IP_CMD_SEP_");
}

void ProjectManager::setIpInstanceCmdList(
    const std::vector<std::string>& newIpInstanceCmdList) {
  Project::Instance()->ipConfig()->setInstanceCmdList(newIpInstanceCmdList);
  emit saveFile();
};

void ProjectManager::addIpInstanceCmd(const std::string& ipInstanceCmd) {
  Project::Instance()->ipConfig()->addInstanceCmd(ipInstanceCmd);
  emit saveFile();
}

int ProjectManager::CreateAndAddFile(const QString& suffix,
                                     const QString& filename,
                                     const QString& filenameAdd,
                                     bool copyFile) {
  int ret{-1};
  if (!suffix.compare("v", Qt::CaseInsensitive)) {
    ret = CreateVerilogFile(filename);
  } else if (!suffix.compare("sv", Qt::CaseInsensitive)) {
    ret = CreateSystemVerilogFile(filename);
  } else if (!suffix.compare("vh", Qt::CaseInsensitive)) {
    ret = CreateSystemVerilogFile(filename);
  } else if (!suffix.compare("svh", Qt::CaseInsensitive)) {
    ret = CreateSystemVerilogFile(filename);
  } else if (!suffix.compare("blif", Qt::CaseInsensitive)) {
    ret = CreateSystemVerilogFile(filename);
  } else if (!suffix.compare("eblif", Qt::CaseInsensitive)) {
    ret = CreateSystemVerilogFile(filename);
  } else if (!suffix.compare("vhd", Qt::CaseInsensitive)) {
    ret = CreateVHDLFile(filename);
  } else if (!suffix.compare("sdc", Qt::CaseInsensitive)) {
    ret = CreateSDCFile(filename);
  } else if (!suffix.compare("pin", Qt::CaseInsensitive)) {
    ret = CreateSDCFile(filename);
  } else if (suffix.compare("cpp", Qt::CaseInsensitive) == 0 ||
             suffix.compare("c", Qt::CaseInsensitive) == 0) {
    ret = CreateCFile(filename);
  }
  if (0 == ret) {
    ret = AddOrCreateFileToFileSet(filenameAdd, copyFile);
  }
  return ret;
}

void ProjectManager::UpdateProjectInternal(const ProjectOptions& opt,
                                           bool setTargetConstr) {
  setCurrentFileSet(opt.currentFileSet);
  auto addDesignFiles = [this](const QString& commands, const QString& libs,
                               const QStringList& fileNames, int lang,
                               const QString& grName, bool isFileCopy,
                               bool localToProject,
                               const QStringList& override) {
    setDesignFiles(commands, libs, fileNames, lang, grName, isFileCopy,
                   localToProject, override);
  };

  AddFiles(opt.sourceFileData, addDesignFiles);

  setTopModule(opt.designOptions.topModule);
  setTopModuleLibrary(opt.designOptions.topModuleLib);

  const std::string delimiter = " ";
  // Set Library Extensions
  std::vector<std::string> ext;
  StringUtils::tokenize(opt.designOptions.libraryExtList.toStdString(),
                        delimiter, ext);
  setLibraryExtensionList(ext);

  // Set Library Paths
  std::vector<std::string> tokens;
  StringUtils::tokenize(opt.designOptions.libraryPathList.toStdString(),
                        delimiter, tokens);
  setLibraryPathList(tokens);

  // Set Include Paths
  std::vector<std::string> inc;
  StringUtils::tokenize(opt.designOptions.includePathList.toStdString(),
                        delimiter, inc);
  setIncludePathList(inc);

  // Set Macros
  auto macro = ParseMacro(opt.designOptions.macroList);
  setMacroList(macro);

  setCurrentFileSet(DEFAULT_FOLDER_CONSTRS);
  QString strDefaultCts;
  const auto constr = opt.constrFileData.fileData;
  for (const filedata& fdata : constr) {
    if (LocalToProject == fdata.m_filePath) {
      setConstrsFile(fdata.m_fileName, false, true, fdata.override);
    } else {
      setConstrsFile(fdata.m_filePath + "/" + fdata.m_fileName,
                     opt.constrFileData.isCopySource, true, fdata.override);
    }
    strDefaultCts = fdata.m_fileName;
  }

  if (!strDefaultCts.isEmpty() && setTargetConstr) {
    setTargetConstrs(strDefaultCts);
  }

  // simulation
  setCurrentFileSet(DEFAULT_FOLDER_SIM);
  auto addSimFiles = [this](const QString& commands, const QString& libs,
                            const QStringList& fileNames, int lang,
                            const QString& grName, bool isFileCopy,
                            bool localToProject, const QStringList& override) {
    setSimulationFiles(commands, libs, fileNames, lang, grName, isFileCopy,
                       localToProject, override);
  };

  AddFiles(opt.simFileData, addSimFiles);
  setTopModuleSim(opt.simulationOptions.topModule);
  setTopModuleLibrary(opt.simulationOptions.topModuleLib);

  ext.clear();
  StringUtils::tokenize(opt.simulationOptions.libraryExtList.toStdString(),
                        delimiter, ext);
  setSimLibraryExtensionList(ext);
  // Set Macros
  macro = ParseMacro(opt.simulationOptions.macroList);
  setSimMacroList(macro);
  // Set Library Paths
  tokens.clear();
  StringUtils::tokenize(opt.simulationOptions.libraryPathList.toStdString(),
                        delimiter, tokens);
  setLibraryPathListSim(tokens);

  // Set Include Paths
  inc.clear();
  StringUtils::tokenize(opt.simulationOptions.includePathList.toStdString(),
                        delimiter, inc);
  setIncludePathListSim(inc);

  // -------------------------------------------------------------------------

  setCurrentRun(DEFAULT_FOLDER_SYNTH);

  if (opt.device.count() >= 4) {
    QStringList strlist = opt.device;
    QList<QPair<QString, QString>> listParam;
    QPair<QString, QString> pair;
    pair.first = PROJECT_PART_SERIES;
    pair.second = strlist.at(0);
    listParam.append(pair);
    pair.first = PROJECT_PART_FAMILY;
    pair.second = strlist.at(1);
    listParam.append(pair);
    pair.first = PROJECT_PART_PACKAGE;
    pair.second = strlist.at(2);
    listParam.append(pair);
    pair.first = PROJECT_PART_DEVICE;
    pair.second = strlist.at(3);
    listParam.append(pair);
    setSynthesisOption(listParam);

    auto targetDevice = strlist.at(3);
    target_device(targetDevice);
  }

  FinishedProject();
}

void ProjectManager::AddFiles(const ProjectOptions::FileData& fileData,
                              const AddFileFunction& addFileFunction) {
  const QList<filedata> listFile = fileData.fileData;

  // Group and add files to project based off m_groupName
  sequential_multi_map<QString, QList<filedata>> fileGroups{};
  QStringList overrideFiles{};
  for (const filedata& fdata : listFile) {
    if (fdata.m_groupName.isEmpty())
      fileGroups.push_back(std::make_pair(fdata.m_groupName, QList{fdata}));
    else
      fileGroups[fdata.m_groupName].append(fdata);
    if (fdata.override) overrideFiles.push_back(fdata.m_fileName);
  }

  // Step through the group key and try to combine all the settings
  for (const auto& it : fileGroups.values()) {
    const auto key = it.first;
    if (key.isEmpty()) {
      // Use non-grouping project file logic for any file that didn't set a
      // group/compileUnit
      const QList<filedata> noGroupFiles = it.second;
      for (const filedata& fdata : noGroupFiles) {
        auto libraries = fdata.m_workLibrary;
        auto command = libraries.isEmpty() ? QString() : "-work";

        if (LocalToProject == fdata.m_filePath) {
          addFileFunction(command, libraries, {fdata.m_fileName},
                          fdata.m_language, QString{}, false, true,
                          overrideFiles);
        } else {
          addFileFunction(
              command, libraries,
              {QtUtils::CreatePath(fdata.m_filePath, fdata.m_fileName)},
              fdata.m_language, QString{}, fileData.isCopySource, false,
              overrideFiles);
        }
      }
      continue;
    }
    sequential_multi_map<QString, QStringList> fileListStr{};
    QStringList libs{};
    int language = -1;
    bool hasLocalFiles = false;
    bool hasNonLocalFiles = false;
    bool multipleLanguages = false;

    // Loop through each file in this specific group
    const auto files = fileGroups[key];
    for (auto& fdata : files) {
      QString addFilePath{};

      // Track the language
      if (language == -1) {
        language = fdata.m_language;
      } else {
        if (language != fdata.m_language) {
          multipleLanguages = true;
        }
      }

      // Split libraries by space and then store any new, unique library names
      const auto libList = fdata.m_workLibrary.split(" ");
      for (const auto& lib : libList) {
        if (!libs.contains(lib)) {
          libs.append(lib);
        }
      }

      // Handle local and non-local paths
      if (LocalToProject == fdata.m_filePath) {
        hasLocalFiles = true;
        addFilePath = fdata.m_fileName;
      } else {
        hasNonLocalFiles = true;
        addFilePath = fdata.m_filePath + "/" + fdata.m_fileName;
      }

      // Add the file to the list
      fileListStr[key].emplace_back(addFilePath);
    }  // End looping through files

    // create a string of the unique libs requested by the files in this
    // group
    QString libraries{};
    for (const auto& lib : libs) {
      if (!libraries.isEmpty()) {
        libraries += " ";
      }
      libraries += lib;
    }
    auto command = libraries.isEmpty() ? QString{} : "-work";

    // Check if we are combing local and non-local files in a group
    if (hasLocalFiles && hasNonLocalFiles) {
      std::cerr << "different arguements when local or non-local" << std::endl;
      // This is probably an error condition as the setDesignFiles call
      // has different arguements when local or non-local
    } else if (multipleLanguages) {
      std::cerr << "Multiple languages" << std::endl;
      // This seems like a pontential error scenario as well
    } else if (hasLocalFiles) {
      addFileFunction(command, libraries, fileListStr[key], language, key,
                      false, true, overrideFiles);
    } else {
      addFileFunction(command, libraries, fileListStr[key], language, key,
                      fileData.isCopySource, false, overrideFiles);
    }
  }
}

const std::vector<std::string>& ProjectManager::libraryPathList() const {
  return Project::Instance()->compilerConfig()->libraryPathList();
}

QString ProjectManager::libraryPath() const {
  auto pathList = Project::Instance()->compilerConfig()->libraryPathList();
  QStringList tmpList;
  for (const auto& p : pathList) tmpList.append(QString::fromStdString(p));
  return tmpList.join(" ");
}

void ProjectManager::setLibraryPathList(
    const std::vector<std::string>& newLibraryPathList) {
  Project::Instance()->compilerConfig()->setLibraryPathList(newLibraryPathList);
}

void ProjectManager::addLibraryPath(const std::string& libraryPath) {
  Project::Instance()->compilerConfig()->addLibraryPath(libraryPath);
}

const std::vector<std::string>& ProjectManager::libraryExtensionList() const {
  return Project::Instance()->compilerConfig()->libraryExtensionList();
}

QString ProjectManager::libraryExtension() const {
  auto extList = Project::Instance()->compilerConfig()->libraryExtensionList();
  QStringList tmpList;
  for (const auto& ext : extList) tmpList.append(QString::fromStdString(ext));
  return tmpList.join(" ");
}

void ProjectManager::setLibraryExtensionList(
    const std::vector<std::string>& newLibraryExtensionList) {
  Project::Instance()->compilerConfig()->setLibraryExtensionList(
      newLibraryExtensionList);
}

void ProjectManager::addLibraryExtension(const std::string& libraryExt) {
  Project::Instance()->compilerConfig()->addLibraryExtension(libraryExt);
}

void ProjectManager::setMacroList(
    const std::vector<std::pair<std::string, std::string>>& newMacroList) {
  Project::Instance()->compilerConfig()->setMacroList(newMacroList);
}

void ProjectManager::addMacro(const std::string& macroName,
                              const std::string& macroValue) {
  Project::Instance()->compilerConfig()->addMacro(macroName, macroValue);
}

const std::vector<std::pair<std::string, std::string>>&
ProjectManager::macroList() const {
  return Project::Instance()->compilerConfig()->macroList();
}

QString ProjectManager::macros() const {
  auto macro = Project::Instance()->compilerConfig()->macroList();
  QStringList tmpList;
  for (const auto& m : macro)
    tmpList.append(QString::fromStdString(m.first + "=" + m.second));
  return tmpList.join(" ");
}

const std::vector<std::string>& ProjectManager::simLibraryExtensionList()
    const {
  return Project::Instance()->simulationConfig()->libraryExtensionList();
}

QString ProjectManager::simLibraryExtension() const {
  auto extList =
      Project::Instance()->simulationConfig()->libraryExtensionList();
  QStringList tmpList;
  for (const auto& ext : extList) tmpList.append(QString::fromStdString(ext));
  return tmpList.join(" ");
}

void ProjectManager::setSimLibraryExtensionList(
    const std::vector<std::string>& newLibraryExtensionList) {
  Project::Instance()->simulationConfig()->setLibraryExtensionList(
      newLibraryExtensionList);
}

void ProjectManager::addSimLibraryExtension(const std::string& libraryExt) {
  Project::Instance()->simulationConfig()->addLibraryExtension(libraryExt);
}

void ProjectManager::setSimMacroList(
    const std::vector<std::pair<std::string, std::string>>& newMacroList) {
  Project::Instance()->simulationConfig()->setMacroList(newMacroList);
}

void ProjectManager::addSimMacro(const std::string& macroName,
                                 const std::string& macroValue) {
  Project::Instance()->simulationConfig()->addMacro(macroName, macroValue);
}

const std::vector<std::pair<std::string, std::string>>&
ProjectManager::macroListSim() const {
  return Project::Instance()->simulationConfig()->macroList();
}

QString ProjectManager::macrosSim() const {
  auto macro = Project::Instance()->simulationConfig()->macroList();
  QStringList tmpList;
  for (const auto& m : macro)
    tmpList.append(QString::fromStdString(m.first + "=" + m.second));
  return tmpList.join(" ");
}

const std::vector<std::string>& ProjectManager::includePathListSim() const {
  return Project::Instance()->simulationConfig()->includePathList();
}

QString ProjectManager::includePathSim() const {
  auto pathList = Project::Instance()->simulationConfig()->includePathList();
  QStringList tmpList;
  for (const auto& p : pathList) tmpList.append(QString::fromStdString(p));
  return tmpList.join(" ");
}

void ProjectManager::setIncludePathListSim(
    const std::vector<std::string>& newIncludePathList) {
  Project::Instance()->simulationConfig()->setIncludePathList(
      newIncludePathList);
}

void ProjectManager::addIncludePathSim(const std::string& includePath) {
  Project::Instance()->simulationConfig()->addIncludePath(includePath);
}

const std::vector<std::string>& ProjectManager::libraryPathListSim() const {
  return Project::Instance()->simulationConfig()->libraryPathList();
}

QString ProjectManager::libraryPathSim() const {
  auto pathList = Project::Instance()->simulationConfig()->libraryPathList();
  QStringList tmpList;
  for (const auto& p : pathList) tmpList.append(QString::fromStdString(p));
  return tmpList.join(" ");
}

void ProjectManager::setLibraryPathListSim(
    const std::vector<std::string>& newLibraryPathList) {
  Project::Instance()->simulationConfig()->setLibraryPathList(
      newLibraryPathList);
}

void ProjectManager::addLibraryPathSim(const std::string& libraryPath) {
  Project::Instance()->simulationConfig()->addLibraryPath(libraryPath);
}

void ProjectManager::setTargetDevice(const std::string& deviceName) {
  setCurrentRun(getActiveSynthRunName());
  auto result = setSynthesisOption(
      {{PROJECT_PART_DEVICE, QString::fromStdString(deviceName)}});
  if (result != 0)
    std::cerr << "setSynthesisOption(): something went wrong, return value is: "
              << result
              << std::endl;  // TODO @volodymyrk backlog,logging improve
  emit saveFile();
}

void ProjectManager::setTargetDeviceData(const std::string& family,
                                         const std::string& series,
                                         const std::string& package) {
  setCurrentRun(getActiveSynthRunName());
  auto result = setSynthesisOption({
      {PROJECT_PART_SERIES, QString::fromStdString(series)},
      {PROJECT_PART_FAMILY, QString::fromStdString(family)},
      {PROJECT_PART_PACKAGE, QString::fromStdString(package)},
  });
  if (result != 0)
    std::cerr << "setSynthesisOption(): something went wrong, return value is: "
              << result
              << std::endl;  // TODO @volodymyrk backlog,logging improve
  emit saveFile();
}

std::string ProjectManager::getTargetDevice() {
  setCurrentRun(getActiveSynthRunName());
  return getSynthOption(PROJECT_PART_DEVICE).toStdString();
}

const std::vector<std::string>& ProjectManager::includePathList() const {
  return Project::Instance()->compilerConfig()->includePathList();
}

QString ProjectManager::includePath() const {
  auto includeList = Project::Instance()->compilerConfig()->includePathList();
  QStringList tmpList;
  for (const auto& i : includeList) tmpList.append(QString::fromStdString(i));
  return tmpList.join(" ");
}

void ProjectManager::setIncludePathList(
    const std::vector<std::string>& newIncludePathList) {
  Project::Instance()->compilerConfig()->setIncludePathList(newIncludePathList);
}

void ProjectManager::addIncludePath(const std::string& includePath) {
  Project::Instance()->compilerConfig()->addIncludePath(includePath);
}

QString ProjectManager::getCurrentRun() const { return m_currentRun; }

void ProjectManager::setCurrentRun(const QString& currentRun) {
  m_currentRun = currentRun;
}

QString ProjectManager::currentFileSet() const { return m_currentFileSet; }

void ProjectManager::setCurrentFileSet(const QString& currentFileSet) {
  m_currentFileSet = currentFileSet;
}

std::ostream& operator<<(std::ostream& out, const QString& text) {
  out << text.toStdString();
  return out;
}

bool Suffixes::TestSuffix(const QString& s) const {
  for (const auto& suffix : suffixes)
    if (!suffix.compare(s, Qt::CaseInsensitive)) return true;
  return false;
}
