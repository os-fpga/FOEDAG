#include "summary_form.h"
#include "ui_summary_form.h"
#include "project_type_form.h"

summaryForm::summaryForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::summaryForm)
{
    ui->setupUi(this);
    ui->m_labelTitle->setText(tr("New Project Summary"));
    ui->m_labelTail->setText(tr("To create the project,click Finish."));
}

summaryForm::~summaryForm()
{
    delete ui;
}
void summaryForm::setprojectname(QString strname,int itype)
{
    QString tstrtype = itype == TYPE_RTL ? QString("RTL") : QString("Post-synthesis");
    ui->m_labelNamePic->setStyleSheet(QString("image: url(:/img/info.png);"));
    ui->m_labelName->setText(QString(tr("A new %1 project named '%2' will be created.")).arg(tstrtype).arg(strname));
}

void summaryForm::setsourcecount(int source, int constr)
{
    if(0 == source)
    {
        ui->m_labelSourcesPic->setStyleSheet(QString("image: url(:/img/warn.png);"));
        ui->m_labelSources->setText(tr("No source files or directories will be added. Use Add Sources to add them later."));
    }
    else
    {
        ui->m_labelSourcesPic->setStyleSheet(QString("image: url(:/img/info.png);"));
        ui->m_labelSources->setText(QString(tr("%1 source files will be added.")).arg(source));
    }

    if(0 == constr)
    {
        ui->m_labelConstraintsPic->setStyleSheet(QString("image: url(:/img/warn.png);"));
        ui->m_labelConstraints->setText(tr("No constraints files will be added. Use Add Sources to add them later."));
    }
    else
    {
        ui->m_labelConstraintsPic->setStyleSheet(QString("image: url(:/img/info.png);"));
        ui->m_labelConstraints->setText(QString(tr("%1 constraints files will be added.")).arg(constr));
    }
}

void summaryForm::setdeviceinfo(QString strseries, QString device)
{
    ui->m_labelSDevPic->setStyleSheet(QString("image: url(:/img/info.png);"));
    ui->m_labelSeries->setText(QString("Series: %1").arg(strseries));
    ui->m_labelDevice->setText(QString("Device: %1").arg(device));
}
