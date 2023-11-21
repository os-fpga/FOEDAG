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

#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QTableView>

class QMovie;
class QCheckBox;

namespace FOEDAG {

class TaskManager;
class Task;

/*!
 * \brief The TaskTableView class
 * Implements view for tasl manager. Some of the rows could be expandable like
 * in the tree view.
 */
class TaskTableView : public QTableView {
  Q_OBJECT
 public:
  explicit TaskTableView(TaskManager *tManager, QWidget *parent = nullptr);
  ~TaskTableView() override;
  void setModel(QAbstractItemModel *model) override;
  void setViewDisabled(bool disabled);
  void updateEnableColumn();

  void updateLastColumn();

 protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

 private slots:
  void customMenuRequested(const QPoint &pos);
  void userActionHandle(const QModelIndex &index);
  void userActionCleanHandle(const QModelIndex &index);
  void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                   const QVector<int> &roles) override;

 signals:
  void TaskDialogRequested(const QString &category, const QString &path);
  void ViewFileRequested(const QString &filePath);
  void ViewReportRequested(FOEDAG::Task *task, const QString &reportId);
  void ViewWaveform(FOEDAG::Task *task);

 private:
  QRect contextArea(const QModelIndex &index) const;
  void addTaskLogAction(QMenu *menu, Task *task);
  void addTaskViewWaveformAction(QMenu *menu, Task *task);
  void TaskDialogRequestedHandler(Task *task);

 private:
  /*!
   * \brief The TasksDelegate class for painting TaskTableView.
   * Takes care of painting following columns:
   * - Status - makes sure animation is painted when task has InProgress status
   * - Title - Implements the item that has parent and it looks similar like in
   * the tree view.
   */
  class TasksDelegate : public QStyledItemDelegate {
   public:
    explicit TasksDelegate(TaskTableView &view, QObject *parent = nullptr);

   protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

   private:
    static const QString LOADING_GIF;

    TaskTableView &m_view;
    QMovie *m_inProgressMovie{nullptr};  // In progress gif animation
  };

  TaskManager *m_taskManager{nullptr};
  static constexpr int StatusCol{0};
  static constexpr int TitleCol{1};
  static constexpr int FMaxCol{2};
  // Indicates that view is disabled and shouldn't be interactive
  bool m_viewDisabled{false};
  QMap<QModelIndex, QCheckBox *> m_enableCheck{};
};
}  // namespace FOEDAG
