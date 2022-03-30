#ifndef CREATE_RUNS_DIALOG_H
#define CREATE_RUNS_DIALOG_H

#include <QDialog>

#include "create_runs_form.h"

namespace Ui {
class CreateRunsDialog;
}

namespace FOEDAG {

class CreateRunsForm;
class SelectDesignTypeForm;
class RunsSummaryForm;

enum CreateRunsFormIndex {
  CRFI_SELECTTYPE = 1,
  CRFI_SYNTHRUN,
  CRFI_IMPLERUN,
  CRFI_RUNSUMARY
};

class CreateRunsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit CreateRunsDialog(QWidget *parent = nullptr);
  ~CreateRunsDialog();

 signals:
  void RefreshRuns();

 private slots:
  void on_m_btnCancel_clicked();
  void on_m_btnOK_clicked();
  void on_m_btnBack_clicked();

 private:
  Ui::CreateRunsDialog *ui;
  int m_formIndex;

  SelectDesignTypeForm *m_selectTypeForm;
  CreateRunsForm *m_createSynthRun;
  CreateRunsForm *m_createImpleRun;
  RunsSummaryForm *m_runsSummaryForm;

  ProjectManager *m_pm;

  void UpdateDialogView();
  int CreateSynthRuns();
  int CreateImpleRuns();
};
}  // namespace FOEDAG
#endif  // CREATE_RUNS_DIALOG_H
