#include "new_file_dialog.h"

#include <qevent.h>

#include <QTextStream>

using namespace FOEDAG;

NewFileDialog::NewFileDialog(QWidget *parent) : QFileDialog(parent) {}

NewFileDialog::NewFileDialog(QWidget *parent, const QString &caption,
                             const QString &directory, const QString &filter)
    : QFileDialog(parent, caption, directory, filter) {}

void NewFileDialog::closeEvent(QCloseEvent *event) {
  event->ignore();
  this->hide();
}

void NewFileDialog::done(int result) {
  emit finished(result);
  this->hide();
}
