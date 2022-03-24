#include "add_constraints_form.h"

#include "ui_add_constraints_form.h"

using namespace FOEDAG;

addConstraintsForm::addConstraintsForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::addConstraintsForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Add Constraints (optional)"));
  ui->m_labelDetail->setText(
      tr("Specify or create constraint file for physical and timing "
         "constraints."));

  m_widgetGrid = new sourceGrid(ui->m_frame);
  m_widgetGrid->setGridType(GT_CONSTRAINTS);
  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_frame);
  box->addWidget(m_widgetGrid);
  box->setContentsMargins(0, 0, 0, 1);
  box->setSpacing(0);
  ui->m_frame->setLayout(box);

  ui->m_ckkBoxCopy->setText(tr("Copy sources into project."));
  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Checked);
}

addConstraintsForm::~addConstraintsForm() { delete ui; }

QList<filedata> addConstraintsForm::getFileData() {
  return m_widgetGrid->getTableViewData();
}

bool addConstraintsForm::IsCopySource() {
  return ui->m_ckkBoxCopy->checkState() == Qt::CheckState::Checked ? true
                                                                   : false;
}
