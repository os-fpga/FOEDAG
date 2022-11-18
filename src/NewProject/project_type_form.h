#ifndef PROJECTTYPEFORM_H
#define PROJECTTYPEFORM_H

#include <QWidget>

#include "ProjectManager/project_manager.h"

namespace Ui {
class projectTypeForm;
}

namespace FOEDAG {

class projectTypeForm : public QWidget {
  Q_OBJECT

 public:
  explicit projectTypeForm(QWidget *parent = nullptr);
  ~projectTypeForm();

  ProjectType projectType() const;
  QString projectTypeStr() const;
  static QString projectTypeStr(ProjectType type);

 signals:
  void skipSources(bool skip);

 private:
  Ui::projectTypeForm *ui;
};
}  // namespace FOEDAG
#endif  // PROJECTTYPEFORM_H
