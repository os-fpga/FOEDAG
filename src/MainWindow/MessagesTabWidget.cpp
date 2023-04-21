#include "MessagesTabWidget.h"

#include <QGridLayout>
#include <QTreeWidget>

#include "Compiler/TaskManager.h"
#include "MessageItemParser.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "TextEditor/text_editor_form.h"
#include "Utils/FileUtils.h"
#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

namespace {
static constexpr auto FilePathRole = Qt::UserRole + 1;
static constexpr auto LineNumberRole = Qt::UserRole + 2;
static constexpr auto LineNumSrcFileRole = Qt::UserRole + 3;
static constexpr auto LevelRole = Qt::UserRole + 4;
}  // namespace

namespace FOEDAG {

MessagesTabWidget::MessagesTabWidget(const TaskManager &taskManager,
                                     const std::filesystem::path &dataPath)
    : m_taskManager{taskManager},
      m_parsers{new VerificParser{}, new TimingAnalysisParser{}} {
  auto layout = new QGridLayout();
  auto treeWidget = new QTreeWidget();

  layout->addWidget(treeWidget);
  layout->setMargin(0);
  setLayout(layout);

  treeWidget->setColumnCount(1);
  treeWidget->setHeaderLabel(tr("Task Messages"));

  auto &reports = m_taskManager.getReportManagerRegistry();
  const auto &tasks = m_taskManager.tasks();
  for (auto task : tasks) {
    auto taskId = m_taskManager.taskId(task);

    if (auto reportManager = reports.getReportManager(taskId)) {
      reportManager->setSuppressList(loadSuppressList(dataPath));
      auto filePath = task->logFileReadPath();
      filePath.replace(PROJECT_OSRCDIR, Project::Instance()->projectPath());

      const bool fileExists{FileUtils::FileExists(filePath.toStdString())};
      if (!fileExists) filePath = tr("log file not found");

      const QString hyperLink = fileExists ? createLink(filePath) : filePath;
      auto itemName = QString("%1 (%2)").arg(task->title(), hyperLink);
      auto taskItem = new QTreeWidgetItem({itemName});
      if (fileExists) m_convertToLabel.push_back(taskItem);

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

  for (const auto &item : qAsConst(m_convertToLabel)) {
    auto label = new QLabel{item->text(0)};
    label->setTextInteractionFlags(label->textInteractionFlags() |
                                   Qt::LinksAccessibleByMouse);
    connect(label, &QLabel::linkActivated, this, [item](const QString &link) {
      auto line = item->data(0, LineNumSrcFileRole).toInt();
      auto level = item->data(0, LevelRole).toInt();
      TextEditorForm::Instance()->OpenFileWithLine(link, line, level == Error);
    });
    treeWidget->setItemWidget(item, 0, label);
    item->setText(0, QString{});
  }
  treeWidget->expandAll();
  connect(treeWidget, &QTreeWidget::itemDoubleClicked, this,
          &MessagesTabWidget::onMessageClicked);
}

MessagesTabWidget::~MessagesTabWidget() { qDeleteAll(m_parsers); }

QTreeWidgetItem *MessagesTabWidget::createTaskMessageItem(
    const TaskMessage &msg, const QString &filePath) {
  auto msgItem = createItem(msg.m_message);
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

QTreeWidgetItem *MessagesTabWidget::createItem(const QString &message) {
  QTreeWidgetItem *item = new QTreeWidgetItem({message});
  for (const auto &parser : m_parsers) {
    auto [found, info] = parser->parse(message);
    if (found) {
      auto m = message;
      m = m.replace(info.fileName, createLink(info.fileName));
      item->setText(0, m);
      item->setData(0, LineNumSrcFileRole, info.line);
      item->setData(0, LevelRole, info.level);
      m_convertToLabel.append(item);
      break;
    }
  }
  return item;
}

QString MessagesTabWidget::createLink(const QString &str) {
  return QString{"<a href=\"%1\">%1</a>"}.arg(str);
}

QStringList MessagesTabWidget::loadSuppressList(
    const std::filesystem::path &dataPath) {
  auto fullPath = dataPath / "etc" / "settings" / "messages" / "suppress.json";
  QString fileName{QString::fromStdString(fullPath.string())};
  QFile file{fileName};
  if (file.exists()) {
    if (!file.open(QFile::ReadOnly)) return {};

    auto content = file.readAll();
    json jsonObject;
    try {
      jsonObject = json::parse(content.toStdString());
    } catch (...) {
      return {};
    }

    QStringList results;
    auto suppressList = jsonObject.at("suppress");
    for (const auto &s : suppressList) {
      results.push_back(QString::fromStdString(s));
    }
    return results;
  }
  return {};
}

void MessagesTabWidget::onMessageClicked(const QTreeWidgetItem *item, int col) {
  if (!item->parent()) return;  // top level items are tasks

  auto filePath = item->data(0, FilePathRole).toString();

  auto line = item->data(col, LineNumberRole).toInt();
  TextEditorForm::Instance()->OpenFileWithSelection(QString(filePath), line,
                                                    line);
}

}  // namespace FOEDAG
