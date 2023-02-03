#include "MessagesTabWidget.h"

#include <QGridLayout>
#include <QTreeWidget>

#include "Compiler/TaskManager.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "TextEditor/text_editor_form.h"
#include "Utils/FileUtils.h"

namespace {
static constexpr auto FilePathRole = Qt::UserRole + 1;
static constexpr auto LineNumberRole = Qt::UserRole + 2;
}  // namespace

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

      const bool fileExists{FileUtils::FileExists(filePath.toStdString())};
      if (!fileExists) filePath = tr("log file not found");
      auto itemName = QString("%1 (%2)").arg(task->title(), filePath);
      auto taskItem = new QTreeWidgetItem({itemName});

      if (fileExists) {
        const auto &msgs = reportManager->getMessages();
        for (auto it = msgs.cbegin(); it != msgs.cend(); it++) {
          auto msgItem = createTaskMessageItem(it.value(), filePath);
          taskItem->addChild(msgItem);
        }
      }
      treeWidget->addTopLevelItem(taskItem);
    }
  }
  treeWidget->expandAll();
  connect(treeWidget, &QTreeWidget::itemDoubleClicked, this,
          &MessagesTabWidget::onMessageClicked);
}

QTreeWidgetItem *MessagesTabWidget::createTaskMessageItem(
    const TaskMessage &msg, const QString &filePath) const {
  auto msgItem = new QTreeWidgetItem({msg.m_message});
  msgItem->setData(0, LineNumberRole, QVariant(msg.m_lineNr));
  msgItem->setData(0, FilePathRole, filePath);

  switch (msg.m_severity) {
    case MessageSeverity::INFO_MESSAGE:
      msgItem->setIcon(0, QIcon(":/img/info.png"));
      break;
    case MessageSeverity::ERROR_MESSAGE:
      msgItem->setIcon(0, QIcon(":/images/error.png"));
      break;
    case MessageSeverity::WARNING_MESSAGE:
      msgItem->setIcon(0, QIcon(":/img/warn.png"));
      break;
    default:
      break;
  }

  for (const auto &childMsg : msg.m_childMessages)
    msgItem->addChild(createTaskMessageItem(childMsg, filePath));

  return msgItem;
}

void MessagesTabWidget::onMessageClicked(const QTreeWidgetItem *item, int col) {
  if (!item->parent()) return;  // top level items are tasks

  auto filePath = item->data(0, FilePathRole).toString();

  auto line = item->data(col, LineNumberRole).toInt();
  TextEditorForm::Instance()->OpenFileWithSelection(QString(filePath), line,
                                                    line);
}

}  // namespace FOEDAG
