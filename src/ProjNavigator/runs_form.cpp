#include "runs_form.h"

#include <QVBoxLayout>

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

void RunsForm::SlotItempressed(QTreeWidgetItem *item, int column) {}

void RunsForm::SlotDelete() {}

void RunsForm::SlotMakeActive() {}

void RunsForm::SlotLaunchRuns() {}

void RunsForm::SlotReSetRuns() {}

void RunsForm::SlotCreateRuns() {}

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
      itemSynth->setText(0, strSynthName + "(Active)");
    } else {
      itemSynth->setText(0, strSynthName);
    }
    itemSynth->setText(3, "Not Started");

    // Start creating the implementation view
    QString strImpleName = m_projManager->SynthUsedByImple(strSynthName);
    if ("" != strImpleName) {
      QTreeWidgetItem *itemImple = new QTreeWidgetItem(itemSynth);
      itemImple->setText(0, strImpleName);
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
      itemImple->setText(3, "Not Started");
    }
  }
  m_treeRuns->expandAll();
}
