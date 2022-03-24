#include "select_file_type_form.h"

#include "NewProject/source_grid.h"
#include "ui_select_file_type_form.h"

using namespace FOEDAG;

SelectFileTypeForm::SelectFileTypeForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::SelectFileTypeForm) {
  ui->setupUi(this);
  ui->m_radioBtnDesign->setChecked(true);
}

SelectFileTypeForm::~SelectFileTypeForm() { delete ui; }

void SelectFileTypeForm::setSelectedType(int itype) {
  if (GT_CONSTRAINTS == itype) {
    ui->m_radioBtnConstr->setChecked(true);
  } else if (GT_SIM == itype) {
    ui->m_radioBtnSim->setChecked(true);
  } else {
    ui->m_radioBtnDesign->setChecked(true);
  }
}

int SelectFileTypeForm::getSelectedType() {
  if (ui->m_radioBtnConstr->isChecked()) {
    return GT_CONSTRAINTS;
  } else if (ui->m_radioBtnSim->isChecked()) {
    return GT_SIM;
  } else {
    return GT_SOURCE;
  }
}
