#ifndef PROJECTTYPEFORM_H
#define PROJECTTYPEFORM_H

#include <QWidget>

namespace Ui {
class projectTypeForm;
}

namespace FOEDAG {

class projectTypeForm : public QWidget {
  Q_OBJECT

 public:
  explicit projectTypeForm(QWidget *parent = nullptr);
  ~projectTypeForm();

  QString getProjectType();

 private:
  Ui::projectTypeForm *ui;
};
}  // namespace FOEDAG
#endif  // PROJECTTYPEFORM_H
