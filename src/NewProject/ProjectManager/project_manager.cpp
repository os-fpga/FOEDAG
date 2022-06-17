#include "project_manager.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QXmlStreamWriter>
#include <filesystem>

#include "Compiler/CompilerDefines.h"
extern const char* foedag_version_number;

using namespace FOEDAG;

ProjectManager::ProjectManager(QObject* parent) : QObject(parent) {
  // Re-emit projectPathChanged signals
  QObject::connect(Project::Instance(), &Project::projectPathChanged, this,
                   &ProjectManager::projectPathChanged);
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
  QString strDefaultSrc;
  QList<filedata> listFile = opt.sourceFileData.fileData;
  foreach (filedata fdata, listFile) {
    if ("<Local to Project>" == fdata.m_filePath) {
      setDesignFiles(fdata.m_fileName, FromFileType(fdata.m_fileType), false);
    } else {
      setDesignFiles(fdata.m_filePath + "/" + fdata.m_fileName,
                     FromFileType(fdata.m_fileType),
                     opt.sourceFileData.isCopySource);
    }
    if (!fdata.m_isFolder) {
      strDefaultSrc = fdata.m_fileName;
    }
  }

  if (!strDefaultSrc.isEmpty()) {
    QString module = strDefaultSrc.left(strDefaultSrc.lastIndexOf("."));
    setTopModule(module);

    // set default simulation source
    setCurrentFileSet(DEFAULT_FOLDER_SIM);
    setDesignFile("sim_" + strDefaultSrc, false);
    setTopModule("sim_" + module);
  }

  setCurrentFileSet(DEFAULT_FOLDER_CONSTRS);
  QString strDefaultCts;
  listFile.clear();
  listFile = opt.constrFileData.fileData;
  foreach (filedata fdata, listFile) {
    if ("<Local to Project>" == fdata.m_filePath) {
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

  FinishedProject();
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
      ExportProjectData();
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

int ProjectManager::setDesignFiles(const QString& fileNames, int lang,
                                   bool isFileCopy) {
  setCurrentFileSet(getDesignActiveFileSet());
  QStringList fileList = StringSplit(fileNames, " ");
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(m_currentFileSet);
  if (nullptr == proFileSet) {
    return -1;
  }
  proFileSet->addFiles(fileList, lang);

  int result{0};
  for (const auto& file : fileList) {
    int res = setDesignFile(file, isFileCopy);
    if (res != 0) result = res;
  }
  return result;
}

int ProjectManager::setDesignFile(const QString& strFileName, bool isFileCopy) {
  int ret = 0;
  QFileInfo fileInfo(strFileName);
  QString suffix = fileInfo.suffix();
  if (fileInfo.isDir()) {
    QStringList fileList = getAllChildFiles(strFileName);
    foreach (QString strfile, fileList) {
      suffix = QFileInfo(strfile).suffix();
      if (!suffix.compare("v", Qt::CaseInsensitive) ||
          !suffix.compare("sv", Qt::CaseInsensitive) ||
          !suffix.compare("vh", Qt::CaseInsensitive) ||
          !suffix.compare("vhd", Qt::CaseInsensitive) ||
          !suffix.compare("blif", Qt::CaseInsensitive) ||
          !suffix.compare("eblif", Qt::CaseInsensitive)) {
        ret = AddOrCreateFileToFileSet(strfile, isFileCopy);
      }
    }
  } else if (fileInfo.exists()) {
    if (!suffix.compare("v", Qt::CaseInsensitive) ||
        !suffix.compare("sv", Qt::CaseInsensitive) ||
        !suffix.compare("vh", Qt::CaseInsensitive) ||
        !suffix.compare("vhd", Qt::CaseInsensitive) ||
        !suffix.compare("blif", Qt::CaseInsensitive) ||
        !suffix.compare("eblif", Qt::CaseInsensitive)) {
      ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
    }
  } else {
    if (strFileName.contains("/")) {
      if (!suffix.compare("v", Qt::CaseInsensitive)) {
        ret = CreateVerilogFile(strFileName);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
        }
      } else if (!suffix.compare("sv", Qt::CaseInsensitive)) {
        ret = CreateSystemVerilogFile(strFileName);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
        }
      } else if (!suffix.compare("vh", Qt::CaseInsensitive)) {
        ret = CreateSystemVerilogFile(strFileName);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
        }
      } else if (!suffix.compare("blif", Qt::CaseInsensitive)) {
        ret = CreateSystemVerilogFile(strFileName);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
        }
      } else if (!suffix.compare("eblif", Qt::CaseInsensitive)) {
        ret = CreateSystemVerilogFile(strFileName);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
        }
      } else if (!suffix.compare("vhd", Qt::CaseInsensitive)) {
        ret = CreateVHDLFile(strFileName);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
        }
      }
    } else {
      QString filePath = Project::Instance()->projectPath() + "/" +
                         Project::Instance()->projectName() + ".srcs/" +
                         m_currentFileSet + "/" + strFileName;
      QString fileSetPath = PROJECT_OSRCDIR;
      fileSetPath += "/" + Project::Instance()->projectName() + ".srcs/" +
                     m_currentFileSet + "/" + strFileName;
      if (!suffix.compare("v", Qt::CaseInsensitive)) {
        ret = CreateVerilogFile(filePath);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(fileSetPath, false);
        }
      } else if (!suffix.compare("sv", Qt::CaseInsensitive)) {
        ret = CreateSystemVerilogFile(filePath);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(fileSetPath, false);
        }
      } else if (!suffix.compare("vh", Qt::CaseInsensitive)) {
        ret = CreateSystemVerilogFile(filePath);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(fileSetPath, false);
        }
      } else if (!suffix.compare("blif", Qt::CaseInsensitive)) {
        ret = CreateSystemVerilogFile(filePath);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(fileSetPath, false);
        }
      } else if (!suffix.compare("eblif", Qt::CaseInsensitive)) {
        ret = CreateSystemVerilogFile(filePath);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(fileSetPath, false);
        }
      } else if (!suffix.compare("vhd", Qt::CaseInsensitive)) {
        ret = CreateVHDLFile(filePath);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(fileSetPath, false);
        }
      }
    }
  }
  return ret;
}

