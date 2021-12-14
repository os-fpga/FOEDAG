#include "create_runs_dialog.h"
#include "ui_create_runs_dialog.h"

CreateRunsDialog::CreateRunsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateRunsDialog)
{
    ui->setupUi(this);
}

CreateRunsDialog::~CreateRunsDialog()
{
    delete ui;
}
