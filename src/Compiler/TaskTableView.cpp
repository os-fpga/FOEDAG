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
#include "TaskTableView.h"

#include <QApplication>
#include <QBoxLayout>
#include <QDebug>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMovie>
#include <QPushButton>

#include "Compiler.h"
#include "NewProject/ProjectManager/project.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "TaskGlobal.h"
#include "TaskManager.h"

inline void initializeResources() { Q_INIT_RESOURCE(compiler_resources); }
namespace FOEDAG {

const QString TaskTableView::TasksDelegate::LOADING_GIF = ":/loading.gif";

TaskTableView::TaskTableView(TaskManager *tManager, QWidget *parent)
    : QTableView(parent), m_taskManager(tManager) {
  verticalHeader()->hide();
  auto delegate = new TasksDelegate(*this, this);
  setItemDelegateForColumn(StatusCol, delegate);
  setItemDelegateForColumn(TitleCol, delegate);
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(customMenuRequested(const QPoint &)));
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  initializeResources();
}

void TaskTableView::mousePressEvent(QMouseEvent *event) {
  auto idx = indexAt(event->pos());
  if (idx.column() == TitleCol) {
    bool expandAreaClicked = expandArea(idx).contains(event->pos());
    if (expandAreaClicked) {
      model()->setData(idx, ExpandAreaAction::Invert, ExpandAreaRole);
    }
  }

  QTableView::mousePressEvent(event);
}

void TaskTableView::mouseDoubleClickEvent(QMouseEvent *event) {
  // We only disabled the viewport in order to have scrollbar enabled.
  // Mouse events keep on coming in this case though, so we catch them manually.
  if (m_viewDisabled) return;
  auto idx = indexAt(event->pos());
  if (idx.isValid() && (idx.column() == TitleCol) &&
      !expandArea(idx).contains(event->pos())) {
    userActionHandle(idx);
  }
  QTableView::mouseDoubleClickEvent(event);
}

void TaskTableView::setModel(QAbstractItemModel *model) {
  QTableView::setModel(model);
  for (int i = 0; i < this->model()->rowCount(); i++) {
    auto statusIndex = this->model()->index(i, StatusCol);
    // Table view can't play gif animations automatically, so we set QLabel,
    // which can.
    setIndexWidget(statusIndex, new QLabel);
    auto task = m_taskManager->task(statusIndex.data(TaskId).toUInt());
    if (task && ((task->type() == TaskType::Settings) ||
                 (task->type() == TaskType::Button))) {
      auto index = this->model()->index(i, TitleCol);
      auto button = new QPushButton(task->title());
      button->setObjectName(task->title());
      if (task->type() == TaskType::Settings)
        connect(button, &QPushButton::clicked, this,
                [=]() { emit TaskDialogRequested(task->settingsKey()); });
      else
        connect(button, &QPushButton::clicked, this, [=]() {
          this->model()->setData(index, QVariant(), UserActionRole);
        });
      setIndexWidget(index, button);
    }
  }
}

void TaskTableView::setViewDisabled(bool disabled) {
  m_viewDisabled = disabled;
  viewport()->setEnabled(!m_viewDisabled);
}

void TaskTableView::customMenuRequested(const QPoint &pos) {
  // We only disabled the viewport in order to have scrollbar enabled.
  // Mouse events keep on coming in this case though, so we catch them manually.
  if (m_viewDisabled) return;
  QModelIndex index = indexAt(pos);
  if (index.column() == TitleCol) {
    auto task = m_taskManager->task(model()->data(index, TaskId).toUInt());
    if (!task) return;
    if (task->type() == TaskType::Button || task->type() == TaskType::Settings)
      return;

    QMenu *menu = new QMenu(this);
    if (task->type() != TaskType::None) {
      QAction *start = new QAction("Run", this);
      connect(start, &QAction::triggered, this,
              [this, index]() { userActionHandle(index); });
      menu->addAction(start);
      if (task->cleanTask() != nullptr) {
        QAction *clean = new QAction("Clean", this);
        connect(clean, &QAction::triggered, this,
                [this, index]() { userActionCleanHandle(index); });
        menu->addAction(clean);
      }
      if (TaskManager::isSimulation(task)) {
        QAction *view = new QAction("View waveform", this);
        connect(view, &QAction::triggered, this,
                [this, task]() { emit ViewWaveform(task); });
        menu->addAction(view);
      }
      addTaskLogAction(menu, task);
      menu->addSeparator();
    }
    addExpandCollapse(menu);
    menu->popup(viewport()->mapToGlobal(pos));
  }
}

void TaskTableView::userActionHandle(const QModelIndex &index) {
  model()->setData(index, QVariant(), UserActionRole);
}

void TaskTableView::userActionCleanHandle(const QModelIndex &index) {
  model()->setData(index, QVariant(), UserActionCleanRole);
}

void TaskTableView::dataChanged(const QModelIndex &topLeft,
                                const QModelIndex &bottomRight,
                                const QVector<int> &roles) {
  if (roles.contains(Qt::DecorationRole)) {
    auto indexRow = topLeft.row();
    while (indexRow <= bottomRight.row()) {
      setRowHidden(indexRow,
                   model()->data(bottomRight, RowVisibilityRole).toBool());
      indexRow++;
    }
  }
  QTableView::dataChanged(topLeft, bottomRight, roles);
}

