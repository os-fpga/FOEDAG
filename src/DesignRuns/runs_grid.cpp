#include "runs_grid.h"

#include <QMouseEvent>
#include <QVBoxLayout>

#include "runs_grid_delegate.h"

using namespace FOEDAG;

RunsGrid::RunsGrid(RunsType type, QWidget *parent) : QWidget(parent) {
  m_type = type;

  m_toolBar = new QToolBar(this);
  m_toolBar->setIconSize(QSize(32, 32));

  m_actAdd = new QAction(m_toolBar);
  // m_actAdd->setIcon( QIcon(""));
  m_actAdd->setText(tr("&Add"));
  m_toolBar->addAction(m_actAdd);
  m_toolBar->addSeparator();

  m_actDelete = new QAction(m_toolBar);
  // m_actDelete->setIcon( QIcon(""));
  m_actDelete->setText(tr("&Delete"));
  m_actDelete->setEnabled(false);
  m_toolBar->addAction(m_actDelete);

  m_tableViewRuns = new QTableView(this);

  // Set properties
  m_tableViewRuns->verticalHeader()->hide();
  // Color separation between lines
  m_tableViewRuns->setAlternatingRowColors(true);
  // Last column adaptive width
  // m_tableViewRuns->horizontalHeader()->setStretchLastSection(true);
  m_tableViewRuns->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);

  m_model = new QStandardItemModel(m_tableViewRuns);
  m_selectModel = new QItemSelectionModel(m_model, m_tableViewRuns);
  m_tableViewRuns->setModel(m_model);
  m_tableViewRuns->setSelectionModel(m_selectModel);
  connect(m_selectModel, &QItemSelectionModel::selectionChanged, this,
          &RunsGrid::SlotTableViewSelectionChanged);

  m_tableViewRuns->horizontalHeader()->setMinimumHeight(30);

  m_model->setHorizontalHeaderItem(0, new QStandardItem("Name"));
  m_model->setHorizontalHeaderItem(1, new QStandardItem("Sources Set"));
  m_model->setHorizontalHeaderItem(2, new QStandardItem("Constraints Set"));
  if (type == RT_SYNTH) {
    m_model->setHorizontalHeaderItem(3, new QStandardItem("Device"));
  } else if (type == RT_IMPLE) {
    m_model->setHorizontalHeaderItem(3, new QStandardItem("Synth Name"));
  }

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addWidget(m_toolBar);
  vbox->addWidget(m_tableViewRuns);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(1);
  setLayout(vbox);

  connect(m_actAdd, SIGNAL(triggered()), this, SLOT(SlotAddRuns()));
  connect(m_actDelete, SIGNAL(triggered()), this, SLOT(SlotDeleteRuns()));

  m_projManager = new ProjectManager(this);
  m_runId = CreateFirstRunId();
  m_strSrcSet = m_projManager->getDesignActiveFileSet();
  m_strConstrSet = m_projManager->getConstrActiveFileSet();
  m_strDevice = m_projManager->getActiveRunDevice();
  m_strSynthName = m_projManager->getActiveSynthRunName();

  QStringList listSrcSet = m_projManager->getDesignFileSets();
  m_tableViewRuns->setItemDelegateForColumn(
      1, new RunsGridDelegate(DT_COMBOX, listSrcSet, this));
  QStringList listConstrSet = m_projManager->getConstrFileSets();
  m_tableViewRuns->setItemDelegateForColumn(
      2, new RunsGridDelegate(DT_COMBOX, listConstrSet, this));
  if (type == RT_IMPLE) {
    QStringList listSynth = m_projManager->getSynthRunsNames();
    m_tableViewRuns->setItemDelegateForColumn(
        3, new RunsGridDelegate(DT_COMBOX, listSynth, this));
  } else if (type == RT_SYNTH) {
    m_tableViewRuns->setItemDelegateForColumn(
        3, new RunsGridDelegate(DT_LABLE, listConstrSet, this));
  }
}