int ProjectManager::setSimulationFile(const QString& strFileName,
                                      bool isFileCopy) {
  int ret = 0;
  QFileInfo fileInfo(strFileName);
  QString suffix = fileInfo.suffix();
  if (fileInfo.isDir()) {
    QStringList fileList = getAllChildFiles(strFileName);
    foreach (QString strfile, fileList) {
      suffix = QFileInfo(strfile).suffix();
      if (!suffix.compare("v", Qt::CaseInsensitive)) {
        ret = AddOrCreateFileToFileSet(strfile, isFileCopy);
      }
    }
  } else if (fileInfo.exists()) {
    if (!suffix.compare("v", Qt::CaseInsensitive)) {
      ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
    }
  } else {
    if (strFileName.contains("/")) {
      if (!suffix.compare("v", Qt::CaseInsensitive)) {
        ret = CreateVerilogFile(strFileName);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
        }
      }
    } else {
      QString filePath = Project::Instance()->projectPath() + "/" +
                         Project::Instance()->projectName() + ".srcs/" +
                         m_currentFileSet + "/" + strFileName;
      QString fileSetPath = PROJECT_OSRCDIR;
      fileSetPath += "/" + Project::Instance()->projectName() + ".srcs/" +
                     m_currentFileSet + "/" + strFileName;
      if (!suffix.compare("v", Qt::CaseInsensitive)) {
        ret = CreateVerilogFile(filePath);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(fileSetPath, false);
        }
      }
    }
  }
  return ret;
}

