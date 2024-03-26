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
#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMovie>
#include <QPushButton>

#include "Compiler.h"
#include "Main/Settings.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "TaskGlobal.h"
#include "TaskManager.h"
#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"

inline void initializeResources() { Q_INIT_RESOURCE(compiler_resources); }
namespace FOEDAG {

const QString TaskTableView::TasksDelegate::LOADING_GIF = ":/loading.gif";

TaskTableView::TaskTableView(TaskManager *tManager, QWidget *parent)
    : QTableView(parent), m_taskManager(tManager) {
  verticalHeader()->hide();
  auto delegate = new TasksDelegate(*this, this);
  setItemDelegateForColumn(TitleCol, delegate);
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &TaskTableView::customContextMenuRequested, this,
          &TaskTableView::customMenuRequested);
  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  setMouseTracking(true);
  initializeResources();
}

TaskTableView::~TaskTableView() { qDeleteAll(m_enableCheck); }

void TaskTableView::mousePressEvent(QMouseEvent *event) {
  auto idx = indexAt(event->pos());
  if (idx.column() == TitleCol) {
    bool expandAreaClicked = contextArea(idx).contains(event->pos());
    if (expandAreaClicked) {
      customMenuRequested(event->pos());
      return;
    }
  }

  QTableView::mousePressEvent(event);
}

void TaskTableView::mouseMoveEvent(QMouseEvent *e) {
  auto index = indexAt(e->pos());
  if (index.column() == TitleCol) update(index);
  QTableView::mouseMoveEvent(e);
}

void TaskTableView::mouseDoubleClickEvent(QMouseEvent *event) {
  // We only disabled the viewport in order to have scrollbar enabled.
  // Mouse events keep on coming in this case though, so we catch them manually.
  if (m_viewDisabled) return;
  auto idx = indexAt(event->pos());
  if (idx.isValid() && (idx.column() == TitleCol)) {
    userActionHandle(idx);
  }
  QTableView::mouseDoubleClickEvent(event);
}

void TaskTableView::setModel(QAbstractItemModel *model) {
  qDeleteAll(m_enableCheck);
  m_enableCheck.clear();
  QTableView::setModel(model);
  for (int i = 0; i < this->model()->rowCount(); i++) {
    auto statusIndex = this->model()->index(i, StatusCol);
    auto task = m_taskManager->task(statusIndex.data(TaskId).toUInt());
    // Table view can't play gif animations automatically, so we set QLabel,
    // which can.
    QWidget *w = new QWidget;
    w->setLayout(new QVBoxLayout);
    w->layout()->setContentsMargins(0, 0, 0, 0);
    w->layout()->setAlignment(Qt::AlignCenter);
    auto check = new QCheckBox;
    m_enableCheck.insert(statusIndex, check);
    check->setChecked(task->isEnable());
    auto titleIndex = this->model()->index(i, TitleCol);
    auto fmaxIndex = this->model()->index(i, FMaxCol);
    connect(check, &QCheckBox::stateChanged, this,
            [this, task, titleIndex, fmaxIndex](int state) {
              task->setEnable(state == Qt::Checked);
              update(titleIndex);
              update(fmaxIndex);
            });
    w->layout()->addWidget(check);
    w->setMinimumWidth(0);
    setIndexWidget(statusIndex, w);
    auto label = new QLabel{this};
    label->setMouseTracking(true);
    setIndexWidget(titleIndex, label);
  }
}

void TaskTableView::setViewDisabled(bool disabled) {
  m_viewDisabled = disabled;
  viewport()->setEnabled(!m_viewDisabled);
}

void TaskTableView::updateEnableColumn() {
  if (model()) {
    auto indexFrom = model()->index(0, StatusCol);
    auto indexTo = model()->index(model()->rowCount() - 1, StatusCol);
    dataChanged(indexFrom, indexTo, {TaskEnabledRole});
  }
}