void RunsGrid::setNewSynth(const QStringList &listNewSynth) {
  if (listNewSynth.size()) {
    QStringList listSynth = m_projManager->getSynthRunsNames();
    listSynth += listNewSynth;
    m_tableViewRuns->setItemDelegateForColumn(
        3, new RunsGridDelegate(DT_COMBOX, listSynth, this));
    m_strSynthName = listNewSynth.at(0);
  }
}

QList<rundata> RunsGrid::getRunDataList() {
  QList<rundata> listRunData;
  int rows = m_model->rowCount();
  for (int i = 0; i < rows; ++i) {
    rundata rd;
    rd.m_runName = m_model->item(i, 0)->text();
    rd.m_srcSet = m_model->item(i, 1)->text();
    rd.m_constrSet = m_model->item(i, 2)->text();
    rd.m_iRunType = m_type;
    if (m_type == RT_SYNTH) {
      rd.m_device = m_model->item(i, 3)->text();
    } else if (m_type == RT_IMPLE) {
      rd.m_synthName = m_model->item(i, 3)->text();
    }
    listRunData.append(rd);
  }
  return listRunData;
}

int RunsGrid::getRunDataSize() { return m_model->rowCount(); }

void RunsGrid::ClearGrid() {
  while (m_model->rowCount()) {
    QModelIndex index = m_model->indexFromItem(m_model->item(0));
    m_model->removeRow(index.row());
  }
  emit RowsChanged();
}
void RunsGrid::SlotAddRuns() {
  int rows = m_model->rowCount();
  QList<QStandardItem *> items;
  QStandardItem *item = nullptr;

  QString strRunName;
  if (m_type == RT_SYNTH) {
    strRunName = QString("synth_%1").arg(m_runId);
  } else if (m_type == RT_IMPLE) {
    strRunName = QString("imple_%1").arg(m_runId);
  }

  item = new QStandardItem();
  item->setText(strRunName);
  item->setTextAlignment(Qt::AlignCenter);
  items.append(item);

  item = new QStandardItem();
  item->setText(m_strSrcSet);
  item->setTextAlignment(Qt::AlignCenter);
  items.append(item);

  item = new QStandardItem();
  item->setText(m_strConstrSet);
  item->setTextAlignment(Qt::AlignCenter);
  items.append(item);

  item = new QStandardItem();

  if (m_type == RT_SYNTH) {
    item->setText(m_strDevice);
  } else if (m_type == RT_IMPLE) {
    item->setText(m_strSynthName);
  }
  item->setTextAlignment(Qt::AlignCenter);
  items.append(item);

  m_model->insertRow(rows, items);
  m_runId++;
  emit RowsChanged();
}

void RunsGrid::SlotDeleteRuns() {
  int curRow = m_selectModel->currentIndex().row();
  if (curRow < 0) return;
  m_model->removeRow(curRow);
  emit RowsChanged();
}

void RunsGrid::SlotTableViewSelectionChanged() {
  int curRow = m_selectModel->currentIndex().row();
  if (curRow < 0) {
    m_actDelete->setEnabled(false);
  } else {
    m_actDelete->setEnabled(true);
  }
  return;
}

int RunsGrid::CreateFirstRunId() {
  QStringList strlist;
  if (m_type == RT_SYNTH) {
    strlist = m_projManager->getSynthRunsNames();
  } else if (m_type == RT_IMPLE) {
    strlist = m_projManager->getImpleRunsNames();
  } else {
    return 0;
  }
  int runId = 0;
  foreach (QString str, strlist) {
    QString strNumber = str.right(str.size() - (str.lastIndexOf("_") + 1));
    if (isDigitStr(strNumber)) {
      int number = strNumber.toInt();
      if (number >= runId) {
        runId = number + 1;
      }
    }
  }
  return runId;
}

bool RunsGrid::isDigitStr(QString src) {
  QByteArray ba = src.toLatin1();  // QString to char*
  const char *s = ba.data();

  while (*s && *s >= '0' && *s <= '9') s++;
  if (*s) {
    return false;
  } else {
    return true;
  }
}
