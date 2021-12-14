#include "runs_form.h"

#include <QMenu>
#include <QVBoxLayout>

#include "create_runs_dialog.h"

using namespace FOEDAG;

RunsForm::RunsForm(QString strProPath, QWidget *parent) : QWidget(parent) {
  m_treeRuns = new QTreeWidget(this);
  m_treeRuns->setSelectionMode(
      QAbstractItemView::SelectionMode::SingleSelection);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addWidget(m_treeRuns);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(0);
  setLayout(vbox);

  CreateActions();

  m_projManager = new ProjectManager(this);
  m_projManager->StartProject(strProPath + PROJECT_FILE_FORMAT);

  UpdateDesignRunsTree();

  connect(m_treeRuns, SIGNAL(itemPressed(QTreeWidgetItem *, int)), this,
          SLOT(SlotItempressed(QTreeWidgetItem *, int)));
}

void RunsForm::SlotItempressed(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  if (qApp->mouseButtons() == Qt::RightButton) {
    QMenu *menu = new QMenu(m_treeRuns);
    menu->addAction(m_actDelete);
    menu->addAction(m_actMakeActive);
    menu->addAction(m_actLaunchRuns);
    menu->addAction(m_actResetRuns);
    menu->addAction(m_actCreateRuns);
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
      m_actResetRuns->setEnabled(false);
    } else {
      m_actResetRuns->setEnabled(true);
    }
    QPoint p = QCursor::pos();
    menu->exec(QPoint(p.rx(), p.ry() + 3));
  }
}

void RunsForm::SlotDelete() {}

void RunsForm::SlotMakeActive() {}

void RunsForm::SlotLaunchRuns() {}

void RunsForm::SlotReSetRuns() {}

void RunsForm::SlotCreateRuns() {
  CreateRunsDialog dlg;
  dlg.InitDialog(RT_SYNTH);
  dlg.exec();
}

void RunsForm::CreateActions() {
  m_actDelete = new QAction(tr("Delete"), m_treeRuns);
  connect(m_actDelete, SIGNAL(triggered()), this, SLOT(SlotDelete()));

  m_actMakeActive = new QAction(tr("Make Active"), m_treeRuns);
  connect(m_actMakeActive, SIGNAL(triggered()), this, SLOT(SlotMakeActive()));

  m_actLaunchRuns = new QAction(tr("Launch Runs"), m_treeRuns);
  connect(m_actLaunchRuns, SIGNAL(triggered()), this, SLOT(SlotLaunchRuns()));

  m_actResetRuns = new QAction(tr("Reset Runs"), m_treeRuns);
  connect(m_actResetRuns, SIGNAL(triggered()), this, SLOT(SlotReSetRuns()));

  m_actCreateRuns = new QAction(tr("Create Runs"), m_treeRuns);
  connect(m_actCreateRuns, SIGNAL(triggered()), this, SLOT(SlotCreateRuns()));
}

void RunsForm::UpdateDesignRunsTree() {
  if (nullptr == m_projManager) {
    return;
  }

  m_treeRuns->clear();
  QStringList strList;
  strList << "Name"
          << "Sources"
          << "Constraints"
          << "Status"
          << "Device"
          << "Start"
          << "Elapsed";
  m_treeRuns->setHeaderLabels(strList);

  QStringList listSynthRunNames = m_projManager->getSynthRunsNames();
  foreach (auto strSynthName, listSynthRunNames) {
    QTreeWidgetItem *itemSynth = new QTreeWidgetItem(m_treeRuns);

    QList<QPair<QString, QString>> listSynthProperties =
        m_projManager->getRunsProperties(strSynthName);
    QString strState;
    for (int i = 0; i < listSynthProperties.count(); ++i) {
      QPair<QString, QString> pair = listSynthProperties.at(i);
      if (pair.first == PROJECT_RUN_SRCSET) {
        itemSynth->setText(1, pair.second);
      } else if (pair.first == PROJECT_RUN_CONSTRSSET) {
        itemSynth->setText(2, pair.second);
      } else if (pair.first == PROJECT_RUN_STATE) {
        strState = pair.second;
      } else if (pair.first == PROJECT_PART_DEVICE) {
        itemSynth->setText(4, pair.second);
      }
    }
    if (strState == RUN_STATE_CURRENT) {
      itemSynth->setText(0, strSynthName + RUNS_TREE_ACTIVE);
    } else {
      itemSynth->setText(0, strSynthName);
    }
    itemSynth->setText(3, RUNS_TREE_STATUS);

    // Start creating the implementation view
    QString strImpleName = m_projManager->SynthUsedByImple(strSynthName);
    if ("" != strImpleName) {
      QTreeWidgetItem *itemImple = new QTreeWidgetItem(itemSynth);
      if (strState == RUN_STATE_CURRENT) {
        itemImple->setText(0, strImpleName + RUNS_TREE_ACTIVE);
      } else {
        itemImple->setText(0, strImpleName);
      }

      QList<QPair<QString, QString>> listImpleProperties =
          m_projManager->getRunsProperties(strImpleName);
      for (int i = 0; i < listImpleProperties.count(); ++i) {
        QPair<QString, QString> pair = listSynthProperties.at(i);
        if (pair.first == PROJECT_RUN_SRCSET) {
          itemImple->setText(1, pair.second);
        } else if (pair.first == PROJECT_RUN_CONSTRSSET) {
          itemImple->setText(2, pair.second);
        } else if (pair.first == PROJECT_PART_DEVICE) {
          itemImple->setText(4, pair.second);
        }
      }
      itemImple->setText(3, RUNS_TREE_STATUS);
    }
  }
  m_treeRuns->expandAll();
}
