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
#include "ProjectFileLoader.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QXmlStreamWriter>

#include "Compiler/CompilerDefines.h"
#include "FileLoaderOldStructure.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjectFileComponent.h"
#include "foedag_version.h"
#include "scope_guard/scope_guard.hpp"

namespace FOEDAG {

static constexpr bool ERROR{true};
static constexpr bool PASS{false};

static const QString MessageTitle{"Project Migration Tool"};

ProjectFileLoader::ProjectFileLoader(Project *project, QObject *parent)
    : QObject(parent) {
  connect(Project::Instance(), &Project::saveFile, this,
          &ProjectFileLoader::Save);
  m_components.resize(static_cast<size_t>(ComponentId::Count), nullptr);
}

ProjectFileLoader::~ProjectFileLoader() {
  for (const auto &component : m_components) delete component;
}

void ProjectFileLoader::registerComponent(ProjectFileComponent *comp,
                                          ComponentId id) {
  connect(comp, &ProjectFileComponent::saveFile, this, [this]() { Save(); });
  m_components[static_cast<size_t>(id)] = comp;
}

ErrorCode ProjectFileLoader::Load(const QString &filename) {
  m_loadDone = false;
  auto result = LoadInternal(filename);
  m_loadDone = true;
  if (!result.errorCode && result.migrationDoneSuccessfully) Save();
  return result.errorCode;
}

void ProjectFileLoader::setParentWidget(QWidget *parent) { m_parent = parent; }

ProjectFileLoader::LoadResult ProjectFileLoader::LoadInternal(
    const QString &filename) {
  if (filename.isEmpty()) return {{ERROR, "Empty project filename"}};

  QString strTemp = QString("%1/%2%3").arg(Project::Instance()->projectPath(),
                                           Project::Instance()->projectName(),
                                           PROJECT_FILE_FORMAT);
  if (filename == strTemp) return {};

  // this is starting point for backward compatibility
  QString version = ProjectVersion(filename);
  if (version.isEmpty()) return {{ERROR, "Failed to get project version"}};
  bool ok{};
  auto ver = toVersion(version, &ok);
  ProjectFileComponent *compilerComponent{nullptr};
  ProjectFileComponent *taskComponent{nullptr};

  bool compatibleOk{};
  Version compatibleVersion = toVersion(
      QString::fromLatin1(TO_C_STR(FOEDAG_VERSION_COMPAT)), &compatibleOk);
  bool migrationDoneSuccessfully{false};
  if (ok && compatibleOk && (ver < compatibleVersion)) {
    QString newVersion{TO_C_STR(FOEDAG_VERSION)};
    QString path{QFileInfo{filename}.absolutePath()};
    if (m_parent) {
      auto btn = QMessageBox::warning(
          m_parent, MessageTitle,
          QString{"Project created with older version %1 is incompatible with "
                  "current version %2. Press OK to proceed with the migration "
                  "automatically.\nNote: Backup will be available as a zip "
                  "once it is successfully completed at %3"}
              .arg(version, newVersion, path),
          QMessageBox::Ok | QMessageBox::Cancel);
      if (btn == QMessageBox::Cancel) return {{PASS, {}}};
    }
    compilerComponent = m_components[static_cast<int>(ComponentId::Compiler)];
    taskComponent = m_components[static_cast<int>(ComponentId::TaskManager)];
    m_components[static_cast<int>(ComponentId::Compiler)] = nullptr;
    m_components[static_cast<int>(ComponentId::TaskManager)] = nullptr;

    const FileLoaderOldStructure loader{filename};
    auto result = loader.Migrate();
    if (!result.first) return {{ERROR, result.second}};
    if (m_parent)
      QMessageBox::information(
          m_parent, MessageTitle,
          QString{"Successfully migrated the project to %1 version. Backup is "
                  "available at %2\nNote: Please verify the project before "
                  "compiling again. Delete any unnecessary files that are not "
                  "part of the design."}
              .arg(newVersion, path));
    migrationDoneSuccessfully = true;
  }

  auto guard = sg::make_scope_guard([this, compilerComponent, taskComponent] {
    if (compilerComponent)
      m_components[static_cast<int>(ComponentId::Compiler)] = compilerComponent;
    if (taskComponent)
      m_components[static_cast<int>(ComponentId::TaskManager)] = taskComponent;
  });

  QFile file(filename);
  if (!file.open(QFile::ReadOnly | QFile::Text))
    return {{ERROR, QString{"Failed to open project file %1"}.arg(filename)}};

  Project::Instance()->InitProject();
  QXmlStreamReader reader;
  reader.setDevice(&file);
  while (!reader.atEnd()) {
    QXmlStreamReader::TokenType type = reader.readNext();
    if (type == QXmlStreamReader::StartElement) {
      if (reader.name() == PROJECT_PROJECT) {
        QFileInfo path(filename);
        QString projPath = path.absolutePath();
        QString strName = path.baseName();
        Project::Instance()->setProjectName(strName);
        Project::Instance()->setProjectPath(projPath);
      }
      for (const auto &component : m_components) {
        if (component) {
          auto errorCode = component->Load(&reader);
          if (errorCode) return {errorCode};
        }
      }
    }
    if (reader.hasError()) break;
  }

  // set device
  auto proRun = Project::Instance()->getProjectRun(DEFAULT_FOLDER_SYNTH);
  if (proRun) {
    const auto device = proRun->getOption(PROJECT_PART_DEVICE);
    if (!device.isEmpty()) {
      target_device(device);
    }
  }

  for (const auto &component : m_components)
    if (component) component->LoadDone();

  if (reader.hasError()) {
    return {{ERROR, QString{"Project file syntax issue: %1"}.arg(
                        reader.errorString())}};
  }
  file.close();
  return {ErrorCode{}, migrationDoneSuccessfully};
}

QString FOEDAG::ProjectFileLoader::ProjectVersion(const QString &filename) {
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

void ProjectFileLoader::Save() {
  if (!m_loadDone) return;
  QString tmpName = Project::Instance()->projectName();
  QString tmpPath = Project::Instance()->projectPath();
  QString xmlPath = tmpPath + "/" + tmpName + PROJECT_FILE_FORMAT;
  QFile file(xmlPath);
  if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
    return;
  }
  QXmlStreamWriter stream(&file);
  stream.setAutoFormatting(true);
  stream.writeStartDocument();
  stream.writeComment(
      tr("                                                   "));
  stream.writeComment(
      tr("Copyright (c) 2021-2022 The Open-Source FPGA Foundation."));
  stream.writeStartElement(PROJECT_PROJECT);
  stream.writeAttribute(PROJECT_VERSION, TO_C_STR(FOEDAG_VERSION));

  for (const auto &component : m_components) component->Save(&stream);

  stream.writeEndDocument();
  file.close();
}

}  // namespace FOEDAG
