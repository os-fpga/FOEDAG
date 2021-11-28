#include "create_design_dialog.h"

#include <QMessageBox>

#include "ui_create_design_dialog.h"

using namespace FOEDAG;

CreateDesignDialog::CreateDesignDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CreateDesignDialog) {
  ui->setupUi(this);
  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
  setWindowTitle(tr("Create Design"));
}

CreateDesignDialog::~CreateDesignDialog() { delete ui; }

void CreateDesignDialog::InitDialog(QString strContent) {
  ui->m_labelContent->setText(strContent);
}

QString CreateDesignDialog::getDesignName() const {
  return ui->m_lineEditName->text();
}

void FOEDAG::CreateDesignDialog::on_m_btnOK_clicked() {
  if ("" == ui->m_lineEditName->text()) {
    QMessageBox::information(this, tr("Information"),
                             tr("Please specify a set name"), QMessageBox::Ok);
    return;
  }
  this->setResult(1);
  this->hide();
}

void FOEDAG::CreateDesignDialog::on_m_btnCancel_clicked() {
  this->setResult(0);
  this->hide();
}
