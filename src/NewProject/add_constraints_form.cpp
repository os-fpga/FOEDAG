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
  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Checked);
  ui->select_defineOrder->setChecked(true);
  Compiler *compiler = GlobalSession->GetCompiler();
  compiler->PinAssignOpts(Compiler::PinAssignOpt::In_Define_Order);
  connect(ui->select_defineOrder, &QRadioButton::clicked, this,
          &addConstraintsForm::pinAssign_opt_listen);
  connect(ui->select_random, &QRadioButton::clicked, this,
          &addConstraintsForm::pinAssign_opt_listen);
}

addConstraintsForm::~addConstraintsForm() { delete ui; }

QList<filedata> addConstraintsForm::getFileData() {
  return m_widgetGrid->getTableViewData();
}

bool addConstraintsForm::IsCopySource() {
  return ui->m_ckkBoxCopy->checkState() == Qt::CheckState::Checked ? true
                                                                   : false;
}

void addConstraintsForm::pinAssign_opt_listen() {
  Compiler *compiler = GlobalSession->GetCompiler();
  if (!ui->select_random->isChecked())
    compiler->PinAssignOpts(Compiler::PinAssignOpt::In_Define_Order);
  else
    compiler->PinAssignOpts(Compiler::PinAssignOpt::Random);
}
