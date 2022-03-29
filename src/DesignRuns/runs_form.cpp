#include "runs_form.h"

#include <QMenu>
#include <QTextStream>
#include <QVBoxLayout>

#include "Compiler/Compiler.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "create_runs_dialog.h"

using namespace FOEDAG;

RunsForm::RunsForm(std::ostream &out, QWidget *parent)
    : QWidget(parent), m_out(out) {
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

  UpdateDesignRunsTree();

  connect(m_treeRuns, SIGNAL(itemPressed(QTreeWidgetItem *, int)), this,
          SLOT(SlotItempressed(QTreeWidgetItem *, int)));
}

void RunsForm::InitRunsForm(const QString &strFile) {
  m_projManager->StartProject(strFile);

  UpdateDesignRunsTree();
}

void RunsForm::setCompiler(TclInterpreter *interp,
                           TclInterpreterHandler *tclInterpreterHandler) {
  m_interp = interp;
  m_tclInterpreterHandler = tclInterpreterHandler;
}

void RunsForm::SlotItempressed(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  if (qApp->mouseButtons() == Qt::RightButton) {
    QMenu *menu = new QMenu(m_treeRuns);
    menu->addAction(m_actDelete);
    menu->addAction(m_actMakeActive);
    menu->addAction(m_actLaunchRuns);
    menu->addAction(m_actResetRuns);
    menu->addAction(m_actCreateSynthRuns);
    menu->addAction(m_actCreateImpleRuns);
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

void RunsForm::SlotDelete() {
  if (m_treeRuns == nullptr) {
    return;
  }
  QTreeWidgetItem *item = m_treeRuns->currentItem();
  if (item == nullptr) {
    return;
  }
  QString strRunName = item->text(0);

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

  QString strRunName = item->text(0);

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
  QString strRunName = item->text(0);
  if (strRunName.contains(RUNS_TREE_ACTIVE)) {
    m_interp->evalCmd("synth");

  } else {
    Compiler *compiler = new Compiler(m_interp, m_out, m_tclInterpreterHandler);
    std::string name = strRunName.toStdString();
    Design *design = new Design(name);
    compiler->SetDesign(design);
    WorkerThread *wthread =
        new WorkerThread("synth_th", Compiler::Action::Synthesis, compiler);
    wthread->start();
  }
  item->setIcon(0, QIcon(":/loading.png"));
  item->setText(3, tr("Running..."));
}

void RunsForm::SlotReSetRuns() {}

void RunsForm::SlotCreateSynthRuns() { CreateRuns(RT_SYNTH); }

void RunsForm::SlotCreateImpleRuns() { CreateRuns(RT_IMPLE); }

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

  m_actCreateSynthRuns = new QAction(tr("Create Synthesis"), m_treeRuns);
  connect(m_actCreateSynthRuns, SIGNAL(triggered()), this,
          SLOT(SlotCreateSynthRuns()));

  m_actCreateImpleRuns = new QAction(tr("Create Implement"), m_treeRuns);
  connect(m_actCreateImpleRuns, SIGNAL(triggered()), this,
          SLOT(SlotCreateImpleRuns()));
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
          << "Elapsed"
          << "LUT"
          << "FF"
          << "BRAM"
          << "URAM"
          << "DSP"
          << "CLBs";
  m_treeRuns->setHeaderLabels(strList);

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

      itemImple->setText(3, RUNS_TREE_STATUS);
    }
  }
  m_treeRuns->expandAll();
}

void RunsForm::CreateRuns(int type) {
  CreateRunsDialog *createRunsDlg = new CreateRunsDialog(this);
  createRunsDlg->InitDialog(type);
  if (createRunsDlg->exec()) {
    QList<rundata> listRun = createRunsDlg->getRunDataList();
    if (listRun.size()) {
      foreach (auto rd, listRun) {
        if (rd.m_iRunType == RT_SYNTH) {
          m_projManager->setSynthRun(rd.m_runName);
          QList<QPair<QString, QString>> listParam;
          QPair<QString, QString> pair;
          pair.first = PROJECT_PART_DEVICE;
          pair.second = rd.m_device;
          listParam.append(pair);
          m_projManager->setSynthesisOption(listParam);
        } else {
          m_projManager->setImpleRun(rd.m_runName);
          m_projManager->setRunSynthRun(rd.m_synthName);
        }
        m_projManager->setRunSrcSet(rd.m_srcSet);
        m_projManager->setRunConstrSet(rd.m_constrSet);
      }
    }
    UpdateDesignRunsTree();
    m_projManager->FinishedProject();
  }
  createRunsDlg->close();
}
