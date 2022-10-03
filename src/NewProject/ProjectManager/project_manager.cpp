#include "project_manager.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QXmlStreamWriter>
#include <filesystem>
#include <iostream>

#include "Compiler/CompilerDefines.h"
#include "MainWindow/Session.h"
#include "Utils/StringUtils.h"

extern FOEDAG::Session* GlobalSession;

using namespace FOEDAG;
constexpr auto LocalToProject{"<Local to Project>"};

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
    std::filesystem::path srcsDir =
        tmpPath / std::string(projectName + ".srcs");
    std::filesystem::remove_all(srcsDir);
    std::filesystem::path runsDir =
        tmpPath / std::string(projectName + ".runs");
    std::filesystem::remove_all(runsDir);
    // DO NOT REMOVE FILES BLINDLY, COMPILATION RESULTS GET SAVED IN THE SAME
    // PROJECT DIR
    // if (dir.exists()) dir.removeRecursively();
  }
  CreateProject(opt.projectName, opt.projectPath, opt.currentFileSet);

  setProjectType(opt.projectType);

  setCurrentFileSet(opt.currentFileSet);
  const QList<filedata> listFile = opt.sourceFileData.fileData;
  for (const filedata& fdata : listFile) {
    auto libraries = fdata.m_workLibrary;
    auto command = libraries.isEmpty() ? QString() : "-work";

    if (LocalToProject == fdata.m_filePath) {
      setDesignFiles(command, libraries, fdata.m_fileName, fdata.m_language,
                     false, true);
    } else {
      setDesignFiles(command, libraries,
                     fdata.m_filePath + "/" + fdata.m_fileName,
                     fdata.m_language, opt.sourceFileData.isCopySource, false);
    }
  }

  setTopModule(opt.topModule);
  setTopModuleLibrary(opt.topModuleLib);

  std::string delimiter = " ";
  // Set Library Paths
  if (!opt.libraryPathList.isEmpty()) {
    std::vector<std::string> tokens;
    StringUtils::tokenize(opt.libraryPathList.toStdString(), delimiter, tokens);
    for (auto file : tokens) {
      std::string cmd = "add_library_path " + file;
      GlobalSession->CmdStack()->push_and_exec(new Command(cmd));
    }
  }

  // Set Library Extensions
  if (!opt.libraryExtList.isEmpty()) {
    std::string cmd = "add_library_ext " + opt.libraryExtList.toStdString();
    GlobalSession->CmdStack()->push_and_exec(new Command(cmd));
  }

  // Set Include Paths
  if (!opt.includePathList.isEmpty()) {
    std::vector<std::string> tokens;
    StringUtils::tokenize(opt.includePathList.toStdString(), delimiter, tokens);
    for (auto file : tokens) {
      std::string cmd = "add_include_path " + file;
      GlobalSession->CmdStack()->push_and_exec(new Command(cmd));
    }
  }

  // Set Macros
  if (!opt.macroList.isEmpty()) {
    std::string cmd = "set_macro " + opt.macroList.toStdString();
    GlobalSession->CmdStack()->push_and_exec(new Command(cmd));
  }

  setCurrentFileSet(DEFAULT_FOLDER_CONSTRS);
  QString strDefaultCts;
  const auto constr = opt.constrFileData.fileData;
  for (const filedata& fdata : constr) {
    if (LocalToProject == fdata.m_filePath) {
      setConstrsFile(fdata.m_fileName, false);
    } else {
      setConstrsFile(fdata.m_filePath + "/" + fdata.m_fileName,
                     opt.constrFileData.isCopySource);
    }
    strDefaultCts = fdata.m_fileName;
  }

  if (!strDefaultCts.isEmpty()) {
    setTargetConstrs(strDefaultCts);
  }

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
  std::filesystem::path p = projPath;
  auto folder = projName + ".srcs";
  p = p / folder / fileSet;
  if (!file.empty()) p = p / file;
  return p;
}

