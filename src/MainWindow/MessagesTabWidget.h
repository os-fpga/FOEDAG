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

#include <QWidget>

class QTreeWidgetItem;

namespace FOEDAG {
class TaskManager;
struct TaskMessage;
class MessageItemParser;

class MessagesTabWidget final : public QWidget {
  Q_OBJECT
 public:
  MessagesTabWidget(const TaskManager &taskManager);
  ~MessagesTabWidget();

 private slots:
  // Reacts on double click on one of tree items.
  void onMessageClicked(const QTreeWidgetItem *item, int col);

 private:
  QTreeWidgetItem *createTaskMessageItem(const TaskMessage &msg,
                                         const QString &filePath);
  QTreeWidgetItem *createItem(const QString &message);
  static QString createLink(const QString &str);
  const TaskManager &m_taskManager;
  QVector<QTreeWidgetItem *> m_convertToLabel;
  const QVector<MessageItemParser *> m_parsers;
};

}  // namespace FOEDAG
