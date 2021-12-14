#include "create_runs_dialog.h"

#include <QDesktopWidget>

#include "ui_create_runs_dialog.h"

using namespace FOEDAG;

CreateRunsDialog::CreateRunsDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CreateRunsDialog) {
  ui->setupUi(this);
  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

  // One thirds of desktop size
  QDesktopWidget dw;
  int w = dw.width() / 3;
  int h = dw.height() / 3;
  setGeometry(w, h, w, h);

  m_createRunsForm = new CreateRunsForm(this);
}

CreateRunsDialog::~CreateRunsDialog() { delete ui; }

void CreateRunsDialog::InitDialog(int itype) {
  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_widgetForm);
  box->addWidget(m_createRunsForm);
  box->setContentsMargins(0, 0, 0, 1);
  box->setSpacing(0);
  ui->m_widgetForm->setLayout(box);

  setWindowTitle(tr("Create New Runs"));

  m_createRunsForm->InitForm(itype);
}
