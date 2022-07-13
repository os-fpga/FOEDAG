#include "runs_form.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QProcess>
#include <QTextStream>
#include <QTime>
#include <QVBoxLayout>

#include "MainWindow/Session.h"
#include "create_runs_dialog.h"

using namespace FOEDAG;

RunsForm::RunsForm(QWidget *parent) : QWidget(parent) {
  m_treeRuns = new QTreeWidget(this);
  m_treeRuns->setObjectName("m_treeRuns");
  m_treeRuns->setSelectionMode(
      QAbstractItemView::SelectionMode::SingleSelection);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addWidget(m_treeRuns);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(0);
  setLayout(vbox);

  CreateActions();

  m_projManager = new ProjectManager(this);

  UpdateDesignRunsTree();

  connect(m_treeRuns, SIGNAL(itemPressed(QTreeWidgetItem *, int)), this,
          SLOT(SlotItempressed(QTreeWidgetItem *, int)));
}

void RunsForm::InitRunsForm() { UpdateDesignRunsTree(); }

void RunsForm::RegisterCommands(Session *session) { m_session = session; }

ProjectManager *RunsForm::projectManager() { return m_projManager; }

void RunsForm::SlotItempressed(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  if (qApp->mouseButtons() == Qt::RightButton) {
    QMenu *menu = new QMenu(m_treeRuns);
    menu->addAction(m_actDelete);
    menu->addAction(m_actMakeActive);
    menu->addAction(m_actLaunchRuns);
    menu->addAction(m_actResetRuns);
    menu->addAction(m_actCreateRuns);
    menu->addAction(m_actOpenRunDir);
    QString strName = item->text(0);
    if (strName.contains(RUNS_TREE_ACTIVE)) {
      m_actDelete->setEnabled(false);
      m_actMakeActive->setEnabled(false);
    } else {
      m_actDelete->setEnabled(true);
      m_actMakeActive->setEnabled(true);
    }
    QString strStatus = item->text(3);
    if (strStatus == RUNS_TREE_STATUS) {
      m_actResetRuns->setEnabled(true);
    } else {
      m_actResetRuns->setEnabled(true);
    }
    QPoint p = QCursor::pos();
    menu->exec(QPoint(p.rx(), p.ry() + 3));
    menu->deleteLater();
  }
}

void RunsForm::SlotRefreshRuns() { UpdateDesignRunsTree(); }

void RunsForm::SlotDelete() {
  if (m_treeRuns == nullptr) {
    return;
  }
  QTreeWidgetItem *item = m_treeRuns->currentItem();
  if (item == nullptr) {
    return;
  }
  QString strRunName = (item->data(0, SaveDataRole)).toString();

  int ret = m_projManager->deleteRun(strRunName);
  if (0 == ret) {
    UpdateDesignRunsTree();
    m_projManager->FinishedProject();
  }
}

void RunsForm::SlotMakeActive() {
  if (m_treeRuns == nullptr) {
    return;
  }
  QTreeWidgetItem *item = m_treeRuns->currentItem();
  if (item == nullptr) {
    return;
  }

  QString strRunName = (item->data(0, SaveDataRole)).toString();

  int ret = m_projManager->setRunActive(strRunName);
  if (0 == ret) {
    UpdateDesignRunsTree();
    m_projManager->FinishedProject();
  }
}

void RunsForm::SlotLaunchRuns() {
  if (m_treeRuns == nullptr) {
    return;
  }
  QTreeWidgetItem *item = m_treeRuns->currentItem();
  if (item == nullptr) {
    return;
  }
  QString strRunName = (item->data(0, SaveDataRole)).toString();
  // To do

  item->setIcon(0, QIcon(":/loading.png"));
  item->setText(3, tr("Running..."));
}

void RunsForm::SlotReSetRuns() {
  if (m_treeRuns == nullptr) {
    return;
  }
  QTreeWidgetItem *item = m_treeRuns->currentItem();
  if (item == nullptr) {
    return;
  }
  QString strRunName = (item->data(0, SaveDataRole)).toString();

  QString strProPath = m_projManager->getProjectPath();
  QString strProName = m_projManager->getProjectName();
  QString strImplePath = strProPath + "/" + strProName + ".runs/" + strRunName;
  RemoveFolderContent(strImplePath);
}

void RunsForm::SlotCreateRuns() {
  CreateRunsDialog *createRunsDlg = new CreateRunsDialog(this);
  connect(createRunsDlg, SIGNAL(RefreshRuns()), this, SLOT(SlotRefreshRuns()));
  createRunsDlg->exec();
  createRunsDlg->close();
  disconnect(createRunsDlg, SIGNAL(RefreshRuns()), this,
             SLOT(SlotRefreshRuns()));
}

void RunsForm::SlotOpenRunDir() {
  if (m_treeRuns == nullptr) {
    return;
  }
  QTreeWidgetItem *item = m_treeRuns->currentItem();
  if (item == nullptr) {
    return;
  }
  QString strRunName = (item->data(0, SaveDataRole)).toString();
  QString strProPath = m_projManager->getProjectPath();
  QString strProName = m_projManager->getProjectName();
  QString strPath = strProPath + "/" + strProName + ".runs/" + strRunName + "/";
  // QProcess::startDetached("nautilus " + strPath);
}

