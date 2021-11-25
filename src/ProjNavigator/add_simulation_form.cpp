#include "add_simulation_form.h"

#include "ui_add_simulation_form.h"

AddSimulationForm::AddSimulationForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::AddSimulationForm) {
  ui->setupUi(this);
}

AddSimulationForm::~AddSimulationForm() { delete ui; }
