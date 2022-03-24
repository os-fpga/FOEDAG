#include "create_fileset_dialog.h"

#include <QMessageBox>

#include "ui_create_fileset_dialog.h"

using namespace FOEDAG;

CreateFileSetDialog::CreateFileSetDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CreateFileSetDialog) {
  ui->setupUi(this);
  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
}

CreateFileSetDialog::~CreateFileSetDialog() { delete ui; }

void CreateFileSetDialog::InitDialog(int itype) {
  if (FST_DESIGN == itype) {
    setWindowTitle(tr("Create Design File Set"));
    ui->m_labelContent->setText(tr("Enter Design File Set Name"));

  } else if (FST_CONSTR == itype) {
    setWindowTitle(tr("Create Constraint File Set"));
    ui->m_labelContent->setText(tr("Enter Constraint File Set Name"));
  } else if (FST_SIM == itype) {
    setWindowTitle(tr("Create Simulation File Set"));
    ui->m_labelContent->setText(tr("Enter Simulation File Set Name"));
  }
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
