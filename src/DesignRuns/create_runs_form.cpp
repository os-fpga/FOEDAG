#include "create_runs_form.h"

#include "ui_create_runs_form.h"

CreateRunsForm::CreateRunsForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::CreateRunsForm) {
  ui->setupUi(this);
}

CreateRunsForm::~CreateRunsForm() { delete ui; }
