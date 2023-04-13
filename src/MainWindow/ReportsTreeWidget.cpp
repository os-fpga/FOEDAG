#include "ReportsTreeWidget.h"

#include <QGridLayout>
#include <QTreeWidget>

#include "Compiler/TaskManager.h"
#include "Main/Tasks.h"
#include "TextEditor/text_editor_form.h"

namespace FOEDAG {

ReportsTreeWidget::ReportsTreeWidget(Compiler *compiler,
                                     const TaskManager &taskManager)
    : m_taskManager{taskManager}, m_compiler{compiler} {
  auto layout = new QGridLayout();
  auto treeWidget = new QTreeWidget();

  layout->addWidget(treeWidget);
  layout->setMargin(0);
  setLayout(layout);

  const auto reportManagers = m_taskManager.getReportManagerRegistry();
  treeWidget->setColumnCount(1);
  treeWidget->setHeaderLabel(tr("Task Reports"));

  auto rowIndex = 0;
  for (auto task : m_taskManager.tasks()) {
    auto taskId = m_taskManager.taskId(task);
    auto reportManager = reportManagers.getReportManager(taskId);
    if (reportManager && !reportManager->getAvailableReportIds().isEmpty()) {
      auto taskItem = new QTreeWidgetItem({task->title()});
      for (auto reportId : reportManager->getAvailableReportIds()) {
        auto reportItem = new QTreeWidgetItem({reportId});
        // task id is needed to locate corresponding report manager in case of
        // report request
        reportItem->setData(0, Qt::UserRole, taskId);
        taskItem->addChild(reportItem);
      }
      treeWidget->insertTopLevelItem(rowIndex++, taskItem);
    }
  }
  treeWidget->expandAll();

  connect(treeWidget, &QTreeWidget::itemDoubleClicked, this,
          &ReportsTreeWidget::onReportRequested);
}

void ReportsTreeWidget::onReportRequested(const QTreeWidgetItem *item,
                                          int col) {
  if (item->childCount()) return;  // only leaf items represent reports
  auto reportId = item->text(col);
  auto tabWidget = TextEditorForm::Instance()->GetTabWidget();
  for (auto i = 0; i < tabWidget->count(); ++i) {
    if (tabWidget->tabText(i) == reportId) {
      tabWidget->setCurrentIndex(i);
      return;
    }
  }
  auto taskId = item->data(col, Qt::UserRole).toUInt();
  auto reportsManager =
      m_taskManager.getReportManagerRegistry().getReportManager(taskId);
  if (!reportsManager) return;
  FOEDAG::handleViewReportRequested(m_compiler, m_taskManager.task(taskId),
                                    reportId, *reportsManager);
}

}  // namespace FOEDAG
