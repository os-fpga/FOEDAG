#include "new_project_dialog.h"
#include "ui_new_project_dialog.h"

#include <QDesktopWidget>
#include <QMessageBox>
using namespace FOEDAG;

newProjectDialog::newProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::newProjectDialog),
    m_index(INDEX_LOCATION)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("New Project"));

    m_bfinished = false;
    //One thirds of desktop size
    QDesktopWidget dw;
    int w=dw.width();
    int h=dw.height();
    setGeometry(w/3,h/3,w/3,h/3);

    m_locationForm = new locationForm(this);
    ui->m_stackedWidget->insertWidget(1,m_locationForm);
    m_proTypeForm = new projectTypeForm(this);
    ui->m_stackedWidget->insertWidget(2,m_proTypeForm);
    m_addSrcForm = new addSourceForm(this);
    ui->m_stackedWidget->insertWidget(3,m_addSrcForm);
    m_addConstrsForm = new addConstraintsForm(this);
    ui->m_stackedWidget->insertWidget(4,m_addConstrsForm);
    m_devicePlanForm = new devicePlannerForm(this);
    ui->m_stackedWidget->insertWidget(5,m_devicePlanForm);
    m_sumForm = new summaryForm(this);
    ui->m_stackedWidget->insertWidget(6,m_sumForm);
    ui->m_stackedWidget->adjustSize();

    updatedialogview();

}

newProjectDialog::~newProjectDialog()
{
    delete ui;
}

void newProjectDialog::on_m_btnBack_clicked()
{
    m_index--;
    updatedialogview();
}

void newProjectDialog::on_m_btnNext_clicked()
{
    m_index++;
    updatedialogview();
}

void newProjectDialog::on_m_btnFinish_clicked()
{
    m_bfinished = true;
    this->close();
}

void newProjectDialog::on_m_btnCancel_clicked()
{
    m_bfinished = false;
    this->close();
}
void newProjectDialog::updatedialogview()
{
    if(INDEX_LOCATION == m_index)
    {
        ui->m_btnBack->setEnabled(false);
    }
    else {
        ui->m_btnBack->setEnabled(true);
    }

    if(INDEX_SUMMARYF == m_index)
    {
        ui->m_btnNext->setEnabled(false);
        ui->m_btnFinish->setEnabled(true);
        int ptype = m_proTypeForm->getprojecttype();

        QString series;QString device;QString package;
        m_devicePlanForm->getdeviceinfo(series, device, package);
        m_sumForm->setprojectname(m_locationForm->getprojectname(),ptype);
        m_sumForm->setdeviceinfo(series,device);
        m_sumForm->setsourcecount(0,0);
    }
    else
    {
        ui->m_btnNext->setEnabled(true);
        ui->m_btnFinish->setEnabled(false);
    }

        ui->m_stackedWidget->setCurrentIndex(m_index);
}

