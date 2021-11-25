#include "add_file_form.h"

#include "ui_add_file_form.h"

using namespace FOEDAG;

AddFileForm::AddFileForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::AddFileForm) {
  ui->setupUi(this);
}

AddFileForm::~AddFileForm() { delete ui; }

void AddFileForm::InitForm(int itype) {
  if (GT_SOURCE == itype) {
    ui->m_labelTitle->setText(tr("Add or Create Design Sources"));
    ui->m_labelDetail->setText(tr(
        "Specify design files,or directories containing those files,to add to "
        "your project."
        "Create a new source file on disk and add it to your project."));

    m_widgetGrid = new sourceGrid(GT_SOURCE, ui->m_groupBox);
    ui->m_ckkBoxCopy->setText(tr("Copy sources into project."));

  } else if (GT_CONSTRAINTS == itype) {
    ui->m_labelTitle->setText(tr("Add or Create Constraints"));
    ui->m_labelDetail->setText(
        tr("Specify or create constraint files for physical and timing "
           "constraints to add to your project."));

    m_widgetGrid = new sourceGrid(GT_CONSTRAINTS, ui->m_groupBox);
    ui->m_ckkBoxCopy->setText(tr("Copy constraints files into project."));
  } else if (GT_SIM == itype) {
    ui->m_labelTitle->setText(tr("Add or Create Simulation Sources"));
    ui->m_labelDetail->setText(
        tr("Specify simulation specific HDL files,or directories containing "
           "HDL files,to add to "
           "your project."
           "Create a new source file on disk and add it to your project."));

    m_widgetGrid = new sourceGrid(GT_SOURCE, ui->m_groupBox);
    ui->m_ckkBoxCopy->setText(tr("Copy sources into project."));
  }

  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_groupBox);
  box->addWidget(m_widgetGrid);
  box->setContentsMargins(0, 0, 0, 1);
  box->setSpacing(0);
  ui->m_groupBox->setLayout(box);

  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Checked);
}

QList<filedata> AddFileForm::getFileData() {
  return m_widgetGrid->getTableViewData();
}

bool AddFileForm::IsCopySource() {
  return ui->m_ckkBoxCopy->checkState() == Qt::CheckState::Checked ? true
                                                                   : false;
}
