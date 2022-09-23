#ifndef CREATEPROJECTDIALOG_H
#define CREATEPROJECTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVector>

#include "ProjectManager/project_manager.h"
#include "add_constraints_form.h"
#include "add_source_form.h"
#include "device_planner_form.h"
#include "location_form.h"
#include "project_type_form.h"
#include "summary_form.h"

namespace Ui {
class newProjectDialog;
}

namespace FOEDAG {

enum FormIndex {
  INDEX_LOCATION = 0,
  INDEX_PROJTYPE,
  INDEX_ADDSOURC,
  INDEX_ADDCONST,
  INDEX_DEVICEPL,
  INDEX_SUMMARYF
};

class newProjectDialog : public QDialog {
  Q_OBJECT

 public:
  explicit newProjectDialog(QWidget* parent = nullptr);
  ~newProjectDialog();

  void Next_TclCommand_Test();
  void CreateProject_Tcl_Test(int argc, const char* argv[]);

  QString getProject();
  void Reset();

 private slots:

  void on_buttonBox_accepted();
  void on_buttonBox_rejected();
  void on_next();
  void on_back();

 private:
  Ui::newProjectDialog* ui;
  int m_index;
  QPushButton* NextBtn;
  QPushButton* BackBtn;
  locationForm* m_locationForm;
  projectTypeForm* m_proTypeForm;
  addSourceForm* m_addSrcForm;
  addConstraintsForm* m_addConstrsForm;
  devicePlannerForm* m_devicePlanForm;
  summaryForm* m_sumForm;

  ProjectManager* m_projectManager;
  void UpdateDialogView();
};
}  // namespace FOEDAG
#endif  // CREATEPROJECTDIALOG_H
