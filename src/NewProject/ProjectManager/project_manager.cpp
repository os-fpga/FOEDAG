#include "project_manager.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QXmlStreamWriter>

using namespace FOEDAG;

ProjectManager::ProjectManager(QObject* parent) : QObject(parent) {}

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
        pair.first = "device";
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
              ret = setTopModule(reader.attributes().value("file").toString());
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
              ret = setTopModule(reader.attributes().value("file").toString());
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
                                  const QString& strPath) {
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

  ret = setDesignFileSet(DEFAULT_FOLDER_SOURCE);

  ret = setConstrFileSet(DEFAULT_FOLDER_CONSTRS);

  ret = setSimulationFileSet(DEFAULT_FOLDER_SIM);

  ProjectRun* proRun = new ProjectRun(this);
  proRun->setRunName(DEFAULT_FOLDER_IMPLE);
  proRun->setRunType(RUN_TYPE_IMPLEMENT);
  proRun->setSrcSet(DEFAULT_FOLDER_SOURCE);
  proRun->setConstrsSet(DEFAULT_FOLDER_CONSTRS);
  proRun->setRunState(RUN_STATE_CURRENT);
  proRun->setSynthRun(DEFAULT_FOLDER_SYNTH);
  Project::Instance()->setProjectRun(proRun);

  proRun = new ProjectRun(this);
  proRun->setRunName(DEFAULT_FOLDER_SYNTH);
  proRun->setRunType(RUN_TYPE_SYNTHESIS);
  proRun->setSrcSet(DEFAULT_FOLDER_SOURCE);
  proRun->setConstrsSet(DEFAULT_FOLDER_CONSTRS);
  proRun->setRunState(RUN_STATE_CURRENT);
  proRun->setOption("Compilation Flow", "Classic Flow");
  proRun->setOption("LanguageVersion", "SYSTEMVERILOG_2005");
  proRun->setOption("TargetLanguage", "VERILOG");
  Project::Instance()->setProjectRun(proRun);

  ProjectConfiguration* projectConfig = Project::Instance()->projectConfig();
  projectConfig->setActiveSimSet(DEFAULT_FOLDER_SIM);

  return ret;
}

QString ProjectManager::getProjectName() const {
  return Project::Instance()->projectName();
}

QString ProjectManager::getProjectPath() const {
  return Project::Instance()->projectPath();
}

