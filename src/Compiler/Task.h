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
#pragma once

#include <QObject>
#include <QVariant>
#include <QVector>

namespace FOEDAG {

enum class TaskStatus {
  None,
  InProgress,
  Success,
  Fail,
};

enum class TaskType {
  None,
  Action,
  Settings,
  Button,
  Clean,
};

enum class CustomDataType {
  None,
  Sim,
};

struct CustomData {
  CustomDataType type{CustomDataType::None};
  QVariant data;
};

/*!
 * \brief The Task class
 * Implements task entity.
 */
class Task : public QObject {
  Q_OBJECT
 public:
  explicit Task(QObject *parent = nullptr);
  explicit Task(const QString &title, TaskType tType = TaskType::Action,
                QObject *parent = nullptr);
  const QString &title() const;
  void setTitle(const QString &newTitle);
  bool hasSubTask() const;
  void appendSubTask(Task *t);
  Task *parentTask() const;

  TaskStatus status() const;
  void setStatus(TaskStatus newStatus);

  TaskType type() const;
  void setTaskType(TaskType newType);

  QString settingsKey() const;
  void setSettingsKey(QString key);

  QString logFileReadPath() const;
  void setLogFileReadPath(QString key);

  QString abbreviation() const;
  void setAbbreviation(QString key);

  void setCustomData(const CustomData &data);
  const CustomData &cusomData() const;

  /*!
   * \brief trigger
   * Emits \a taskTriggered() signal to notify that some commend can be run.
   */
  void trigger();
  const QVector<Task *> &subTask() const;

  /*!
   * \brief isValid
   * \return true if task is valid. The valid task can run some command. If task
   * not valid it will not be run.
   */
  bool isValid() const;
  /*!
   * \brief setValid
   * Change valid state of the task.
   */
  void setValid(bool newValid);

  bool isEnable() const;
  bool isEnableDefault() const;
  void setEnable(bool newEnable, bool enableDefault = true);

  Task *cleanTask() const;
  void setCleanTask(Task *newClean);

 signals:
  /*!
   * \brief statusChanged. Emits whenever status has changed.
   */
  void statusChanged(FOEDAG::TaskStatus);
  /*!
   * \brief taskTriggered. Emits when user trigger the task.
   */
  void taskTriggered();
  void enableChanged();

 private:
  QString m_title;
  QString m_abbreviation;
  QString m_settings_key;
  TaskStatus m_status{TaskStatus::None};
  TaskType m_type{TaskType::Action};
  QVector<Task *> m_subTask;
  Task *m_parent{nullptr};
  Task *m_clean{nullptr};
  bool m_valid{false};
  QString m_logFilePath{};
  bool m_enable{true};
  bool m_enableDefault{true};
  CustomData m_customData{};
};

}  // namespace FOEDAG

Q_DECLARE_METATYPE(FOEDAG::TaskStatus)
