#include "project_type_form.h"

#include "ui_project_type_form.h"

using namespace FOEDAG;

projectTypeForm::projectTypeForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::projectTypeForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Type of Project"));
  ui->m_labelDetail->setText(tr("Specify the type of project to create."));
  ui->m_radioBtnRTL->setText(tr("RTL Project"));
  ui->m_radioBtnPost->setText(tr("Gate-level Project"));
  ui->m_labelRTL->setText(
      tr("- Generate IP\n"
         "- Run Analysis, Synthesis, P&R timing & generate bitstream"));
  ui->m_labelPOS->setText(
      tr("- Only one of .edif, .edf, .blif, .eblif, .v file allowed\n"
         "- Run P&R timing & generate bitstream"));

  ui->m_radioBtnRTL->setChecked(true);

  connect(ui->m_skipSourcesCheckbox, &QAbstractButton::clicked, this,
          &projectTypeForm::skipSources);
}

projectTypeForm::~projectTypeForm() { delete ui; }

ProjectType projectTypeForm::projectType() const {
  if (ui->m_radioBtnRTL->isChecked()) {
    return RTL;
  } else if (ui->m_radioBtnPost->isChecked()) {
    return GateLevel;
  }
  return RTL;
}

QString projectTypeForm::projectTypeStr() const {
  return projectTypeStr(projectType());
}

QString projectTypeForm::projectTypeStr(ProjectType type) {
  switch (type) {
    case RTL:
      return "RTL";
    case GateLevel:
      return "Gate-level";
  }
  return QString{};
}
