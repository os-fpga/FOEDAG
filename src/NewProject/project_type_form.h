#ifndef PROJECTTYPEFORM_H
#define PROJECTTYPEFORM_H

#include <QWidget>

namespace Ui {
class projectTypeForm;
}

class projectTypeForm : public QWidget {
  Q_OBJECT

 public:
  explicit projectTypeForm(QWidget *parent = nullptr);
  ~projectTypeForm();

  QString getProjectType();

 private:
  Ui::projectTypeForm *ui;
};

#endif  // PROJECTTYPEFORM_H
