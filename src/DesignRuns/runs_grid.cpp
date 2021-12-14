#include "runs_grid.h"

#include <QVBoxLayout>

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

  m_actModify = new QAction(m_toolBar);
  // m_actModify->setIcon( QIcon(""));
  m_actModify->setText(tr("&Modify"));
  m_actModify->setEnabled(false);
  m_toolBar->addAction(m_actModify);

  m_actDelete = new QAction(m_toolBar);
  // m_actDelete->setIcon( QIcon(""));
  m_actDelete->setText(tr("&Delete"));
  m_actDelete->setEnabled(false);
  m_toolBar->addAction(m_actDelete);

  m_tableViewRuns = new QTableView(this);

  // Set properties
  m_tableViewRuns->verticalHeader()->hide();
  m_tableViewRuns->verticalHeader()->setDefaultSectionSize(30);
  // Last column adaptive width
  m_tableViewRuns->horizontalHeader()->setStretchLastSection(true);
  m_tableViewRuns->setEditTriggers(QTableView::NoEditTriggers);
  m_tableViewRuns->setSelectionBehavior(QTableView::SelectRows);
  // Single line selection
  m_tableViewRuns->setSelectionMode(QTableView::SingleSelection);
  // Color separation between lines
  m_tableViewRuns->setAlternatingRowColors(true);

  m_tableViewRuns->setStyleSheet(
      "QTableView {border: 1px solid rgb(230,230,230);}\
                            QTableView::item:selected{color:black;background: #63B8FF;}");
  m_tableViewRuns->setColumnWidth(0, 80);

  m_model = new QStandardItemModel(m_tableViewRuns);
  m_selectModel = new QItemSelectionModel(m_model, m_model);

  m_tableViewRuns->horizontalHeader()->setMinimumHeight(30);

  if (type == RT_SYNTH) {
    m_model->setHorizontalHeaderItem(0, new QStandardItem(tr("Name")));
    m_model->setHorizontalHeaderItem(1, new QStandardItem(tr("Sources Set")));
    m_model->setHorizontalHeaderItem(2,
                                     new QStandardItem(tr("Constraints Set")));
    m_model->setHorizontalHeaderItem(3, new QStandardItem(tr("Device")));
  } else if (type == RT_IMPLE) {
    m_model->setHorizontalHeaderItem(0, new QStandardItem(tr("Name")));
    m_model->setHorizontalHeaderItem(1, new QStandardItem(tr("Sources Set")));
    m_model->setHorizontalHeaderItem(2,
                                     new QStandardItem(tr("Constraints Set")));
    m_model->setHorizontalHeaderItem(0, new QStandardItem(tr("Synth Name")));
  }

  m_tableViewRuns->setModel(m_model);
  m_tableViewRuns->setSelectionModel(m_selectModel);
  //    connect(m_selectModel, &QItemSelectionModel::selectionChanged, this,
  //            &sourceGrid::TableViewSelectionChanged);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addWidget(m_toolBar);
  vbox->addWidget(m_tableViewRuns);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(1);
  setLayout(vbox);
}
