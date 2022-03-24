#ifndef ADD_FILE_DIALOG_H
#define ADD_FILE_DIALOG_H

#include <QDialog>

#include "add_file_form.h"
#include "select_file_type_form.h"

namespace Ui {
class AddFileDialog;
}

namespace FOEDAG {

enum AddFileFormIndex { INDEX_TYPESELECT = 1, INDEX_FILEFORM };

class AddFileDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AddFileDialog(QWidget *parent = nullptr);
  ~AddFileDialog();

  void setSelected(int iSelected);

 signals:
  void RefreshFiles();

 private slots:
  void on_m_btnCancel_clicked();
  void on_m_btnOK_clicked();
  void on_m_btnBack_clicked();

 private:
  Ui::AddFileDialog *ui;
  int m_formIndex;

  ProjectManager *m_pm;

  AddFileForm *m_fileForm;
  SelectFileTypeForm *m_selectForm;

  void UpdateDialogView();
};
}  // namespace FOEDAG
#endif  // ADD_FILE_DIALOG_H
