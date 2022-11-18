#include "project_type_form.h"

#include "ui_project_type_form.h"

using namespace FOEDAG;

projectTypeForm::projectTypeForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::projectTypeForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Type of Project"));
  ui->m_labelDetail->setText(tr("Specify the type of project to create."));
  ui->m_radioBtnRTL->setText(tr("RTL Project"));
  ui->m_radioBtnPost->setText(tr("Post-synthesis Project"));
  ui->m_labelRTL->setText(
      tr("- Generate IP.\n"
         "- Run Analysis, Synthesis, P&R timing & generate bitstream."));

  ui->m_radioBtnRTL->setChecked(true);

  connect(ui->m_skipSourcesCheckbox, &QAbstractButton::clicked, this,
          &projectTypeForm::skipSources);

  ui->radioBtnPOSMixed->setEnabled(false);
  ui->radioBtnPOSPure->setEnabled(false);
  ui->radioBtnPOSMixed->setChecked(true);

  connect(ui->m_radioBtnPost, &QAbstractButton::toggled, this,
          [this](bool clicked) {
            ui->radioBtnPOSMixed->setEnabled(clicked);
            ui->radioBtnPOSPure->setEnabled(clicked);
          });
}

projectTypeForm::~projectTypeForm() { delete ui; }

ProjectType projectTypeForm::projectType() const {
  if (ui->m_radioBtnRTL->isChecked()) {
    return RTL;
  } else if (ui->m_radioBtnPost->isChecked()) {
    if (ui->radioBtnPOSMixed->isChecked()) {
      return PostSynthWithHDL;
    } else if (ui->radioBtnPOSPure->isChecked()) {
      return PostSynthPure;
    }
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
    case PostSynthWithHDL:
      return "Post-synthesis with HDL";
    case PostSynthPure:
      return "Post-synthesis";
  }
  return QString{};
}
