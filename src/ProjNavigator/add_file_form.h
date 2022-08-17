#ifndef ADD_SIMULATION_FORM_H
#define ADD_SIMULATION_FORM_H

#include <QWidget>

#include "NewProject/source_grid.h"

namespace Ui {
class AddFileForm;
}

namespace FOEDAG {

class ProjectManager;

class AddFileForm : public QWidget {
  Q_OBJECT

 public:
  explicit AddFileForm(QWidget *parent = nullptr);
  ~AddFileForm();

  void InitForm(int itype);

  QString getFileSet() const;
  QList<filedata> getFileData() const;
  bool IsCopySource() const;

 private slots:
  void on_m_comboBoxSets_currentIndexChanged(const QString &arg1);

 private slots:
  void pinAssign_opt_listen();

 private:
  Ui::AddFileForm *ui;

  ProjectManager *m_pm;

  sourceGrid *m_widgetGrid;
  void initSetComboBox(int itype);
};
}  // namespace FOEDAG
#endif  // ADD_SIMULATION_FORM_H
