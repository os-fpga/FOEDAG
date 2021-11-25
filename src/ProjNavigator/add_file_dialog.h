#ifndef ADD_FILE_DIALOG_H
#define ADD_FILE_DIALOG_H

#include <QDialog>

namespace Ui {
class AddFileDialog;
}

class AddFileDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AddFileDialog(QWidget *parent = nullptr);
  ~AddFileDialog();

 private:
  Ui::AddFileDialog *ui;
};

#endif  // ADD_FILE_DIALOG_H
