#include "add_file_dialog.h"

#include <QDesktopWidget>

#include "ui_add_file_dialog.h"

AddFileDialog::AddFileDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AddFileDialog) {
  ui->setupUi(this);

  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

  // One thirds of desktop size
  QDesktopWidget dw;
  int w = dw.width() / 3;
  int h = dw.height() / 3;
  setGeometry(w, h, w, h);
}

AddFileDialog::~AddFileDialog() { delete ui; }
