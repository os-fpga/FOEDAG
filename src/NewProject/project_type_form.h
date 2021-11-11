#ifndef PROJECTTYPEFORM_H
#define PROJECTTYPEFORM_H

#include <QWidget>

enum project_type { TYPE_RTL, TYPE_POST };

namespace Ui {
class projectTypeForm;
}

class projectTypeForm : public QWidget {
  Q_OBJECT

 public:
  explicit projectTypeForm(QWidget *parent = nullptr);
  ~projectTypeForm();

  int getProjectType();

 private:
  Ui::projectTypeForm *ui;
};

#endif  // PROJECTTYPEFORM_H
