#include "create_runs_form.h"

#include "ui_create_runs_form.h"

using namespace FOEDAG;

CreateRunsForm::CreateRunsForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::CreateRunsForm) {
  ui->setupUi(this);
}

CreateRunsForm::~CreateRunsForm() { delete ui; }

void CreateRunsForm::InitForm(int itype) {
  m_formType = itype;
  if (RT_SYNTH == m_formType) {
    ui->m_labelTitle->setText(tr("Configure Synthesis Runs"));
    ui->m_labelDetail->setText(
        tr("Create and configure one or more synthesis runs using various "
           "parts,constraints,flows and strategies."));
    ui->m_labelSummary->setText(tr("0 synthesis run will be created."));
  } else if (RT_IMPLE == m_formType) {
    ui->m_labelTitle->setText(tr("Configure Implementation Runs"));
    ui->m_labelDetail->setText(
        tr("Create and configure one or more Implementation runs using various "
           "parts,constraints,flows and strategies."));
    ui->m_labelSummary->setText(tr("0 Implementation run will be created."));
  }

  m_widgetRunsGrid = new RunsGrid(m_formType == RT_SYNTH ? RT_SYNTH : RT_IMPLE,
                                  ui->m_groupBox);
  connect(m_widgetRunsGrid, SIGNAL(RowsChanged()), this,
          SLOT(SlotGridRowsChanged()));

  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_groupBox);
  box->addWidget(m_widgetRunsGrid);
  box->setContentsMargins(0, 0, 0, 1);
  box->setSpacing(0);
  ui->m_groupBox->setLayout(box);
}

QList<rundata> CreateRunsForm::getRunDataList() {
  return m_widgetRunsGrid->getRunDataList();
}

void CreateRunsForm::SlotGridRowsChanged() {
  if (RT_SYNTH == m_formType) {
    QString strSummary = QString("%1 synthesis run will be created.")
                             .arg(m_widgetRunsGrid->getRunDataList().count());
    ui->m_labelSummary->setText(strSummary);
  } else if (RT_IMPLE == m_formType) {
    QString strSummary = QString("%1 implementation run will be created.")
                             .arg(m_widgetRunsGrid->getRunDataList().count());
    ui->m_labelSummary->setText(strSummary);
  }
}