QRect TaskTableView::expandArea(const QModelIndex &index) const {
  QStyleOptionViewItem opt;
  opt.initFrom(this);
  opt.rect = visualRect(index);
  auto r = style()->proxy()->subElementRect(QStyle::SE_ItemViewItemDecoration,
                                            &opt, this);
  int h = rowHeight(index.row());
  r.setTopLeft(r.topLeft() + QPoint{20, 0});  //  move out of the check box
  r.setSize({h, h});
  return r;
}

void TaskTableView::addTaskLogAction(QMenu *menu, FOEDAG::Task *task) {
  // Grab the title and log file path of the given task
  QString title{task->title()};
  QString logFilePath{task->logFileReadPath()};
  auto parent = task->parentTask();
  // If this is a sub-task
  if (parent != nullptr) {
    // If the sub-task has its own log file
    if (!logFilePath.isEmpty() && parent != nullptr) {
      title = parent->title() + " - " + title;
      logFilePath = parent->logFileReadPath();
    } else {
      // otherwise take parent title and log file
      title = parent->title();
      logFilePath = parent->logFileReadPath();
    }
  }

  // Create View Log action and disable it if the file doesn't exist
  QString viewLogStr = "View " + title + " Logs";
  QAction *viewLog = new QAction(viewLogStr, this);
  auto compiler = m_taskManager->GetCompiler();
  auto action = static_cast<Compiler::Action>(
      FOEDAG::toAction(m_taskManager->taskId(task)));
  auto filePath = compiler->FilePath(action);
  logFilePath.replace(PROJECT_OSRCDIR,
                      QString::fromStdString(filePath.string()));
  auto logExists = QFile::exists(logFilePath);
  viewLog->setEnabled(logExists);
  connect(viewLog, &QAction::triggered, this,
          [this, logFilePath]() { emit ViewFileRequested(logFilePath); });
  menu->addAction(viewLog);

  auto taskId = m_taskManager->taskId(task);
  auto reportManager =
      m_taskManager->getReportManagerRegistry().getReportManager(taskId);
  if (reportManager) {
    for (auto &reportId : reportManager->getAvailableReportIds()) {
      auto viewReportStr = "View " + reportId;
      auto *viewReport = new QAction(viewReportStr, this);
      viewReport->setEnabled(logExists);
      connect(viewReport, &QAction::triggered, this, [this, task, reportId]() {
        emit ViewReportRequested(task, reportId);
      });
      menu->addAction(viewReport);
    }
  }
}

void TaskTableView::addExpandCollapse(QMenu *menu) {
  auto areaAction = [this](ExpandAreaAction action) {
    for (int row{0}; row < model()->rowCount(); row++)
      model()->setData(model()->index(row, TitleCol),
                       QVariant::fromValue(action), ExpandAreaRole);
  };
  QAction *expandAll = new QAction{"Expand All", this};
  connect(expandAll, &QAction::triggered, this,
          [areaAction]() { areaAction(ExpandAreaAction::Expand); });
  menu->addAction(expandAll);
  QAction *collapse = new QAction{"Collapse All", this};
  connect(collapse, &QAction::triggered, this,
          [areaAction]() { areaAction(ExpandAreaAction::Collapse); });
  menu->addAction(collapse);
}

TaskTableView::TasksDelegate::TasksDelegate(TaskTableView &view,
                                            QObject *parent)
    : QStyledItemDelegate(parent),
      m_view{view},
      m_inProgressMovie{new QMovie(LOADING_GIF, {}, &view)} {}

void TaskTableView::TasksDelegate::paint(QPainter *painter,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const {
  if (index.column() == TitleCol && index.data(ParentDataRole).toBool()) {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    opt.rect.setTopLeft(opt.rect.topLeft() - QPoint(-30, 0));
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    return;
  }
  if (index.column() == StatusCol) {
    auto statusData = index.data(Qt::DecorationRole);
    auto label = qobject_cast<QLabel *>(m_view.indexWidget(index));
    if (!label) return;
    // QTableView can't paint animations. Do it manually via QLabel.
    if (statusData.type() == QVariant::Bool) {
      label->setMovie(m_inProgressMovie);
      // Place the animation to cells left side, similar to other decorations
      label->move(m_view.visualRect(index).center() -
                  (label->rect().center() - QPoint{6, 0}));
      m_inProgressMovie->start();
      return;
    } else {
      // Reset the movie when task is no longer in progress
      label->setMovie(nullptr);
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.decorationAlignment = Qt::AlignCenter;
    opt.decorationPosition = QStyleOptionViewItem::Top;
    opt.decorationSize = QSize{20, 20};
    opt.showDecorationSelected = false;
    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    opt.icon = icon;
    opt.rect = opt.rect.adjusted(0, 4, 0, 0);
    QStyledItemDelegate::paint(painter, opt, index);
    return;
  }
  QStyledItemDelegate::paint(painter, option, index);
}

}  // namespace FOEDAG
