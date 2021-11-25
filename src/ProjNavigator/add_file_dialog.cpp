#include "add_file_dialog.h"

#include <QDesktopWidget>

#include "ui_add_file_dialog.h"

using namespace FOEDAG;

AddFileDialog::AddFileDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AddFileDialog) {
  ui->setupUi(this);

  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

  // One thirds of desktop size
  QDesktopWidget dw;
  int w = dw.width() / 3;
  int h = dw.height() / 3;
  setGeometry(w, h, w, h);

  m_fileForm = new AddFileForm(this);
}

AddFileDialog::~AddFileDialog() { delete ui; }

void AddFileDialog::InitDialog(int itype) {
  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_widgetForm);
  box->addWidget(m_fileForm);
  box->setContentsMargins(0, 0, 0, 1);
  box->setSpacing(0);
  ui->m_widgetForm->setLayout(box);

  if (GT_SOURCE == itype) {
    setWindowTitle(tr("Add Sources"));
  } else if (GT_CONSTRAINTS == itype) {
    setWindowTitle(tr("Add Sources"));
  } else if (GT_SIM == itype) {
    setWindowTitle(tr("Add Sources"));
  }

  m_fileForm->InitForm(itype);
}

void AddFileDialog::on_m_btnOK_clicked() {
  this->setResult(1);
  this->hide();
}

void AddFileDialog::on_m_btnCancel_clicked() {
  this->setResult(0);
  this->hide();
}