void ProjectManager::Tcl_CreateProject(int argc, const char* argv[]) {
  QTextStream out(stdout);
  if (argc < 3 || "--file" != QString(argv[1])) {
    out << "-----------create_project ------------\n";
    out << " \n";
    out << " Description: \n";
    out << " Create a new project. \n";
    out << " \n";
    out << " Syntax: \n";
    out << " create_project --file <filename.xml> \n";
    out << " \n";
    out << "--------------------------------------\n";
    return;
  }

  QFileInfo fileInfo;
  fileInfo.setFile(QString(argv[2]));
  if (fileInfo.exists()) {
    if (0 == CreateProjectbyXml(QString(argv[2]))) {
      emit saveFile();
      out << "Project created successfully! \n";
    }
  } else {
    out << " Warning : This file <" << QString(argv[2]) << "> is not exist! \n";
  }
}

int ProjectManager::CreateProjectbyXml(const QString& strProXMl) {
  /**xml format ***********
  <project type="RTL" name="project_1" path="/root">
      <device name="x7c100t"/>
      <sources>
          <source file="/counter.v" is_top="true"/>
           … more <source>
      </sources>
      <constraints>
          <constraint file="/counter.sdc"/>
           … more <constraint>
      </constraints>
      <testbenches>
          <testbench file="/counter_tb.v"/>
          … more <testbench>
      </testbenches>
  </project>
   ****************************/
  QTextStream out(stdout);
  int ret = 0;
  QFile file(strProXMl);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    out << " Warning : This file <" << strProXMl << "> cannot be opened! \n";
    return -1;
  }

  Project::Instance()->InitProject();
  QXmlStreamReader reader;
  reader.setDevice(&file);
  while (!reader.atEnd()) {
    QXmlStreamReader::TokenType type = reader.readNext();
    if (type == QXmlStreamReader::StartElement) {
      if (reader.name() == "project") {
        if (reader.attributes().hasAttribute("type") &&
            reader.attributes().hasAttribute("name") &&
            reader.attributes().hasAttribute("path")) {
          QString strName = reader.attributes().value("name").toString();
          QString strPath = reader.attributes().value("path").toString();
          QString strProjectPath = strPath + "/" + strName;
          ret = CreateProject(strName, strProjectPath);
          ret = setProjectType(reader.attributes().value("type").toString());
        } else {
          file.close();
          out << " Warning : This file < " << strProXMl
              << " > is missing required fields! \n";
          out << " The type,name and path fields are required. \n";
          return -2;
        }
      } else if (reader.name() == "device" &&
                 reader.attributes().hasAttribute("name")) {
        setCurrentRun(DEFAULT_FOLDER_SYNTH);
        QList<QPair<QString, QString>> listParam;
        QPair<QString, QString> pair;
        pair.first = PROJECT_PART_DEVICE;
        pair.second = reader.attributes().value("name").toString();
        listParam.append(pair);
        ret = setSynthesisOption(listParam);
      } else if (reader.name() == "sources") {
        while (true) {
          type = reader.readNext();
          if (type == QXmlStreamReader::EndElement &&
              reader.name() == "sources") {
            break;
          } else if (type == QXmlStreamReader::StartElement &&
                     reader.attributes().hasAttribute("file") &&
                     reader.attributes().hasAttribute("is_top")) {
            setCurrentFileSet(DEFAULT_FOLDER_SOURCE);
            ret = setDesignFile(reader.attributes().value("file").toString());
            if (0 != ret) {
              out << "Failed to add source. Check the format of file field.\n";
            }
            if (0 == ret &&
                "true" == reader.attributes().value("is_top").toString()) {
              QString fileName = reader.attributes().value("file").toString();
              QString module = fileName.left(fileName.lastIndexOf("."));
              ret = setTopModule(module);
            }
          }
        }
      } else if (reader.name() == "constraints") {
        while (true) {
          type = reader.readNext();
          if (type == QXmlStreamReader::EndElement &&
              reader.name() == "constraints") {
            break;
          } else if (type == QXmlStreamReader::StartElement &&
                     reader.attributes().hasAttribute("file")) {
            setCurrentFileSet(DEFAULT_FOLDER_CONSTRS);
            ret = setConstrsFile(reader.attributes().value("file").toString());
            if (0 != ret) {
              out << "Failed to add constraint. Check the format of file "
                     "field.\n";
            } else {
              ret = setTargetConstrs(
                  reader.attributes().value("file").toString());
            }
          }
        }
      } else if (reader.name() == "testbenches") {
        while (true) {
          type = reader.readNext();
          if (type == QXmlStreamReader::EndElement &&
              reader.name() == "testbenches") {
            break;
          } else if (type == QXmlStreamReader::StartElement &&
                     reader.attributes().hasAttribute("file")) {
            setCurrentFileSet(DEFAULT_FOLDER_SIM);
            ret =
                setSimulationFile(reader.attributes().value("file").toString());
            if (0 != ret) {
              out << "Failed to add testbench. Check the format of file "
                     "field.\n";
            } else {
              QString fileName = reader.attributes().value("file").toString();
              QString module = fileName.left(fileName.lastIndexOf("."));
              ret = setTopModule(module);
            }
          }
        }
      }
    }
  }
  if (reader.hasError()) {
    return -2;
  }
  file.close();
  return ret;
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

