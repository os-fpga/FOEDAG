#ifndef CREATEPROJECTDIALOG_H
#define CREATEPROJECTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVector>

#include "add_constraints_form.h"
#include "add_source_form.h"
#include "device_planner_form.h"
#include "location_form.h"
#include "project_type_form.h"
#include "summary_form.h"

enum FormIndex {
  INDEX_LOCATION = 1,
  INDEX_PROJTYPE,
  INDEX_ADDSOURC,
  INDEX_ADDCONST,
  INDEX_DEVICEPL,
  INDEX_SUMMARYF
};

namespace Ui {
class newProjectDialog;
}

namespace FOEDAG {

class newProjectDialog : public QDialog {
  Q_OBJECT

 public:
  explicit newProjectDialog(QWidget* parent = nullptr);
  ~newProjectDialog();

  void tcl_command_test();
 private slots:
  void on_m_btnBack_clicked();
  void on_m_btnNext_clicked();
  void on_m_btnFinish_clicked();
  void on_m_btnCancel_clicked();

 private:
  Ui::newProjectDialog* ui;
  int m_index;

  locationForm* m_locationForm;
  projectTypeForm* m_proTypeForm;
  addSourceForm* m_addSrcForm;
  addConstraintsForm* m_addConstrsForm;
  devicePlannerForm* m_devicePlanForm;
  summaryForm* m_sumForm;

  void UpdateDialogView();
};
}  // namespace FOEDAG
#endif  // CREATEPROJECTDIALOG_H
