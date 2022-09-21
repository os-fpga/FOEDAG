#include "add_constraints_form.h"

#include "Compiler/Compiler.h"
#include "MainWindow/Session.h"
#include "ui_add_constraints_form.h"

extern FOEDAG::Session *GlobalSession;

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
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);
  ui->m_frame->setLayout(box);

  ui->m_ckkBoxCopy->setText(tr("Copy sources into project."));
  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Unchecked);

  Compiler *compiler = GlobalSession->GetCompiler();
  if (compiler->PinAssignOpts() == Compiler::PinAssignOpt::Random)
    ui->select_random->setChecked(true);
  else
    ui->select_defineOrder->setChecked(true);
}

addConstraintsForm::~addConstraintsForm() { delete ui; }

QList<filedata> addConstraintsForm::getFileData() {
  return m_widgetGrid->getTableViewData();
}

bool addConstraintsForm::IsCopySource() {
  return ui->m_ckkBoxCopy->checkState() == Qt::CheckState::Checked ? true
                                                                   : false;
}

bool addConstraintsForm::IsRandom() const {
  return ui->select_random->isChecked();
}
