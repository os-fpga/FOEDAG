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
#include "ProjectManagerComponent.h"

#include "Compiler/CompilerDefines.h"
#include "Utils/QtUtils.h"
#include "Utils/StringUtils.h"

namespace FOEDAG {

constexpr auto GENERIC_OPTION{"Option"};
constexpr auto GENERIC_NAME{"Name"};
constexpr auto GENERIC_VAL{"Val"};

constexpr auto COMPILER_CONFIG{"CompilerConfig"};
constexpr auto SIMULATION_CONFIG{"SimulationConfig"};
constexpr auto OPTION{"Opt"};
constexpr auto NAME{"Name"};
constexpr auto VAL{"Val"};
constexpr auto LIB_PATH{"LibPath"};
constexpr auto INCLUDE_PATH{"IncludePath"};
constexpr auto LIB_EXT{"LibExt"};
constexpr auto MACRO{"Macro"};

constexpr auto PROJECT_GROUP_LIB_COMMAND{"LibCommand"};
constexpr auto PROJECT_GROUP_LIB_NAME{"LibName"};

constexpr auto IP_CONFIG{"IpConfig"};
constexpr auto IP_INSTANCE_PATHS{"InstancePaths"};
constexpr auto IP_CATALOG_PATHS{"CatalogPaths"};
constexpr auto IP_INSTANCE_CMDS{"InstanceCmds"};

ProjectManagerComponent::ProjectManagerComponent(ProjectManager* pManager,
                                                 QObject* parent)
    : ProjectFileComponent(parent), m_projectManager(pManager) {
  connect(m_projectManager, &ProjectManager::saveFile, this,
          &ProjectManagerComponent::saveFile);
}

void ProjectManagerComponent::Save(QXmlStreamWriter* writer) {
  QXmlStreamWriter& stream = *writer;
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
  stream.writeAttribute(PROJECT_VAL, QString::number(tmpProCfg->projectType()));
  stream.writeEndElement();

  QMap<QString, QString> tmpOption = tmpProCfg->getMapOption();
  for (auto iter = tmpOption.begin(); iter != tmpOption.end(); ++iter) {
    stream.writeStartElement(PROJECT_OPTION);
    stream.writeAttribute(PROJECT_NAME, iter.key());
    stream.writeAttribute(PROJECT_VAL, iter.value());
    stream.writeEndElement();
  }
  stream.writeEndElement();

  stream.writeStartElement(COMPILER_CONFIG);
  stream.writeStartElement(OPTION);
  stream.writeAttribute(NAME, LIB_PATH);
  stream.writeAttribute(VAL, m_projectManager->libraryPath());
  stream.writeEndElement();
  stream.writeStartElement(OPTION);
  stream.writeAttribute(NAME, INCLUDE_PATH);
  stream.writeAttribute(VAL, m_projectManager->includePath());
  stream.writeEndElement();
  stream.writeStartElement(OPTION);
  stream.writeAttribute(NAME, LIB_EXT);
  stream.writeAttribute(VAL, m_projectManager->libraryExtension());
  stream.writeEndElement();
  stream.writeStartElement(OPTION);
  stream.writeAttribute(NAME, MACRO);
  stream.writeAttribute(VAL, m_projectManager->macros());
  stream.writeEndElement();
  stream.writeEndElement();

  stream.writeStartElement(SIMULATION_CONFIG);
  stream.writeStartElement(OPTION);
  stream.writeAttribute(NAME, LIB_PATH);
  stream.writeAttribute(VAL, m_projectManager->libraryPathSim());
  stream.writeEndElement();
  stream.writeStartElement(OPTION);
  stream.writeAttribute(NAME, INCLUDE_PATH);
  stream.writeAttribute(VAL, m_projectManager->includePathSim());
  stream.writeEndElement();
  stream.writeStartElement(OPTION);
  stream.writeAttribute(NAME, LIB_EXT);
  stream.writeAttribute(VAL, m_projectManager->simLibraryExtension());
  stream.writeEndElement();
  stream.writeStartElement(OPTION);
  stream.writeAttribute(NAME, MACRO);
  stream.writeAttribute(VAL, m_projectManager->macrosSim());
  stream.writeEndElement();
  stream.writeEndElement();

  stream.writeStartElement(IP_CONFIG);
  stream.writeStartElement(GENERIC_OPTION);
  stream.writeAttribute(GENERIC_NAME, IP_INSTANCE_PATHS);
  stream.writeAttribute(GENERIC_VAL, m_projectManager->ipInstancePaths());
  stream.writeEndElement();
  stream.writeStartElement(GENERIC_OPTION);
  stream.writeAttribute(GENERIC_NAME, IP_CATALOG_PATHS);
  stream.writeAttribute(GENERIC_VAL, m_projectManager->ipCatalogPaths());
  stream.writeEndElement();
  stream.writeStartElement(GENERIC_OPTION);
  stream.writeAttribute(GENERIC_NAME, IP_INSTANCE_CMDS);
  stream.writeAttribute(GENERIC_VAL, m_projectManager->ipInstanceCmds());
  stream.writeEndElement();
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
      stream.writeAttribute(PROJECT_PATH, relatedPath(iterfile->second));
      stream.writeEndElement();
    }
    auto langMap = tmpFileSet->Files();
    auto libs = tmpFileSet->getLibraries();
    int index{0};
    for (auto it = langMap.cbegin(); it != langMap.cend(); ++it, index++) {
      stream.writeStartElement(PROJECT_GROUP);
      stream.writeAttribute(PROJECT_GROUP_ID,
                            QString::number(it->first.language));
      stream.writeAttribute(PROJECT_GROUP_NAME, it->first.group);
      stream.writeAttribute(PROJECT_GROUP_FILES, relatedPathList(it->second));
      stream.writeAttribute(PROJECT_GROUP_LIB_COMMAND,
                            libs.at(index).first.join(" "));
      stream.writeAttribute(PROJECT_GROUP_LIB_NAME,
                            libs.at(index).second.join(" "));
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
}

void ProjectManagerComponent::Load(QXmlStreamReader* r) {
  QXmlStreamReader& reader{*r};
  while (!reader.atEnd()) {
    QXmlStreamReader::TokenType type = reader.readNext();
    if (type == QXmlStreamReader::StartElement) {
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
              bool ok{true};
              auto type = reader.attributes().value(PROJECT_VAL).toInt(&ok);
              tmpProCfg->setProjectType(ok ? type : RTL);
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
        std::vector<std::pair<CompilationUnit, QString>> langList;
        std::vector<std::pair<QStringList, QStringList>> libs;
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
                absPath(reader.attributes().value(PROJECT_PATH).toString()));
          } else if (type == QXmlStreamReader::StartElement &&
                     reader.attributes().hasAttribute(PROJECT_NAME) &&
                     reader.attributes().hasAttribute(PROJECT_VAL)) {
            mapOption.insert(reader.attributes().value(PROJECT_NAME).toString(),
                             reader.attributes().value(PROJECT_VAL).toString());
          } else if (type == QXmlStreamReader::StartElement &&
                     reader.attributes().hasAttribute(PROJECT_GROUP_ID) &&
                     reader.attributes().hasAttribute(PROJECT_GROUP_FILES)) {
            QString raw{
                reader.attributes().value(PROJECT_GROUP_FILES).toString()};
            QStringList fileList = QtUtils::StringSplit(raw, ' ');
            langList.push_back(std::make_pair(
                CompilationUnit{
                    reader.attributes().value(PROJECT_GROUP_ID).toInt(),
                    reader.attributes().value(PROJECT_GROUP_NAME).toString()},
                absPathList(fileList)));
            auto command =
                reader.attributes().value(PROJECT_GROUP_LIB_COMMAND).toString();
            auto lib =
                reader.attributes().value(PROJECT_GROUP_LIB_NAME).toString();
            libs.push_back(std::make_pair(QtUtils::StringSplit(command, ' '),
                                          QtUtils::StringSplit(lib, ' ')));
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
            int index{0};
            auto projectPath = Project::Instance()->projectPath();
            for (const auto& i : langList) {
              auto designFiles = i.second;
              designFiles.replace(PROJECT_OSRCDIR, projectPath);
              projectFileset.addFiles(libs.at(index).first,
                                      libs.at(index).second,
                                      QtUtils::StringSplit(designFiles, ' '),
                                      i.first.language, i.first.group);
              index++;
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
            langList.clear();
            libs.clear();
          }
        }
      }
      if (reader.name() == COMPILER_CONFIG) {
        while (true) {
          type = reader.readNext();
          if (type == QXmlStreamReader::EndElement &&
              reader.name() == COMPILER_CONFIG) {
            break;
          }

          if (type == QXmlStreamReader::StartElement &&
              reader.attributes().hasAttribute(NAME) &&
              reader.attributes().hasAttribute(VAL)) {
            if (reader.attributes().value(NAME).toString() == LIB_PATH) {
              auto path = reader.attributes().value(VAL).toString();
              std::vector<std::string> pathList;
              StringUtils::tokenize(path.toStdString(), " ", pathList);
              m_projectManager->setLibraryPathList(pathList);
            }
            if (reader.attributes().value(NAME).toString() == INCLUDE_PATH) {
              auto inc = reader.attributes().value(VAL).toString();
              std::vector<std::string> incList;
              StringUtils::tokenize(inc.toStdString(), " ", incList);
              m_projectManager->setIncludePathList(incList);
            }
            if (reader.attributes().value(NAME).toString() == LIB_EXT) {
              auto ext = reader.attributes().value(VAL).toString();
              std::vector<std::string> extList;
              StringUtils::tokenize(ext.toStdString(), " ", extList);
              m_projectManager->setLibraryExtensionList(extList);
            }
            if (reader.attributes().value(NAME).toString() == MACRO) {
              auto macro = reader.attributes().value(VAL).toString();
              auto macroList = ProjectManager::ParseMacro(macro);
              m_projectManager->setMacroList(macroList);
            }
          }
        }
      }
      if (reader.name() == SIMULATION_CONFIG) {
        while (true) {
          type = reader.readNext();
          if (type == QXmlStreamReader::EndElement &&
              reader.name() == SIMULATION_CONFIG) {
            break;
          }

          if (type == QXmlStreamReader::StartElement &&
              reader.attributes().hasAttribute(NAME) &&
              reader.attributes().hasAttribute(VAL)) {
            if (reader.attributes().value(NAME).toString() == LIB_PATH) {
              auto path = reader.attributes().value(VAL).toString();
              std::vector<std::string> pathList;
              StringUtils::tokenize(path.toStdString(), " ", pathList);
              m_projectManager->setLibraryPathListSim(pathList);
            }
            if (reader.attributes().value(NAME).toString() == INCLUDE_PATH) {
              auto inc = reader.attributes().value(VAL).toString();
              std::vector<std::string> incList;
              StringUtils::tokenize(inc.toStdString(), " ", incList);
              m_projectManager->setIncludePathListSim(incList);
            }
            if (reader.attributes().value(NAME).toString() == LIB_EXT) {
              auto ext = reader.attributes().value(VAL).toString();
              std::vector<std::string> extList;
              StringUtils::tokenize(ext.toStdString(), " ", extList);
              m_projectManager->setSimLibraryExtensionList(extList);
            }
            if (reader.attributes().value(NAME).toString() == MACRO) {
              auto macro = reader.attributes().value(VAL).toString();
              auto macroList = ProjectManager::ParseMacro(macro);
              m_projectManager->setSimMacroList(macroList);
            }
          }
        }
      }
      if (reader.name() == IP_CONFIG) {
        while (true) {
          type = reader.readNext();
          if (type == QXmlStreamReader::EndElement &&
              reader.name() == IP_CONFIG) {
            break;
          }

          if (type == QXmlStreamReader::StartElement &&
              reader.attributes().hasAttribute(GENERIC_NAME) &&
              reader.attributes().hasAttribute(GENERIC_VAL)) {
            if (reader.attributes().value(GENERIC_NAME).toString() ==
                IP_INSTANCE_PATHS) {
              auto path = reader.attributes().value(GENERIC_VAL).toString();
              std::vector<std::string> pathList;
              StringUtils::tokenize(path.toStdString(), " ", pathList);
              m_projectManager->setIpInstancePathList(pathList);
            }
            if (reader.attributes().value(GENERIC_NAME).toString() ==
                IP_CATALOG_PATHS) {
              auto path = reader.attributes().value(GENERIC_VAL).toString();
              std::vector<std::string> pathList;
              StringUtils::tokenize(path.toStdString(), " ", pathList);
              m_projectManager->setIpCatalogPathList(pathList);
            }
            if (reader.attributes().value(GENERIC_NAME).toString() ==
                IP_INSTANCE_CMDS) {
              QString cmdsStr =
                  reader.attributes().value(GENERIC_VAL).toString();
              // Using QStringList for multi-char split() function
              QStringList cmds = cmdsStr.split("_IP_CMD_SEP_");
              // Convert to std::vector<std::string>
              std::vector<std::string> cmdList;
              for (auto cmd : cmds) {
                cmdList.push_back(cmd.toStdString());
              }
              m_projectManager->setIpInstanceCmdList(cmdList);
            }
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
        break;
      }
    }
  }
  const auto constrSets = m_projectManager->getConstrFileSets();
  for (const auto& set : constrSets) {
    const auto files = m_projectManager->getConstrFiles(set);
    for (const auto& f : files) {
      const int ret = FOEDAG::read_sdc(f);
      if (ret != 0) {
        break;
      }
    }
  }
}

QString ProjectManagerComponent::relatedPath(const QString& path) const {
  std::filesystem::path p = path.toStdString();
  if (!p.is_absolute()) return path;

  std::filesystem::path project_p = m_projectManager->projectPath();
  auto relative_p = std::filesystem::relative(p, project_p);
  relative_p = PROJECT_OSRCDIR / relative_p;
  return QString::fromStdString(relative_p.string());
}

QString ProjectManagerComponent::absPath(const QString& path) const {
  if (path.contains(PROJECT_OSRCDIR)) {
    QString absPath = path;
    auto replated =
        absPath.replace(PROJECT_OSRCDIR, m_projectManager->getProjectPath());
    std::filesystem::path path{replated.toStdString()};
    path = std::filesystem::canonical(path);
    return QString::fromStdString(path.string());
  }
  return path;
}

QString ProjectManagerComponent::relatedPathList(
    const QStringList& pathList) const {
  QStringList files{};
  for (const auto& file : pathList) files.append(relatedPath(file));
  return files.join(' ');
}

QString ProjectManagerComponent::absPathList(
    const QStringList& pathList) const {
  QStringList files{};
  for (const auto& file : pathList) files.append(absPath(file));
  return files.join(' ');
}

}  // namespace FOEDAG
