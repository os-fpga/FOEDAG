#include "create_design_dialog.h"

#include "ui_create_design_dialog.h"

using namespace FOEDAG;

CreateDesignDialog::CreateDesignDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CreateDesignDialog) {
  ui->setupUi(this);
}

CreateDesignDialog::~CreateDesignDialog() { delete ui; }

void FOEDAG::CreateDesignDialog::on_m_btnOK_clicked() {}

void FOEDAG::CreateDesignDialog::on_m_btnCancel_clicked() { this->close(); }
