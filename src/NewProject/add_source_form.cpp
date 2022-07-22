#include "add_source_form.h"

#include "ui_add_source_form.h"

using namespace FOEDAG;

addSourceForm::addSourceForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::addSourceForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Add Sources"));
  ui->m_labelDetail->setText(tr(
      "Specify design files, or directories containing those files, to add to "
      "your project. "
      "Create a new source file on disk and add it to your project. "));

  m_widgetGrid = new sourceGrid(ui->m_frame);
  m_widgetGrid->setGridType(GT_SOURCE);
  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_frame);
  box->addWidget(m_widgetGrid);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);
  ui->m_frame->setLayout(box);

  ui->m_ckkBoxCopy->setText(tr("Copy sources into project. "));
  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Checked);
}

addSourceForm::~addSourceForm() { delete ui; }

QList<filedata> addSourceForm::getFileData() {
  return m_widgetGrid->getTableViewData();
}

bool addSourceForm::IsCopySource() {
  return ui->m_ckkBoxCopy->checkState() == Qt::CheckState::Checked ? true
                                                                   : false;
}