int ProjectManager::setProjectType(const QString& strType) {
  int ret = 0;
  ProjectConfiguration* projectConfig = Project::Instance()->projectConfig();
  projectConfig->setProjectType(strType);
  return ret;
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
          !suffix.compare("vhd", Qt::CaseInsensitive)) {
        ret = AddOrCreateFileToFileSet(strfile, isFileCopy);
      }
    }
  } else if (fileInfo.exists()) {
    if (!suffix.compare("v", Qt::CaseInsensitive) ||
        !suffix.compare("vhd", Qt::CaseInsensitive)) {
      ret = AddOrCreateFileToFileSet(strFileName, isFileCopy);
    }
  } else {
    if (strFileName.contains("/")) {
      if (!suffix.compare("v", Qt::CaseInsensitive)) {
        ret = CreateVerilogFile(strFileName);
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
      QString fileSetPath = "$OSRCDIR/" + Project::Instance()->projectName() +
                            ".srcs/" + m_currentFileSet + "/" + strFileName;
      if (!suffix.compare("v", Qt::CaseInsensitive)) {
        ret = CreateVerilogFile(filePath);
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
      QString fileSetPath = "$OSRCDIR/" + Project::Instance()->projectName() +
                            ".srcs/" + m_currentFileSet + "/" + strFileName;
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
      QString fileSetPath = "$OSRCDIR/" + Project::Instance()->projectName() +
                            ".srcs/" + m_currentFileSet + "/" + strFileName;
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

int ProjectManager::setTopModule(const QString& strFileName) {
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

  proFileSet->setOption(PROJECT_FILE_CONFIG_TOP, strFileName);
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

  ProjectFileSet* proFileSet = new ProjectFileSet(this);
  proFileSet->setSetName(strSetName);
  proFileSet->setSetType(PROJECT_FILE_TYPE_DS);
  proFileSet->setRelSrcDir("/" + Project::Instance()->projectName() + ".srcs/" +
                           strSetName);
  ret = Project::Instance()->setProjectFileset(proFileSet);
  if (ret) {
    delete proFileSet;
  }
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
    QMap<QString, QString> tmpMapFiles = tmpFileSet->getMapFiles();
    for (auto iter = tmpMapFiles.begin(); iter != tmpMapFiles.end(); ++iter) {
      strList.append(iter.value());
    }
  }
  return strList;
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

int ProjectManager::setConstrFileSet(const QString& strSetName) {
  int ret = 0;
  ret = CreateSrcsFolder(strSetName);
  if (0 != ret) {
    return ret;
  }

  ProjectFileSet* proFileSet = new ProjectFileSet(this);
  proFileSet->setSetName(strSetName);
  proFileSet->setSetType(PROJECT_FILE_TYPE_CS);
  proFileSet->setRelSrcDir("/" + Project::Instance()->projectName() + ".srcs/" +
                           strSetName);
  ret = Project::Instance()->setProjectFileset(proFileSet);
  if (ret) {
    delete proFileSet;
  }
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
    QMap<QString, QString> tmpMapFiles = tmpFileSet->getMapFiles();
    for (auto iter = tmpMapFiles.begin(); iter != tmpMapFiles.end(); ++iter) {
      strList.append(iter.value());
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

  ProjectFileSet* proFileSet = new ProjectFileSet(this);
  proFileSet->setSetName(strSetName);
  proFileSet->setSetType(PROJECT_FILE_TYPE_SS);
  proFileSet->setRelSrcDir("/" + Project::Instance()->projectName() + ".srcs/" +
                           strSetName);
  ret = Project::Instance()->setProjectFileset(proFileSet);
  if (ret) {
    delete proFileSet;
  }
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
    QMap<QString, QString> tmpMapFiles = tmpFileSet->getMapFiles();
    for (auto iter = tmpMapFiles.begin(); iter != tmpMapFiles.end(); ++iter) {
      strList.append(iter.value());
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
    pair.first = PROJECT_PART_DEVICE;
    pair.second = proRun->getOption(PROJECT_PART_DEVICE);
    listProperties.append(pair);
  }

  return listProperties;
}

int ProjectManager::setSynthRun(const QString& strRunName) {
  ProjectRun* proRun = new ProjectRun(this);
  proRun = new ProjectRun(this);
  proRun->setRunName(strRunName);
  proRun->setRunType(RUN_TYPE_SYNTHESIS);
  proRun->setOption("Compilation Flow", "Classic Flow");
  proRun->setOption("LanguageVersion", "SYSTEMVERILOG_2005");
  proRun->setOption("TargetLanguage", "VERILOG");
  Project::Instance()->setProjectRun(proRun);
  CreateRunsFolder(strRunName);
  m_currentRun = strRunName;
  return 0;
}

int ProjectManager::setImpleRun(const QString& strRunName) {
  ProjectRun* proRun = new ProjectRun(this);
  proRun = new ProjectRun(this);
  proRun->setRunName(strRunName);
  proRun->setRunType(RUN_TYPE_IMPLEMENT);
  Project::Instance()->setProjectRun(proRun);
  CreateRunsFolder(strRunName);
  m_currentRun = strRunName;
  return 0;
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

int ProjectManager::setRunConstrSet(const QString& strConstrSet) {
  int ret = 0;
  ProjectRun* proRun = Project::Instance()->getProjectRun(m_currentRun);
  if (nullptr == proRun) {
    return -2;
  }
  proRun->setConstrsSet(strConstrSet);
  return ret;
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

int ProjectManager::deleteFileSet(const QString& strSetName) {
  int ret = 0;
  ProjectFileSet* proFileSet =
      Project::Instance()->getProjectFileset(strSetName);
  if (PROJECT_FILE_TYPE_DS == proFileSet->getSetType() &&
      strSetName == getDesignActiveFileSet()) {
    return -1;
  } else if (PROJECT_FILE_TYPE_CS == proFileSet->getSetType() &&
             strSetName == getConstrActiveFileSet()) {
    return -1;
  } else if (PROJECT_FILE_TYPE_SS == proFileSet->getSetType() &&
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
  QString strTemp = Project::Instance()->projectPath() + "/" +
                    Project::Instance()->projectName() + PROJECT_FILE_FORMAT;
  if (strOspro == strTemp) {
    return ret;
  }

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
        ;
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
          } else if (type == QXmlStreamReader::EndElement &&
                     reader.name() == PROJECT_FILESET) {
            ProjectFileSet* projectFileset = new ProjectFileSet();
            projectFileset->setSetName(strSetName);
            projectFileset->setSetType(strSetType);
            projectFileset->setRelSrcDir(strSetSrcDir);

            foreach (QString strFile, listFiles) {
              projectFileset->addFile(
                  strFile.right(strFile.size() -
                                (strFile.lastIndexOf("/") + 1)),
                  strFile);
            }
            for (auto iter = mapOption.begin(); iter != mapOption.end();
                 ++iter) {
              projectFileset->setOption(iter.key(), iter.value());
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
            ProjectRun* proRun = new ProjectRun();
            proRun->setRunName(strRunName);
            proRun->setRunType(strRunType);
            proRun->setSrcSet(strSrcSet);
            proRun->setConstrsSet(strConstrs);
            proRun->setRunState(strRunState);
            proRun->setSynthRun(strSynthRun);

            for (auto iter = mapOption.begin(); iter != mapOption.end();
                 ++iter) {
              proRun->setOption(iter.key(), iter.value());
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
      tr("Product Version:  FOEDAG   V1.0.0.0                "));
  stream.writeComment(
      tr("                                                   "));
  stream.writeComment(
      tr("Copyright (c) 2021 The Open-Source FPGA Foundation."));
  stream.writeStartElement(PROJECT_PROJECT);
  stream.writeAttribute(PROJECT_PATH, xmlPath);

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

    QMap<QString, QString> tmpFileMap = tmpFileSet->getMapFiles();
    for (auto iterfile = tmpFileMap.begin(); iterfile != tmpFileMap.end();
         ++iterfile) {
      stream.writeStartElement(PROJECT_FILESET_FILE);
      stream.writeAttribute(PROJECT_PATH, iterfile.value());
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
    QTextStream out(stdout);
    return -1;
  }
  QString tempname =
      strFile.mid(strFile.lastIndexOf("/") + 1,
                  strFile.lastIndexOf(".") - (strFile.lastIndexOf("/")) - 1);
  QTextStream out(&file);
  out << "`timescale 1 ps/ 1 ps \n";
  out << "///////////////////////////////////////////////////////////////// \n";
  out << "// Company: \n";
  out << "// Engineer: \n";
  out << "// Create Date: "
      << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
  out << "// Design Name: \n";
  out << "// Module Name: " << tempname << "\n";
  out << "// Project Name: \n";
  out << "// Target Devices: \n";
  out << "// Tool Versions: \n";
  out << "// Description: \n";
  out << "// \n";
  out << "// Dependencies: \n";
  out << "// \n";
  out << "// Revision: \n";
  out << "// Additional Comments:\n";
  out << "// \n";
  out << "///////////////////////////////////////////////////////////////// \n";

  out << "\n\n";
  out << "module " << tempname << "( \n";
  out << "\n";
  out << "    ); \n";
  out << "endmodule \n";

  file.flush();
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
  QString tempname =
      strFile.mid(strFile.lastIndexOf("/") + 1,
                  strFile.lastIndexOf(".") - (strFile.lastIndexOf("/")) - 1);
  QTextStream out(&file);
  out << "------------------------------------------------------------------\n";
  out << "-- Company:\n";
  out << "-- Engineer:\n";
  out << "-- \n";
  out << "-- Create Date: "
      << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "\n";
  out << "-- Design Name: \n";
  out << "-- Module Name: " << tempname << " - Behavioral \n";
  out << "-- Project Name: \n";
  out << "-- Target Devices: \n";
  out << "-- Tool Versions: \n";
  out << "-- Description: \n";
  out << "-- \n";
  out << "-- Dependencies: \n";
  out << "-- \n";
  out << "-- Revision: \n";
  out << "-- Additional Comments: \n";
  out << "-- \n";
  out << "------------------------------------------------------------------\n";

  out << "\n\n";
  out << "library IEEE;\n";
  out << "use IEEE.STD_LOGIC_1164.ALL;\n";
  out << "\n";
  out << "-- Uncomment the following library declaration if using\n";
  out << "-- arithmetic functions with Signed or Unsigned values\n";
  out << "--use IEEE.NUMERIC_STD.ALL;\n";
  out << "\n";
  out << "-- Uncomment the following library declaration if instantiating\n";
  out << "--library UNISIM;\n";
  out << "--use UNISIM.VComponents.all;\n";
  out << "\n";
  out << "entity aaa is\n";
  out << "--  Port ( );\n";
  out << "end aaa;\n";
  out << "\n";
  out << "architecture Behavioral of aaa is\n";
  out << "\n";
  out << "begin\n";
  out << "\n\n";
  out << "end Behavioral;\n";

  file.flush();
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
      proFileSet->addFile(fname, "$OSRCDIR" + filePath);
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

QString ProjectManager::getCurrentRun() const { return m_currentRun; }

void ProjectManager::setCurrentRun(const QString& currentRun) {
  m_currentRun = currentRun;
}

QString ProjectManager::currentFileSet() const { return m_currentFileSet; }

void ProjectManager::setCurrentFileSet(const QString& currentFileSet) {
  m_currentFileSet = currentFileSet;
}
