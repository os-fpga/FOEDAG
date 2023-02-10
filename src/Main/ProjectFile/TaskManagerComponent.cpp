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
#include "TaskManagerComponent.h"

namespace FOEDAG {

TaskManagerComponent::TaskManagerComponent(TaskManager *taskManager,
                                           QObject *parent)
    : ProjectFileComponent(parent), m_taskManager(taskManager) {
  connect(m_taskManager, &TaskManager::done, this,
          &TaskManagerComponent::saveFile);
  connect(m_taskManager, &TaskManager::enableChanged, this,
          &TaskManagerComponent::saveFile);
}

void TaskManagerComponent::Save(QXmlStreamWriter *writer) {
  if (m_taskManager) {
    writer->writeStartElement(TASK_MAIN);
    ProjectFileComponent::Save(writer);
    for (auto &task : m_taskManager->tasks()) {
      if (task->type() != TaskType::Action) continue;
      writer->writeStartElement(TASK_NAME);
      writer->writeAttribute(TASK_ID,
                             QString::number(m_taskManager->taskId(task)));
      writer->writeAttribute(TASK_STATUS,
                             QString::number(static_cast<int>(task->status())));
      writer->writeAttribute(
          TASK_ENABLE, QString::number(static_cast<int>(task->isEnable())));
      writer->writeEndElement();
    }
    writer->writeEndElement();
  }
}

void TaskManagerComponent::Load(QXmlStreamReader *reader) {
  while (!reader->atEnd()) {
    QXmlStreamReader::TokenType type = reader->readNext();
    if (type == QXmlStreamReader::StartElement) {
      if (reader->name() == TASK_MAIN) {
        while (true) {
          type = reader->readNext();
          if (type == QXmlStreamReader::EndElement &&
              reader->name() == TASK_MAIN)
            break;

          if (type == QXmlStreamReader::StartElement) {
            if (reader->name() == TASK_NAME) {
              uint id = reader->attributes().value(TASK_ID).toUInt();
              TaskStatus status = static_cast<TaskStatus>(
                  reader->attributes().value(TASK_STATUS).toInt());
              auto task = m_taskManager->task(id);
              if (task) {
                task->blockSignals(true);
                bool enable = task->isEnableDefault();
                if (reader->attributes().hasAttribute(TASK_ENABLE))
                  enable =
                      (reader->attributes().value(TASK_ENABLE).toInt() == 1);
                task->setStatus(status);
                task->setEnable(enable);
                task->blockSignals(false);
              }
            }
          }
        }
      }
      break;
    }
  }
}
}  // namespace FOEDAG
