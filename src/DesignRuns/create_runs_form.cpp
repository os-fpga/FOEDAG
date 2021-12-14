#include "create_runs_form.h"

#include "ui_create_runs_form.h"

using namespace FOEDAG;

CreateRunsForm::CreateRunsForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::CreateRunsForm) {
  ui->setupUi(this);
}

CreateRunsForm::~CreateRunsForm() { delete ui; }

void CreateRunsForm::InitForm(int itype) {
  if (RT_SYNTH == itype) {
    ui->m_labelTitle->setText(tr("Configure Synthesis Runs"));
    ui->m_labelDetail->setText(
        tr("Create and configure one or more synthesis runs using various "
           "parts,constraints,flows and strategies."));

    ui->m_labelSummary->setText(tr("0 synthesis run will be created."));

    // m_widgetGrid = new sourceGrid(GT_SOURCE, ui->m_groupBox);

  } else if (RT_IMPLE == itype) {
    ui->m_labelTitle->setText(tr("Configure Implementation Runs"));
    ui->m_labelDetail->setText(
        tr("Create and configure one or more Implementation runs using various "
           "parts,constraints,flows and strategies."));
    ui->m_labelSummary->setText(tr("0 Implementation run will be created."));

    // m_widgetGrid = new sourceGrid(GT_CONSTRAINTS, ui->m_groupBox);
  }

  //    QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom,
  //    ui->m_groupBox); box->addWidget(m_widgetGrid);
  //    box->setContentsMargins(0, 0, 0, 1);
  //    box->setSpacing(0);
  //    ui->m_groupBox->setLayout(box);
}