int ProjectManager::setConstrsFile(const QString& strFileName,
                                   bool isFileCopy) {
  int ret = 0;
  QFileInfo fileInfo(strFileName);
  QString suffix = fileInfo.suffix();
  if (fileInfo.isDir()) {
    QStringList fileList = getAllChildFiles(strFileName);
    foreach (QString strfile, fileList) {
      suffix = QFileInfo(strfile).suffix();
      if (!suffix.compare("SDC", Qt::CaseInsensitive)) {
        ret = AddOrCreateFileToFileSet(strfile, isFileCopy);
      }
    }
  } else if (fileInfo.exists()) {
    if (!suffix.compare("SDC", Qt::CaseInsensitive)) {
      ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
    }
  } else {
    if (strFileName.contains("/")) {
      if (!suffix.compare("SDC", Qt::CaseInsensitive)) {
        ret = CreateSDCFile(strFileName);
        if (0 == ret) {
          ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
        }
      }
    } else {
      QString filePath = Project::Instance()->projectPath() + "/" +
                         Project::Instance()->projectName() + ".srcs/" +
                         m_currentFileSet + "/" + strFileName;
      QString fileSetPath = PROJECT_OSRCDIR;
      fileSetPath += "/" + Project::Instance()->projectName() + ".srcs/" +
                     m_currentFileSet + "/" + strFileName;
      if (!suffix.compare("SDC", Qt::CaseInsensitive)) {
        ret = CreateSDCFile(filePath);
        if (0 == ret) {
          AddOrCreateFileToFileSet(fileSetPath, false);
        }
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
  // target or top file cannot be deleted
  if (strFileName == proFileSet->getOption(PROJECT_FILE_CONFIG_TOP) ||
      strFileName == proFileSet->getOption(PROJECT_FILE_CONFIG_TARGET)) {
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

int ProjectManager::StartProject(const QString& strOspro) {
  return ImportProjectData(strOspro);
}

int ProjectManager::FinishedProject() { return ExportProjectData(); }

int ProjectManager::ImportProjectData(QString strOspro) {
  int ret = 0;
  if ("" == strOspro) {
    return ret;
  }
  QString strTemp = Project::Instance()->projectPath();
  strTemp += "/";
  strTemp += Project::Instance()->projectName();
  strTemp += PROJECT_FILE_FORMAT;
  if (strOspro == strTemp) {
    return ret;
  }

  // this is starting point for backward compatibility
  QString version = ProjectVersion(strOspro);
  Q_UNUSED(version)
  // reorganize code in the future to have different loaders for compatible
  // versions

  QFile file(strOspro);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    return -1;
  }

  Project::Instance()->InitProject();
  QXmlStreamReader reader;
  reader.setDevice(&file);
  while (!reader.atEnd()) {
    QXmlStreamReader::TokenType type = reader.readNext();
    if (type == QXmlStreamReader::StartElement) {
      if (reader.name() == PROJECT_PROJECT &&
          reader.attributes().hasAttribute(PROJECT_PATH)) {
        QString strPath = reader.attributes().value(PROJECT_PATH).toString();
        QString strName = strPath.mid(
            strPath.lastIndexOf("/") + 1,
            strPath.lastIndexOf(".") - (strPath.lastIndexOf("/")) - 1);

        Project::Instance()->setProjectName(strName);
        Project::Instance()->setProjectPath(
            strPath.left(strPath.lastIndexOf("/")));
      }
      if (reader.name() == PROJECT_CONFIGURATION) {
        while (true) {
          type = reader.readNext();
          if (type == QXmlStreamReader::EndElement &&
              reader.name() == PROJECT_CONFIGURATION) {
            break;
          }

          ProjectConfiguration* tmpProCfg =
              Project::Instance()->projectConfig();
          if (type == QXmlStreamReader::StartElement &&
              reader.attributes().hasAttribute(PROJECT_NAME) &&
              reader.attributes().hasAttribute(PROJECT_VAL)) {
            if (PROJECT_CONFIG_ID ==
                reader.attributes().value(PROJECT_NAME).toString()) {
              tmpProCfg->setId(
                  reader.attributes().value(PROJECT_VAL).toString());
            } else if (PROJECT_CONFIG_ACTIVESIMSET ==
                       reader.attributes().value(PROJECT_NAME).toString()) {
              tmpProCfg->setActiveSimSet(
                  reader.attributes().value(PROJECT_VAL).toString());
            } else if (PROJECT_CONFIG_TYPE ==
                       reader.attributes().value(PROJECT_NAME).toString()) {
              tmpProCfg->setProjectType(
                  reader.attributes().value(PROJECT_VAL).toString());
            } else {
              tmpProCfg->setOption(
                  reader.attributes().value(PROJECT_NAME).toString(),
                  reader.attributes().value(PROJECT_VAL).toString());
            }
          }
        }
      }
      if (reader.name() == PROJECT_FILESETS) {
        QString strSetName;
        QString strSetType;
        QString strSetSrcDir;
        QStringList listFiles;
        std::vector<std::pair<int, QString>> langList;
        QMap<QString, QString> mapOption;
        while (true) {
          type = reader.readNext();
          if (type == QXmlStreamReader::EndElement &&
              reader.name() == PROJECT_FILESETS) {
            break;
          } else if (type == QXmlStreamReader::StartElement &&
                     reader.attributes().hasAttribute(PROJECT_FILESET_NAME) &&
                     reader.attributes().hasAttribute(PROJECT_FILESET_TYPE) &&
                     reader.attributes().hasAttribute(
                         PROJECT_FILESET_RELSRCDIR)) {
            strSetName =
                reader.attributes().value(PROJECT_FILESET_NAME).toString();
            strSetType =
                reader.attributes().value(PROJECT_FILESET_TYPE).toString();
            strSetSrcDir =
                reader.attributes().value(PROJECT_FILESET_RELSRCDIR).toString();
          } else if (type == QXmlStreamReader::StartElement &&
                     reader.attributes().hasAttribute(PROJECT_PATH)) {
            listFiles.append(
                reader.attributes().value(PROJECT_PATH).toString());
          } else if (type == QXmlStreamReader::StartElement &&
                     reader.attributes().hasAttribute(PROJECT_NAME) &&
                     reader.attributes().hasAttribute(PROJECT_VAL)) {
            mapOption.insert(reader.attributes().value(PROJECT_NAME).toString(),
                             reader.attributes().value(PROJECT_VAL).toString());
          } else if (type == QXmlStreamReader::StartElement &&
                     reader.attributes().hasAttribute(PROJECT_GROUP_ID) &&
                     reader.attributes().hasAttribute(PROJECT_GROUP_FILES)) {
            langList.push_back(std::make_pair(
                reader.attributes().value(PROJECT_GROUP_ID).toInt(),
                reader.attributes().value(PROJECT_GROUP_FILES).toString()));
          } else if (type == QXmlStreamReader::EndElement &&
                     reader.name() == PROJECT_FILESET) {
            ProjectFileSet projectFileset;
            projectFileset.setSetName(strSetName);
            projectFileset.setSetType(strSetType);
            projectFileset.setRelSrcDir(strSetSrcDir);

            foreach (QString strFile, listFiles) {
              projectFileset.addFile(
                  strFile.right(strFile.size() -
                                (strFile.lastIndexOf("/") + 1)),
                  strFile);
            }
            for (const auto& i : langList) {
              projectFileset.addFiles(StringSplit(i.second, " "), i.first);
            }
            for (auto iter = mapOption.begin(); iter != mapOption.end();
                 ++iter) {
              projectFileset.setOption(iter.key(), iter.value());
            }
            Project::Instance()->setProjectFileset(projectFileset);
            // clear data for next
            strSetName = "";
            strSetType = "";
            strSetSrcDir = "";
            listFiles.clear();
            mapOption.clear();
          }
        }
      }
      if (reader.name() == PROJECT_RUNS /*"Runs"*/) {
        QString strRunName;
        QString strRunType;
        QString strSrcSet;
        QString strConstrs;
        QString strRunState;
        QString strSynthRun;
        QMap<QString, QString> mapOption;
        while (true) {
          type = reader.readNext();
          if (type == QXmlStreamReader::EndElement &&
              reader.name() == PROJECT_RUNS) {
            break;
          } else if ((type == QXmlStreamReader::StartElement &&
                      reader.attributes().hasAttribute(PROJECT_RUN_NAME) &&
                      reader.attributes().hasAttribute(PROJECT_RUN_TYPE) &&
                      reader.attributes().hasAttribute(PROJECT_RUN_SRCSET) &&
                      reader.attributes().hasAttribute(
                          PROJECT_RUN_CONSTRSSET) &&
                      reader.attributes().hasAttribute(PROJECT_RUN_STATE)) ||
                     reader.attributes().hasAttribute(PROJECT_RUN_SYNTHRUN)) {
            strRunName = reader.attributes().value(PROJECT_RUN_NAME).toString();
            strRunType = reader.attributes().value(PROJECT_RUN_TYPE).toString();
            strSrcSet =
                reader.attributes().value(PROJECT_RUN_SRCSET).toString();
            strConstrs =
                reader.attributes().value(PROJECT_RUN_CONSTRSSET).toString();
            strRunState =
                reader.attributes().value(PROJECT_RUN_STATE).toString();

            if (reader.attributes().hasAttribute(PROJECT_RUN_SYNTHRUN)) {
              strSynthRun =
                  reader.attributes().value(PROJECT_RUN_SYNTHRUN).toString();
            }
          } else if (type == QXmlStreamReader::StartElement &&
                     reader.attributes().hasAttribute(PROJECT_NAME) &&
                     reader.attributes().hasAttribute(PROJECT_VAL)) {
            mapOption.insert(reader.attributes().value(PROJECT_NAME).toString(),
                             reader.attributes().value(PROJECT_VAL).toString());
          } else if (type == QXmlStreamReader::EndElement &&
                     reader.name() == PROJECT_RUN) {
            ProjectRun proRun;
            proRun.setRunName(strRunName);
            proRun.setRunType(strRunType);
            proRun.setSrcSet(strSrcSet);
            proRun.setConstrsSet(strConstrs);
            proRun.setRunState(strRunState);
            proRun.setSynthRun(strSynthRun);

            for (auto iter = mapOption.begin(); iter != mapOption.end();
                 ++iter) {
              proRun.setOption(iter.key(), iter.value());
            }
            Project::Instance()->setProjectRun(proRun);
            // clear data for next
            strRunName = "";
            strRunType = "";
            strSrcSet = "";
            strConstrs = "";
            strRunState = "";
            strSynthRun = "";
            mapOption.clear();
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

int ProjectManager::ExportProjectData() {
  QString tmpName = Project::Instance()->projectName();
  QString tmpPath = Project::Instance()->projectPath();
  QString xmlPath = tmpPath + "/" + tmpName + PROJECT_FILE_FORMAT;
  QFile file(xmlPath);
  if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
    return -1;
  }
  QXmlStreamWriter stream(&file);
  stream.setAutoFormatting(true);
  stream.writeStartDocument();
  stream.writeComment(
      tr("                                                   "));
  stream.writeComment(
      tr("Copyright (c) 2021-2022 The Open-Source FPGA Foundation."));
  stream.writeStartElement(PROJECT_PROJECT);
  stream.writeAttribute(PROJECT_PATH, xmlPath);
  stream.writeAttribute(PROJECT_VERSION, foedag_version_number);

  stream.writeStartElement(PROJECT_CONFIGURATION);

  ProjectConfiguration* tmpProCfg = Project::Instance()->projectConfig();
  stream.writeStartElement(PROJECT_OPTION);
  stream.writeAttribute(PROJECT_NAME, PROJECT_CONFIG_ID);
  stream.writeAttribute(PROJECT_VAL, tmpProCfg->id());
  stream.writeEndElement();

  stream.writeStartElement(PROJECT_OPTION);
  stream.writeAttribute(PROJECT_NAME, PROJECT_CONFIG_ACTIVESIMSET);
  stream.writeAttribute(PROJECT_VAL, tmpProCfg->activeSimSet());
  stream.writeEndElement();

  stream.writeStartElement(PROJECT_OPTION);
  stream.writeAttribute(PROJECT_NAME, PROJECT_CONFIG_TYPE);
  stream.writeAttribute(PROJECT_VAL, tmpProCfg->projectType());
  stream.writeEndElement();

  QMap<QString, QString> tmpOption = tmpProCfg->getMapOption();
  for (auto iter = tmpOption.begin(); iter != tmpOption.end(); ++iter) {
    stream.writeStartElement(PROJECT_OPTION);
    stream.writeAttribute(PROJECT_NAME, iter.key());
    stream.writeAttribute(PROJECT_VAL, iter.value());
    stream.writeEndElement();
  }
  stream.writeEndElement();

  stream.writeStartElement(PROJECT_FILESETS);
  QMap<QString, ProjectFileSet*> tmpFileSetMap =
      Project::Instance()->getMapProjectFileset();
  for (auto iter = tmpFileSetMap.begin(); iter != tmpFileSetMap.end(); ++iter) {
    ProjectFileSet* tmpFileSet = iter.value();

    stream.writeStartElement(PROJECT_FILESET);

    stream.writeAttribute(PROJECT_FILESET_NAME, tmpFileSet->getSetName());
    stream.writeAttribute(PROJECT_FILESET_TYPE, tmpFileSet->getSetType());
    stream.writeAttribute(PROJECT_FILESET_RELSRCDIR,
                          tmpFileSet->getRelSrcDir());

    auto tmpFileMap = tmpFileSet->getMapFiles();
    for (auto iterfile = tmpFileMap.begin(); iterfile != tmpFileMap.end();
         ++iterfile) {
      stream.writeStartElement(PROJECT_FILESET_FILE);
      stream.writeAttribute(PROJECT_PATH, iterfile->second);
      stream.writeEndElement();
    }
    auto langMap = tmpFileSet->Files();
    for (auto it = langMap.cbegin(); it != langMap.cend(); ++it) {
      stream.writeStartElement(PROJECT_GROUP);
      stream.writeAttribute(PROJECT_GROUP_ID, QString::number(it->first));
      stream.writeAttribute(PROJECT_GROUP_FILES, it->second.join(" "));
      stream.writeEndElement();
    }

    QMap<QString, QString> tmpOptionF = tmpFileSet->getMapOption();
    if (tmpOptionF.size()) {
      stream.writeStartElement(PROJECT_FILESET_CONFIG);
      for (auto iterOption = tmpOptionF.begin(); iterOption != tmpOptionF.end();
           ++iterOption) {
        stream.writeStartElement(PROJECT_OPTION);
        stream.writeAttribute(PROJECT_NAME, iterOption.key());
        stream.writeAttribute(PROJECT_VAL, iterOption.value());
        stream.writeEndElement();
      }
      stream.writeEndElement();
    }
    stream.writeEndElement();
  }
  stream.writeEndElement();

  stream.writeStartElement(PROJECT_RUNS);
  QMap<QString, ProjectRun*> tmpRunMap =
      Project::Instance()->getMapProjectRun();
  for (auto iter = tmpRunMap.begin(); iter != tmpRunMap.end(); ++iter) {
    ProjectRun* tmpRun = iter.value();
    stream.writeStartElement(PROJECT_RUN);
    stream.writeAttribute(PROJECT_RUN_NAME, tmpRun->runName());
    stream.writeAttribute(PROJECT_RUN_TYPE, tmpRun->runType());
    stream.writeAttribute(PROJECT_RUN_SRCSET, tmpRun->srcSet());
    stream.writeAttribute(PROJECT_RUN_CONSTRSSET, tmpRun->constrsSet());
    stream.writeAttribute(PROJECT_RUN_STATE, tmpRun->runState());
    stream.writeAttribute(PROJECT_RUN_SYNTHRUN, tmpRun->synthRun());

    QMap<QString, QString> tmpOptionF = tmpRun->getMapOption();
    for (auto iterOption = tmpOptionF.begin(); iterOption != tmpOptionF.end();
         ++iterOption) {
      stream.writeStartElement(PROJECT_OPTION);
      stream.writeAttribute(PROJECT_NAME, iterOption.key());
      stream.writeAttribute(PROJECT_VAL, iterOption.value());
      stream.writeEndElement();
    }
    stream.writeEndElement();
  }
  stream.writeEndElement();

  stream.writeEndDocument();
  file.close();

  return 0;
}

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

QString ProjectManager::ProjectVersion(const QString& filename) {
  QFile file(filename);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    return QString();
  }

  QXmlStreamReader reader;
  reader.setDevice(&file);
  while (!reader.atEnd()) {
    QXmlStreamReader::TokenType type = reader.readNext();
    if (type == QXmlStreamReader::StartElement) {
      if (reader.name() == PROJECT_PROJECT) {
        QString projectVersion =
            reader.attributes().value(PROJECT_VERSION).toString();
        return projectVersion;
      }
    }
  }

  if (reader.hasError()) {
    return QString();
  }
  file.close();
  return QString();
}

const std::vector<std::string>& ProjectManager::libraryPathList() const {
  return m_libraryPathList;
}

void ProjectManager::setLibraryPathList(
    const std::vector<std::string>& newLibraryPathList) {
  m_libraryPathList = newLibraryPathList;
}

void ProjectManager::addLibraryPath(const std::string& libraryPath) {
  m_libraryPathList.push_back(libraryPath);
}

const std::vector<std::string>& ProjectManager::libraryExtensionList() const {
  return m_libraryExtList;
}

void ProjectManager::setLibraryExtensionList(
    const std::vector<std::string>& newLibraryExtensionList) {
  m_libraryExtList = newLibraryExtensionList;
}
void ProjectManager::addLibraryExtension(const std::string& libraryExt) {
  m_libraryExtList.push_back(libraryExt);
}

void ProjectManager::addMacro(const std::string& macroName,
                              const std::string& macroValue) {
  m_macroList.push_back(std::pair(macroName, macroValue));
}

const std::vector<std::pair<std::string, std::string>>&
ProjectManager::macroList() const {
  return m_macroList;
}

const std::vector<std::string>& ProjectManager::includePathList() const {
  return m_includePathList;
}

void ProjectManager::setIncludePathList(
    const std::vector<std::string>& newIncludePathList) {
  m_includePathList = newIncludePathList;
}

void ProjectManager::addIncludePath(const std::string& includePath) {
  m_includePathList.push_back(includePath);
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
