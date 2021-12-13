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
  connect(m_actMakeActive, SIGNAL(triggered()), this, SLOT(SlotMakeActive));

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
          << "sources"
          << "Constraints"
          << "status"
          << "Part"
          << "Start"
          << "Elapsed";
  m_treeRuns->setHeaderLabels(strList);

  QStringList listSynthRunNames = m_projManager->getSynthRunsName();
  foreach (auto strSynthName, listSynthRunNames) {
    QTreeWidgetItem *itemSynth = new QTreeWidgetItem(m_treeRuns);

    QStringList listSynthProperties =
        m_projManager->getRunsProperties(strSynthName);
    QString strImpleName;
    QString strState;
    for (int i = 0; i < listSynthProperties.count(); ++i) {
      itemSynth->setText(i + 1, listSynthProperties.at(i));
      if (4 == i) {
        strState = listSynthProperties.at(i);
      }
      if (5 == i) {
        strImpleName = listSynthProperties.at(i);
      }
    }
    if (strState == RUN_STATE_CURRENT) {
      itemSynth->setText(0, strSynthName + "(Active)");
    } else {
      itemSynth->setText(0, strSynthName);
    }

    QTreeWidgetItem *itemImple = new QTreeWidgetItem(itemSynth);
    itemImple->setText(0, strImpleName);
    QStringList listImpleProperties =
        m_projManager->getRunsProperties(strSynthName);
    for (int i = 0; i < listImpleProperties.count(); ++i) {
      itemImple->setText(i + 1, listImpleProperties.at(i));
    }
  }
  m_treeRuns->expandAll();
}