void TaskTableView::updateLastColumn() {
  int maxWidth{0};
  for (int i = 0; i < model()->rowCount(); i++) {
    auto data = model()->data(model()->index(i, FMaxCol), Qt::DisplayRole);
    const QFontMetrics fm{font()};
    auto rect = fm.boundingRect(data.toString());
    maxWidth = qMax(maxWidth, rect.width() + 20);
  }
  if (maxWidth != 0) setColumnWidth(FMaxCol, maxWidth);
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
      QAction *start = new QAction("Run " + task->title(), this);
      start->setIcon(task->icon());
      connect(start, &QAction::triggered, this,
              [this, index]() { userActionHandle(index); });
      menu->addAction(start);
      menu->addSeparator();
      if (task->cleanTask() != nullptr) {
        QAction *clean = new QAction("Clean", this);
        connect(clean, &QAction::triggered, this,
                [this, index]() { userActionCleanHandle(index); });
        menu->addAction(clean);
      }
      auto subTask = task->subTask();
      if (auto it = std::find_if(subTask.cbegin(), subTask.cend(),
                                 [](Task *t) {
                                   return t->type() == TaskType::Settings ||
                                          t->type() == TaskType::Button;
                                 });
          it != subTask.cend()) {
        QAction *action = new QAction((*it)->title(), this);
        if ((*it)->type() == TaskType::Settings)
          connect(action, &QAction::triggered, this,
                  [this, it](bool) { TaskDialogRequestedHandler(*it); });
        else
          connect(action, &QAction::triggered, this,
                  [this, it]() { m_taskManager->startTask(*it); });
        if (m_taskManager->taskId((*it)) == PLACE_AND_ROUTE_VIEW) {
          action->setEnabled(m_taskManager->isEnablePnRView());
        }
        menu->addAction(action);
        menu->addSeparator();
      }
      if (TaskManager::isSimulation(task))
        addTaskViewWaveformAction(menu, task);

      addTaskLogAction(menu, task);
    }
    menu->popup(viewport()->mapToGlobal(pos));
  }
}

void TaskTableView::userActionHandle(const QModelIndex &index) {
  model()->setData(index, QVariant(), UserActionRole);
}

void TaskTableView::userActionCleanHandle(const QModelIndex &index) {
  model()->setData(index, QVariant(), UserActionCleanRole);
}

void TaskTableView::chooseFile(const QString &dir) {
  auto option{QFileDialog::DontUseNativeDialog};
  const QString fileName = QFileDialog::getOpenFileName(
      this, tr("Select File"), dir, "Waveform Files (*.fst *.vcd)", nullptr,
      option);
  if (!fileName.isEmpty()) emit ViewWaveform(fileName);
}

void TaskTableView::dataChanged(const QModelIndex &topLeft,
                                const QModelIndex &bottomRight,
                                const QVector<int> &roles) {
  if (roles.contains(TaskEnabledRole)) {
    for (int row = topLeft.row(); row < bottomRight.row(); row++) {
      auto index = model()->index(row, StatusCol);
      auto checked = model()->data(index, TaskEnabledRole).toBool();
      if (auto checkBox = m_enableCheck.value(index, nullptr); checkBox) {
        checkBox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
      }
    }
  }
  QTableView::dataChanged(topLeft, bottomRight, roles);
}

void TaskTableView::TaskDialogRequestedHandler(Task *task) {
  auto path = Settings::getUserSettingsPath(task->settingsKey().type);
  emit TaskDialogRequested(task->settingsKey().key, path);
}

QRect TaskTableView::contextArea(const QModelIndex &index) const {
  auto r = visualRect(index);
  int h = rowHeight(index.row());
  r.setTopLeft(r.topRight() - QPoint{20, 0});
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

#ifdef USE_IPA
  if (taskId == TIMING_SIGN_OFF) {
    QAction *interactivePathAnalysisAction =
        new QAction(tr("View Interactive Path Analysis"), this);
    connect(interactivePathAnalysisAction, &QAction::triggered, this,
            [this]() { emit ViewInteractivePathAnalysisRequested(); });
    interactivePathAnalysisAction->setEnabled(logExists);
    menu->addAction(interactivePathAnalysisAction);
  }
#endif  // USE_IPA
}

