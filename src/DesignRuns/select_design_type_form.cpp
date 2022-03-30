#include "select_design_type_form.h"

#include "ui_select_design_type_form.h"

using namespace FOEDAG;

SelectDesignTypeForm::SelectDesignTypeForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::SelectDesignTypeForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Create New Runs"));
  ui->m_labelDetail->setText(
      tr("This wizard will guide you through the process of creating multiple "
         "synthesis or/and implementation runs. You can apply different file "
         "set and device to these design runs. "));
  ui->m_labelOption->setText(
      tr("What type design runs do you want to create?"));
  ui->m_radioBtnSynth->setText(tr("  Synthesis"));
  ui->m_radioBtnImple->setText(tr("  Implementation (Placement and Route)"));
  ui->m_radioBtnBoth->setText(tr(" Both"));

  ui->m_radioBtnSynth->setChecked(true);
}

SelectDesignTypeForm::~SelectDesignTypeForm() { delete ui; }

int SelectDesignTypeForm::getDesignType() const {
  if (ui->m_radioBtnImple->isChecked()) {
    return DRT_IMPLE;
  } else if (ui->m_radioBtnBoth->isChecked()) {
    return DRT_BOTH;
  } else {
    return DRT_SYNTH;
  }
}
