#include "sources_form.h"
#include "ui_sources_form.h"

SourcesForm::SourcesForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SourcesForm)
{
    ui->setupUi(this);
}

SourcesForm::~SourcesForm()
{
    delete ui;
}
