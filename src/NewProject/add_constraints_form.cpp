#include "add_constraints_form.h"

#include "ui_add_constraints_form.h"

addConstraintsForm::addConstraintsForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::addConstraintsForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Add Constraints (optional)"));
  ui->m_labelDetail->setText(
      tr("Specify or create constraint file for physical and timing "
         "constraints."));

  m_widgetgrid = new sourceGrid(GT_CONSTRAINTS, ui->m_groupBox);
  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_groupBox);
  box->addWidget(m_widgetgrid);
  box->setContentsMargins(0, 0, 0, 1);
  box->setSpacing(0);
  ui->m_groupBox->setLayout(box);

  ui->m_ckkBoxCopy->setText(tr("Copy sources into project."));
  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Checked);
}

addConstraintsForm::~addConstraintsForm() { delete ui; }

QList<filedata> addConstraintsForm::getfiledata() {
  return m_widgetgrid->getgriddata();
}

bool addConstraintsForm::iscopysource() {
  return ui->m_ckkBoxCopy->checkState() == Qt::CheckState::Checked ? true
                                                                   : false;
}