bool ProjectManager::HasDesign() const { return !getProjectName().isEmpty(); }

int ProjectManager::setProjectType(const QString& strType) {
  int ret = 0;
  ProjectConfiguration* projectConfig = Project::Instance()->projectConfig();
  projectConfig->setProjectType(strType);
  return ret;
}

ProjectManager::ErrorInfo ProjectManager::addDesignFiles(
    const QString& commands, const QString& libs, const QString& fileNames,
    int lang, bool isFileCopy, bool localToProject) {
  setCurrentFileSet(getDesignActiveFileSet());
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) return {EC_FileSetNotExist};

  const QStringList commandsList = StringSplit(commands, " ");
  const QStringList libsList = StringSplit(libs, " ");
  const QStringList fileList = StringSplit(fileNames, " ");

  // check file exists
  QStringList notExistingFiles;
  for (const auto& file : fileList) {
    if (const QFileInfo fileInfo{file}; !fileInfo.exists())
      notExistingFiles.append(file);
  }
  if (!notExistingFiles.isEmpty())
    return {EC_FileNotExist, notExistingFiles.join(", ")};
  proFileSet->addFiles(commandsList, libsList, fileList, lang);

  auto result{EC_Success};
  for (const auto& file : fileList) {
    const int res = setDesignFile(file, isFileCopy, false);
    if (res != EC_Success) result = static_cast<ErrorCode>(res);
  }
  return {result};
}

int ProjectManager::setDesignFiles(const QString& fileNames, int lang,
                                   bool isFileCopy, bool localToProject) {
  return setDesignFiles({}, {}, fileNames, lang, isFileCopy, localToProject);
}

int ProjectManager::setDesignFiles(const QString& commands, const QString& libs,
                                   const QString& fileNames, int lang,
                                   bool isFileCopy, bool localToProject) {
  setCurrentFileSet(getDesignActiveFileSet());
  const QStringList fileList = StringSplit(fileNames, " ");
  const QStringList commandsList = StringSplit(commands, " ");
  const QStringList libsList = StringSplit(libs, " ");

  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) {
    return -1;
  }

  if (localToProject) {
    const auto path =
        ProjectFilesPath(Project::Instance()->projectPath(),
                         Project::Instance()->projectName(), m_currentFileSet);
    QStringList fullPathFileList;
    for (const auto& file : fileList) {
      fullPathFileList.append(QString("%1/%2").arg(path, file));
    }
    proFileSet->addFiles(commandsList, libsList, fullPathFileList, lang);
  } else {
    if (isFileCopy) {
      QStringList localFileList;
      for (const auto& file : fileList) {
        const QFileInfo info{file};
        localFileList.append(
            ProjectFilesPath(getProjectPath(), getProjectName(),
                             m_currentFileSet, info.fileName()));
      }
      proFileSet->addFiles(commandsList, libsList, localFileList, lang);
    } else {
      proFileSet->addFiles(commandsList, libsList, fileList, lang);
    }
  }

  int result{0};
  for (const auto& file : fileList) {
    int res = setDesignFile(file, isFileCopy, localToProject);
    if (res != 0) result = res;
  }
  return result;
}

