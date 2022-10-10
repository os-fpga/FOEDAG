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
  ui->m_labelPost->setText(
      tr("- Add design netlist.\n"
         "- P&R, timing & generate bitstream.\n"
         "- Run analysis (optional), Synthesis (optional), timing and "
         "generate bitstream."));

  ui->m_radioBtnRTL->setChecked(true);

  QObject::connect(ui->m_skipSourcesCheckbox, &QAbstractButton::clicked, this,
                   &projectTypeForm::skipSources);

  ui->m_radioBtnPost->hide();
  ui->m_labelPost->hide();
}

projectTypeForm::~projectTypeForm() { delete ui; }
QString projectTypeForm::getProjectType() {
  if (ui->m_radioBtnRTL->isChecked()) {
    return "RTL";
  } else if (ui->m_radioBtnPost->isChecked()) {
    return "Post-synthesis";
  }
  return "RTL";
}
