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
}

CreateRunsDialog::~CreateRunsDialog() { delete ui; }