void TaskTableView::addTaskViewWaveformAction(QMenu *menu, Task *task) {
  QMenu *view = new QMenu("View waveform");
  menu->addMenu(view);

  auto compiler = m_taskManager->GetCompiler();
  auto simType = static_cast<Simulator::SimulationType>(task->cusomData().data);
  std::filesystem::path filePath =
      compiler->FilePath(Compiler::ToCompilerAction(simType));

  auto files = FileUtils::FindFilesByExtension(filePath, ".fst");
  files += FileUtils::FindFilesByExtension(filePath, ".vcd");

  for (const auto &file : files) {
    QAction *fileAction =
        new QAction{QString::fromStdString(file.filename().string())};
    connect(fileAction, &QAction::triggered, this, [this, file]() {
      emit ViewWaveform(QString::fromStdString(file.string()));
    });
    view->addAction(fileAction);
  }

  QAction *chooseFile = new QAction{"Choose file..."};
  connect(chooseFile, &QAction::triggered, this, [this, filePath, compiler]() {
    if (FileUtils::FileExists(filePath))
      this->chooseFile(QString::fromStdString(filePath.string()));
    else {
      if (compiler->ProjManager() && compiler->ProjManager()->HasDesign())
        this->chooseFile(compiler->ProjManager()->getProjectPath());
      else
        this->chooseFile(QDir::homePath());
    }
  });
  view->addAction(chooseFile);
}

TaskTableView::TasksDelegate::TasksDelegate(TaskTableView &view,
                                            QObject *parent)
    : QStyledItemDelegate(parent),
      m_view{view},
      m_inProgressMovie{new QMovie(LOADING_GIF, {}, &view)} {
  m_inProgressMovie->setScaledSize({15, 15});
}

void TaskTableView::TasksDelegate::paint(QPainter *painter,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const {
  if (index.column() == TitleCol) {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const QWidget *widget = option.widget;
    const int offset{5};
    QStyle *style = widget ? widget->style() : QApplication::style();
    opt.rect.setTopRight(opt.rect.topLeft() + QPoint(offset, 0));
    opt.backgroundBrush = Qt::red;
    QPixmap pixmap{opt.rect.size()};
    auto status = static_cast<TaskStatus>(index.data(StatusRole).toInt());
    if (status == TaskStatus::Fail)
      pixmap.fill(QColor{246, 126, 126});
    else if (status == TaskStatus::Success)
      pixmap.fill(QColor{150, 230, 135});
    else
      pixmap.fill(Qt::white);
    style->drawItemPixmap(painter, opt.rect, Qt::AlignCenter, pixmap);
    opt.rect = option.rect;
    opt.rect.setTopLeft(opt.rect.topLeft() + QPoint(offset, 0));
    opt.backgroundBrush = option.backgroundBrush;
    QStyledItemDelegate::paint(painter, opt, index);
    static const QImage image{":/images/three-dots.png"};
    static const QImage image_hov{":/images/three-dots_hovered.png"};
    bool hovered = (option.state & QStyle::StateFlag::State_MouseOver) != 0;
    const bool enabled = (option.state & QStyle::StateFlag::State_Enabled) != 0;
    QPoint globalCursorPos = QCursor::pos();
    QPoint viewportPos = m_view.viewport()->mapFromGlobal(globalCursorPos);

    auto dotRect = m_view.contextArea(index);
    hovered = (dotRect.contains(viewportPos) && hovered);
    style->drawItemPixmap(
        painter, dotRect, Qt::AlignCenter,
        QPixmap::fromImage((hovered && enabled) ? image_hov : image));

    // running icon
    auto label = qobject_cast<QLabel *>(m_view.indexWidget(index));
    if (!label) return;
    // QTableView can't paint animations. Do it manually via QLabel.
    if (status == TaskStatus::InProgress) {
      label->setMovie(m_inProgressMovie);
      label->resize(m_inProgressMovie->frameRect().size());
      // Place the animation to cells left side, similar to other decorations
      auto pos = m_view.visualRect(index).center();
      pos.ry() -= (label->rect().height() / 2);
      pos.rx() = m_view.visualRect(index).left() + offset +
                 opt.decorationSize.width() +
                 option.fontMetrics.boundingRect(opt.text).size().width() +
                 m_inProgressMovie->frameRect().width();
      label->move(pos);
      m_inProgressMovie->start();
      return;
    } else {
      // Reset the movie when task is no longer in progress
      label->setMovie(nullptr);
    }
    return;
  }
  QStyledItemDelegate::paint(painter, option, index);
}

QSize TaskTableView::TasksDelegate::sizeHint(const QStyleOptionViewItem &option,
                                             const QModelIndex &index) const {
  if (index.column() == TitleCol) {
    auto size = QStyledItemDelegate::sizeHint(option, index);
    size.setWidth(std::max(size.width(), 170));
    return size;
  }
  return {};
}

}  // namespace FOEDAG
