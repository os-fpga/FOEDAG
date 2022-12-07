#include "MessagesTabWidget.h"

#include <QGridLayout>
#include <QTreeWidget>

#include "Compiler/TaskManager.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "TextEditor/text_editor_form.h"

namespace FOEDAG {

MessagesTabWidget::MessagesTabWidget(const TaskManager &taskManager)
    : m_taskManager{taskManager} {
  auto layout = new QGridLayout();
  auto treeWidget = new QTreeWidget();

  layout->addWidget(treeWidget);
  layout->setMargin(0);
  setLayout(layout);

  treeWidget->setColumnCount(1);
  treeWidget->setHeaderLabel(tr("Task Messages"));

  auto &reports = m_taskManager.getReportManagerRegistry();
  for (auto task : m_taskManager.tasks()) {
    auto taskId = m_taskManager.taskId(task);

    if (auto reportManager = reports.getReportManager(taskId)) {
      auto filePath = task->logFileReadPath();
      filePath.replace(PROJECT_OSRCDIR, Project::Instance()->projectPath());
      auto itemName = QString("%1 (%2)").arg(task->title(), filePath);
      auto taskItem = new QTreeWidgetItem({itemName});
      taskItem->setData(0, Qt::UserRole, filePath);

      const auto &msgs = reportManager->getMessages();
      for (auto it = msgs.cbegin(); it != msgs.cend(); it++) {
        auto msgItem = createTaskMessageItem(it.value());

        // task id is needed to locate corresponding report manager in case of
        // report request
        msgItem->setData(0, Qt::UserRole, QVariant(it.key()));
        taskItem->addChild(msgItem);
      }
      treeWidget->addTopLevelItem(taskItem);
    }
  }
  treeWidget->expandAll();
  connect(treeWidget, &QTreeWidget::itemDoubleClicked, this,
          &MessagesTabWidget::onMessageClicked);
}

QTreeWidgetItem *MessagesTabWidget::createTaskMessageItem(
    const TaskMessage &msg) const {
  auto msgItem = new QTreeWidgetItem({msg.m_message});
  msgItem->setData(0, Qt::UserRole, QVariant(msg.m_lineNr));

  switch (msg.m_severity) {
    case TaskMessage::MessageSeverity::INFO_MESSAGE:
      msgItem->setIcon(0, QIcon(":/img/info.png"));
      break;
    case TaskMessage::MessageSeverity::ERROR_MESSAGE:
      msgItem->setIcon(0, QIcon(":/images/error.png"));
      break;
    case TaskMessage::MessageSeverity::WARNING_MESSAGE:
      msgItem->setIcon(0, QIcon(":/img/warn.png"));
      break;
    default:
      break;
  }

  for (const auto &childMsg : msg.m_childMessages)
    msgItem->addChild(createTaskMessageItem(childMsg));

  return msgItem;
}

void MessagesTabWidget::onMessageClicked(const QTreeWidgetItem *item, int col) {
  auto parentItem = item->parent();
  if (!parentItem) return;  // only leaf items represent log messages

  auto filePath = parentItem->data(0, Qt::UserRole).toString();

  auto line = item->data(col, Qt::UserRole).toInt();
  TextEditorForm::Instance()->OpenFileWithSelection(QString(filePath), line,
                                                    line);
}

}  // namespace FOEDAG
