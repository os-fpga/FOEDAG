#ifndef CREATE_RUNS_DIALOG_H
#define CREATE_RUNS_DIALOG_H

#include <QDialog>

#include "create_runs_form.h"

namespace Ui {
class CreateRunsDialog;
}

namespace FOEDAG {

class CreateRunsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit CreateRunsDialog(QWidget *parent = nullptr);
  ~CreateRunsDialog();

  void InitDialog(int itype);
  QList<rundata> getRunDataList();

 private slots:
  void on_m_btnCancel_clicked();
  void on_m_btnOK_clicked();

 private:
  Ui::CreateRunsDialog *ui;
  CreateRunsForm *m_createRunsForm;

  ProjectManager *m_projManager;
};
}  // namespace FOEDAG
#endif  // CREATE_RUNS_DIALOG_H