int ProjectManager::setDesignFile(const QString& strFileName, bool isFileCopy,
                                  bool localToProject) {
  int ret = 0;
  QFileInfo fileInfo(strFileName);
  if (localToProject) {
    auto path =
        ProjectFilesPath(Project::Instance()->projectPath(),
                         Project::Instance()->projectName(), m_currentFileSet);
    fileInfo.setFile(path, strFileName);
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
                                      bool isFileCopy, bool localToProject) {
  int ret = 0;
  QFileInfo fileInfo(strFileName);
  if (localToProject) {
    auto path =
        ProjectFilesPath(Project::Instance()->projectPath(),
                         Project::Instance()->projectName(), m_currentFileSet);
    fileInfo.setFile(path, strFileName);
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
                                   bool localToProject) {
  // check file exists
  if (const QFileInfo fileInfo{strFileName}; !fileInfo.exists())
    return EC_FileNotExist;
  return setConstrsFile(strFileName, isFileCopy, localToProject);
}

int ProjectManager::setConstrsFile(const QString& strFileName, bool isFileCopy,
                                   bool localToProject) {
  int ret = 0;
  QFileInfo fileInfo(strFileName);
  if (localToProject) {
    auto path =
        ProjectFilesPath(Project::Instance()->projectPath(),
                         Project::Instance()->projectName(), m_currentFileSet);
    fileInfo.setFile(path, strFileName);
  }
  QString suffix = fileInfo.suffix();
  if (fileInfo.isDir()) {
    QStringList fileList = getAllChildFiles(strFileName);
    foreach (QString strfile, fileList) {
      suffix = QFileInfo(strfile).suffix();
      if (m_constrSuffixes.TestSuffix(suffix)) {
        ret = AddOrCreateFileToFileSet(strfile, isFileCopy);
        if (ret == 0) ret = FOEDAG::read_sdc(strfile);
      }
    }
  } else if (fileInfo.exists()) {
    if (m_constrSuffixes.TestSuffix(suffix)) {
      ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
      if (ret == 0) ret = FOEDAG::read_sdc(strFileName);
    }
  } else {
    if (strFileName.contains("/")) {
      if (m_constrSuffixes.TestSuffix(suffix)) {
        ret = CreateAndAddFile(suffix, strFileName, strFileName, false);
        if (ret == 0) ret = FOEDAG::read_sdc(strFileName);
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
        if (ret == 0) ret = FOEDAG::read_sdc(filePath);
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
  proFileSet.setRelSrcDir("/" + Project::Instance()->projectName() + ".srcs/" +
                          strSetName);
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

std::vector<std::pair<int, std::string>> ProjectManager::DesignFiles() const {
  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(getDesignActiveFileSet());

  std::vector<std::pair<int, std::string>> vec;
  if (tmpFileSet && PROJECT_FILE_TYPE_DS == tmpFileSet->getSetType()) {
    auto tmpMapFiles = tmpFileSet->Files();
    for (auto iter = tmpMapFiles.begin(); iter != tmpMapFiles.end(); ++iter) {
      vec.push_back(
          std::make_pair(iter->first, iter->second.join(" ").toStdString()));
    }
  }
  return vec;
}

std::vector<std::pair<std::vector<std::string>, std::vector<std::string>>>
ProjectManager::DesignLibraries() const {
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

std::vector<std::pair<int, std::vector<std::string>>>
ProjectManager::DesignFileList() const {
  ProjectFileSet* tmpFileSet =
      Project::Instance()->getProjectFileset(getDesignActiveFileSet());

  std::vector<std::pair<int, std::vector<std::string>>> vec;
  if (tmpFileSet && PROJECT_FILE_TYPE_DS == tmpFileSet->getSetType()) {
    auto tmpMapFiles = tmpFileSet->Files();
    for (auto iter = tmpMapFiles.begin(); iter != tmpMapFiles.end(); ++iter) {
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

int ProjectManager::setConstrFileSet(const QString& strSetName) {
  int ret = 0;
  ret = CreateSrcsFolder(strSetName);
  if (0 != ret) {
    return ret;
  }

  ProjectFileSet proFileSet;
  proFileSet.setSetName(strSetName);
  proFileSet.setSetType(PROJECT_FILE_TYPE_CS);
  proFileSet.setRelSrcDir("/" + Project::Instance()->projectName() + ".srcs/" +
                          strSetName);
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

int ProjectManager::setSimulationFileSet(const QString& strSetName) {
  int ret = 0;
  ret = CreateSrcsFolder(strSetName);
  if (0 != ret) {
    return ret;
  }

  ProjectFileSet proFileSet;
  proFileSet.setSetName(strSetName);
  proFileSet.setSetType(PROJECT_FILE_TYPE_SS);
  proFileSet.setRelSrcDir("/" + Project::Instance()->projectName() + ".srcs/" +
                          strSetName);
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
  QString strActive = "";

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
  foreach (QPair pair, listParam) {
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
  int ret = 0;
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(strSetName);
  if (proFileSet && PROJECT_FILE_TYPE_DS == proFileSet->getSetType() &&
      strSetName == getDesignActiveFileSet()) {
    return -1;
  } else if (proFileSet && PROJECT_FILE_TYPE_CS == proFileSet->getSetType() &&
             strSetName == getConstrActiveFileSet()) {
    return -1;
  } else if (proFileSet && PROJECT_FILE_TYPE_SS == proFileSet->getSetType() &&
             strSetName == getSimulationActiveFileSet()) {
    return -1;
  }
  Project::Instance()->deleteProjectFileset(strSetName);
  return ret;
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

    if ("" == tmpName || "" == tmpPath) {
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

    if (!dir.mkdir(tmpPath + "/" + tmpName + ".srcs")) {
      ret = -2;
      break;
    }

    if (!dir.mkdir(tmpPath + "/" + tmpName + ".runs")) {
      ret = -2;
      break;
    }

    if (!dir.mkdir(tmpPath + "/" + tmpName + ".runs/" + DEFAULT_FOLDER_IMPLE)) {
      ret = -2;
      break;
    }

    if (!dir.mkdir(tmpPath + "/" + tmpName + ".runs/" + DEFAULT_FOLDER_SYNTH)) {
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

    QString strPath = tmpPath + "/" + tmpName + ".srcs/" + strFolderName;
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
  QFile templ{":/templates/verilog_templ.v"};
  if (!templ.open(QFile::ReadOnly | QFile::Text)) return -1;

  QString verilog_templ{templ.readAll()};

  QString tempname =
      strFile.mid(strFile.lastIndexOf("/") + 1,
                  strFile.lastIndexOf(".") - (strFile.lastIndexOf("/")) - 1);
  QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
  verilog_templ.replace("${DATE}", date);
  verilog_templ.replace("${MODULE}", tempname);

  file.write(verilog_templ.toLatin1());

  bool ok = file.flush();
  if (!ok) qWarning("%s", file.errorString().toLatin1().constData());
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
  QFile templ{":/templates/vhdl_templ.vhd"};
  if (!templ.open(QFile::ReadOnly | QFile::Text)) return -1;

  QString vhdl_templ{templ.readAll()};
  QString tempname =
      strFile.mid(strFile.lastIndexOf("/") + 1,
                  strFile.lastIndexOf(".") - (strFile.lastIndexOf("/")) - 1);
  QString date{QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")};

  vhdl_templ.replace("${DATE}", date);
  vhdl_templ.replace("${MODULE}", tempname);

  file.write(vhdl_templ.toLatin1());

  bool ok = file.flush();
  if (!ok) qWarning("%s", file.errorString().toLatin1().constData());
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
    QString filePath = "/" + Project::Instance()->projectName() + ".srcs/" +
                       m_currentFileSet + "/" + fname;
    QString destinDir = Project::Instance()->projectPath() + filePath;
    if (CopyFileToPath(strFileName, destinDir)) {
      proFileSet->addFile(fname, PROJECT_OSRCDIR + filePath);
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

QStringList ProjectManager::StringSplit(const QString& str,
                                        const QString& sep) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  return str.split(sep, Qt::SkipEmptyParts);
#else
  return str.split(sep, QString::SkipEmptyParts);
#endif
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
  } else if (!suffix.compare("SDC", Qt::CaseInsensitive)) {
    ret = CreateSDCFile(filename);
  }
  if (0 == ret) {
    ret = AddOrCreateFileToFileSet(filenameAdd, copyFile);
  }
  return ret;
}

const std::vector<std::string>& ProjectManager::libraryPathList() const {
  return Project::Instance()->compilerConfig()->libraryPathList();
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

std::string ProjectManager::getTargetDevice() {
  setCurrentRun(getActiveSynthRunName());
  return getSynthOption(PROJECT_PART_DEVICE).toStdString();
}

const std::vector<std::string>& ProjectManager::includePathList() const {
  return Project::Instance()->compilerConfig()->includePathList();
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