void RunsForm::CreateActions() {
  m_actDelete = new QAction(tr("Delete"), m_treeRuns);
  connect(m_actDelete, SIGNAL(triggered()), this, SLOT(SlotDelete()));

  m_actMakeActive = new QAction(tr("Make Active"), m_treeRuns);
  connect(m_actMakeActive, SIGNAL(triggered()), this, SLOT(SlotMakeActive()));

  m_actLaunchRuns = new QAction(tr("Launch Runs"), m_treeRuns);
  m_actLaunchRuns->setIcon(QIcon(":/images/play.png"));
  connect(m_actLaunchRuns, SIGNAL(triggered()), this, SLOT(SlotLaunchRuns()));

  m_actResetRuns = new QAction(tr("Reset Runs"), m_treeRuns);
  connect(m_actResetRuns, SIGNAL(triggered()), this, SLOT(SlotReSetRuns()));

  m_actCreateRuns = new QAction(tr("Create Runs"), m_treeRuns);
  m_actCreateRuns->setIcon(QIcon(":/images/add.png"));
  connect(m_actCreateRuns, SIGNAL(triggered()), this, SLOT(SlotCreateRuns()));

  m_actOpenRunDir = new QAction(tr("Open Directory"), m_treeRuns);
  connect(m_actOpenRunDir, SIGNAL(triggered()), this, SLOT(SlotOpenRunDir()));
}

void RunsForm::UpdateDesignRunsTree() {
  if (nullptr == m_projManager) {
    return;
  }

  m_treeRuns->clear();
  QStringList strList;
  strList << "Design Name"
          << "Sources set"
          << "Constraints set"
          << "Status"
          << "Device"
          << "Start"
          << "Elapsed"
          << "LUT"
          << "FF"
          << "BRAM"
          << "URAM"
          << "DSP"
          << "CLBs";
  m_treeRuns->setHeaderLabels(strList);
  m_treeRuns->setColumnWidth(0, 200);

  // gets all run names of type synth
  QStringList listSynthRunNames = m_projManager->getSynthRunsNames();
  foreach (auto strSynthName, listSynthRunNames) {
    QTreeWidgetItem *itemSynth = new QTreeWidgetItem(m_treeRuns);

    QList<QPair<QString, QString>> listSynthProperties =
        m_projManager->getRunsProperties(strSynthName);
    QString strSynthState;
    QString strDevice;
    for (int i = 0; i < listSynthProperties.count(); ++i) {
      QPair<QString, QString> pair = listSynthProperties.at(i);
      if (pair.first == PROJECT_RUN_SRCSET) {
        itemSynth->setText(1, pair.second);
      } else if (pair.first == PROJECT_RUN_CONSTRSSET) {
        itemSynth->setText(2, pair.second);
      } else if (pair.first == PROJECT_RUN_STATE) {
        strSynthState = pair.second;
      } else if (pair.first == PROJECT_PART_DEVICE) {
        strDevice = pair.second;
      }
    }

    itemSynth->setText(4, strDevice);
    if (strSynthState == RUN_STATE_CURRENT) {
      itemSynth->setText(0, strSynthName + RUNS_TREE_ACTIVE);
    } else {
      itemSynth->setText(0, strSynthName);
    }
    itemSynth->setIcon(0, QIcon(":/images/play.png"));
    itemSynth->setData(0, SaveDataRole, strSynthName);

    itemSynth->setText(3, RUNS_TREE_STATUS);

    // Start creating the implementation view
    QStringList listImpleName = m_projManager->ImpleUsedSynth(strSynthName);
    foreach (auto strImpleName, listImpleName) {
      QTreeWidgetItem *itemImple = new QTreeWidgetItem(itemSynth);

      QList<QPair<QString, QString>> listImpleProperties =
          m_projManager->getRunsProperties(strImpleName);
      QString strImpleState;
      for (int i = 0; i < listImpleProperties.count(); ++i) {
        QPair<QString, QString> pair = listImpleProperties.at(i);
        if (pair.first == PROJECT_RUN_SRCSET) {
          itemImple->setText(1, pair.second);
        } else if (pair.first == PROJECT_RUN_CONSTRSSET) {
          itemImple->setText(2, pair.second);
        } else if (pair.first == PROJECT_RUN_STATE) {
          strImpleState = pair.second;
        }
      }
      itemImple->setText(4, strDevice);
      if (strImpleState == RUN_STATE_CURRENT) {
        itemImple->setText(0, strImpleName + RUNS_TREE_ACTIVE);
      } else {
        itemImple->setText(0, strImpleName);
      }
      itemImple->setIcon(0, QIcon(":/images/play.png"));
      itemSynth->setData(0, SaveDataRole, strSynthName);

      itemImple->setText(3, RUNS_TREE_STATUS);
    }
  }
  m_treeRuns->expandAll();
}

void RunsForm::RemoveFolderContent(const QString &folderDir) {
  QDir dir(folderDir);
  QFileInfoList fileList;
  QFileInfo curFile;
  if (!dir.exists()) {
    return;
  }
  fileList = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::Readable |
                                   QDir::Writable | QDir::Hidden |
                                   QDir::NoDotAndDotDot,
                               QDir::Name);
  while (fileList.size() > 0) {
    int infoNum = fileList.size();
    for (int i = infoNum - 1; i >= 0; i--) {
      curFile = fileList[i];
      if (curFile.isFile()) {
        QFile fileTemp(curFile.filePath());
        fileTemp.remove();
        fileList.removeAt(i);
      }
      if (curFile.isDir()) {
        QDir dirTemp(curFile.filePath());
        QFileInfoList fileList1 = dirTemp.entryInfoList(
            QDir::Dirs | QDir::Files | QDir::Readable | QDir::Writable |
                QDir::Hidden | QDir::NoDotAndDotDot,
            QDir::Name);
        if (fileList1.size() == 0) {
          dirTemp.rmdir(".");
          fileList.removeAt(i);
        } else {
          for (int j = 0; j < fileList1.size(); j++) {
            if (!(fileList.contains(fileList1[j])))
              fileList.append(fileList1[j]);
          }
        }
      }
    }
  }
  return;
}
