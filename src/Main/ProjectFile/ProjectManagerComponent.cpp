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

namespace FOEDAG {

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
              projectFileset.addFiles(
                  ProjectManager::StringSplit(i.second, " "), i.first);
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
        break;
      }
    }
  }
}
}  // namespace FOEDAG
