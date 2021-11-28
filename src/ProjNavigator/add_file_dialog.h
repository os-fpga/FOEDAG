#ifndef ADD_FILE_DIALOG_H
#define ADD_FILE_DIALOG_H

#include <QDialog>

#include "add_file_form.h"

namespace Ui {
class AddFileDialog;
}

namespace FOEDAG {

class AddFileDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AddFileDialog(QWidget *parent = nullptr);
  ~AddFileDialog();

  void InitDialog(int itype);

 public:
  AddFileForm *m_fileForm;

 private slots:
  void on_m_btnCancel_clicked();
  void on_m_btnOK_clicked();

 private:
  Ui::AddFileDialog *ui;
};
}  // namespace FOEDAG
#endif  // ADD_FILE_DIALOG_H
