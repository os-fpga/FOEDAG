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
class Compiler;
class TaskManager;

/* This class shows reports in a tree view. At shown project task
 * as a parent and all available report ids as its children.
 * Double clicking on leaf node (report id) opens it in application
 * editor. In case the report has been shown already, corresponding
 * tab just gets activated.
 */
class ReportsTreeWidget final : public QWidget {
  Q_OBJECT
 public:
  ReportsTreeWidget(Compiler *compiler, const TaskManager &taskManager);

 private slots:
  // Reacts on double click on one of tree items.
  void onReportRequested(const QTreeWidgetItem *item, int col);

 private:
  Compiler *m_compiler;
  const TaskManager &m_taskManager;
};

}  // namespace FOEDAG
