#ifndef CREATEPROJECTDIALOG_H
#define CREATEPROJECTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVector>

#include "ProjectManager/project_manager.h"
#include "add_constraints_form.h"
#include "add_sim_form.h"
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
  INDEX_ADDSIM,
  INDEX_ADDCONST,
  INDEX_DEVICEPL,
  INDEX_SUMMARYF
};

enum Mode {
  NewProject,
  ProjectSettings,
};

class SettingsGuiInterface;

class newProjectDialog : public QDialog {
  Q_OBJECT

 public:
  explicit newProjectDialog(QWidget* parent = nullptr);
  ~newProjectDialog();

  void Next_TclCommand_Test();
  void CreateProject_Tcl_Test(int argc, const char* argv[]);

  QString getProject();
  void Reset(Mode mode = NewProject);
  Mode GetMode() const;
  void SetPageActive(FormIndex index);

  void SetDefaultPath(const QString& path);
  void SetCustomLayoutPath(const QString& path);

 private slots:
  void updateSummaryPage();
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
  addSourceForm* m_addSimForm;
  addConstraintsForm* m_addConstrsForm;
  devicePlannerForm* m_devicePlanForm;
  summaryForm* m_sumForm;
  Mode m_mode{NewProject};
  QVector<SettingsGuiInterface*> m_settings;
  QMap<FormIndex, int> m_tabIndexes;
  QString m_defaultPath;
  QString m_customLayoutPath;

  ProjectManager* m_projectManager;
  bool m_skipSources{false};
  void UpdateDialogView(Mode mode = NewProject);
  void ResetToNewProject();
  void ResetToProjectSettings();
  std::pair<bool, QString> ValuesValid() const;
  QList<QString> FindCompileUnitConflicts() const;
  void updateSummary(const QString& projectName, const QString& projectType);
  devicePlannerForm* CreatePlannerForm(QWidget* parent) const;
};
}  // namespace FOEDAG
#endif  // CREATEPROJECTDIALOG_H
