#include "project_manager.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QXmlStreamWriter>

ProjectManager::ProjectManager(QObject* parent) : QObject(parent) {
  Project::Instance()->InitProject();
}

int ProjectManager::CreateProject(const QString& strName,
                                  const QString& strPath) {
  int ret = 0;
  if ("" == strName || "" == strPath) {
    return -1;
  }
  Project::Instance()->setProjectName(strName);
  Project::Instance()->setProjectPath(strPath);
  ret = CreateProjectDir();
  if (0 != ret) {
    return ret;
  }

  ProjectFileSet* proFileSet = new ProjectFileSet();
  proFileSet->setSetName(DEFAULT_FOLDER_CONSTRS);
  proFileSet->setSetType(PROJECT_FILE_TYPE_CS);
  proFileSet->setRelSrcDir("/" + strName + ".srcs/" + DEFAULT_FOLDER_CONSTRS);
  Project::Instance()->setProjectFileset(proFileSet);

  proFileSet = new ProjectFileSet();
  proFileSet->setSetName(DEFAULT_FOLDER_SOURCE);
  proFileSet->setSetType(PROJECT_FILE_TYPE_DS);
  proFileSet->setRelSrcDir("/" + strName + ".srcs/" + DEFAULT_FOLDER_SOURCE);
  Project::Instance()->setProjectFileset(proFileSet);

  proFileSet = new ProjectFileSet();
  proFileSet->setSetName(DEFAULT_FOLDER_SIM);
  proFileSet->setSetType(PROJECT_FILE_TYPE_SS);
  proFileSet->setRelSrcDir("/" + strName + ".srcs/" + PROJECT_FILE_TYPE_SS);
  Project::Instance()->setProjectFileset(proFileSet);

  ProjectRun* proRun = new ProjectRun();
  proRun->setRunName(DEFAULT_FOLDER_IMPLE);
  proRun->setRunType(RUN_TYPE_IMPLEMENT);
  proRun->setSrcSet(DEFAULT_FOLDER_SOURCE);
  proRun->setConstrsSet(DEFAULT_FOLDER_CONSTRS);
  proRun->setRunState(RUN_STATE_CURRENT);
  proRun->setSynthRun(DEFAULT_FOLDER_SYNTH);
  Project::Instance()->setProjectRun(proRun);

  proRun = new ProjectRun();
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
      if ("v" == suffix || "vhd" == suffix) {
        setFileSet(strfile, isFileCopy);
      }
    }
  } else if (fileInfo.exists()) {
    if ("v" == suffix || "vhd" == suffix) {
      setFileSet(strFileName, isFileCopy);
    }
  } else {
    if (strFileName.contains("/")) {
      if ("v" == suffix) {
        CreateVerilogFile(strFileName);
        setFileSet(strFileName, isFileCopy);
      } else if ("vhd" == suffix) {
        CreateVHDLFile(strFileName);
        setFileSet(strFileName, isFileCopy);
      }
    } else {
      QString filePath = Project::Instance()->projectPath() + "/" +
                         Project::Instance()->projectName() + ".srcs/" +
                         m_currentFileSet + "/" + strFileName;
      QString fileSetPath = "$OSRCDIR/" + Project::Instance()->projectName() +
                            ".srcs/" + m_currentFileSet + "/" + strFileName;
      if ("v" == suffix) {
        CreateVerilogFile(filePath);
        setFileSet(fileSetPath, false);
      } else if ("vhd" == suffix) {
        CreateVHDLFile(filePath);
        setFileSet(fileSetPath, false);
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
      if ("SDC" == suffix) {
        setFileSet(strfile, isFileCopy);
      }
    }
  } else if (fileInfo.exists()) {
    if ("SDC" == suffix) {
      setFileSet(strFileName, isFileCopy);
    }
  } else {
    if (strFileName.contains("/")) {
      if ("SDC" == suffix) {
        CreateSDCFile(strFileName);
        setFileSet(strFileName, isFileCopy);
      }
    } else {
      QString filePath = Project::Instance()->projectPath() + "/" +
                         Project::Instance()->projectName() + ".srcs/" +
                         m_currentFileSet + "/" + strFileName;
      QString fileSetPath = "$OSRCDIR/" + Project::Instance()->projectName() +
                            ".srcs/" + m_currentFileSet + "/" + strFileName;
      if ("SDC" == suffix) {
        CreateSDCFile(filePath);
        setFileSet(fileSetPath, false);
      }
    }
  }
  return ret;
}

int ProjectManager::setRunSet(QList<QString> listParam) {
  int ret = 0;
  if (listParam.size() < 4) {
    return -1;
  }
  ProjectRun* proRun = Project::Instance()->getProjectRun(m_currentRun);
  if (nullptr == proRun || RUN_TYPE_SYNTHESIS != proRun->runType()) {
    return -2;
  }
  proRun->setOption("Series", listParam.at(0));
  proRun->setOption("Family", listParam.at(1));
  proRun->setOption("Package", listParam.at(2));
  proRun->setOption("Device", listParam.at(3));
  return ret;
}

int ProjectManager::StartProject(const QString& strOspro) {
  return ImportProjectData(strOspro);
}

void ProjectManager::FinishedProject() { ExportProjectData(); }

int ProjectManager::ImportProjectData(QString strOspro) {
  Q_UNUSED(strOspro);
  return 0;
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

  stream.writeStartElement(PROJECT_OPTION);
  stream.writeAttribute(PROJECT_NAME, PROJECT_CONFIG_SIMTOPMODULE);
  stream.writeAttribute(PROJECT_VAL, tmpProCfg->simulationTopMoule());
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

    if (!dir.mkdir(tmpPath + "/" + tmpName + ".srcs/" +
                   DEFAULT_FOLDER_CONSTRS)) {
      ret = -2;
      break;
    }

    if (!dir.mkdir(tmpPath + "/" + tmpName + ".srcs/" + DEFAULT_FOLDER_SIM)) {
      ret = -2;
      break;
    }

    if (!dir.mkdir(tmpPath + "/" + tmpName + ".srcs/" +
                   DEFAULT_FOLDER_SOURCE)) {
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

int ProjectManager::CreateFolder(QString strPath) {
  QDir dir(strPath);
  if (!dir.exists()) {
    if (!dir.mkpath(strPath)) {
      return -2;
    }
  }
  return 0;
}

int ProjectManager::CreateVerilogFile(QString strFile) {
  int ret = 0;
  QFile file(strFile);
  if (file.exists()) return ret;
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
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

int ProjectManager::setFileSet(const QString& strFileName, bool isFileCopy) {
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
    CopyFileToPath(strFileName, destinDir);
    proFileSet->addFile(fname, "$OSRCDIR" + filePath);

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
