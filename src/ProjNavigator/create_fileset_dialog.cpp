#include "create_fileset_dialog.h"

#include <QMessageBox>

#include "ui_create_fileset_dialog.h"

using namespace FOEDAG;

CreateFileSetDialog::CreateFileSetDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CreateFileSetDialog) {
  ui->setupUi(this);
  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
  setWindowTitle(tr("Create File Set"));
}

CreateFileSetDialog::~CreateFileSetDialog() { delete ui; }

void CreateFileSetDialog::InitDialog(QString strContent) {
  ui->m_labelContent->setText(strContent);
}

QString CreateFileSetDialog::getDesignName() const {
  return ui->m_lineEditName->text();
}

void FOEDAG::CreateFileSetDialog::on_m_btnOK_clicked() {
  if ("" == ui->m_lineEditName->text()) {
    QMessageBox::information(this, tr("Information"),
                             tr("Please specify a set name"), QMessageBox::Ok);
    return;
  }
  this->setResult(1);
  this->hide();
}

void FOEDAG::CreateFileSetDialog::on_m_btnCancel_clicked() {
  this->setResult(0);
  this->hide();
}
