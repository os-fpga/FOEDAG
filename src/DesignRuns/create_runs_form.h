#ifndef CREATE_RUNS_FORM_H
#define CREATE_RUNS_FORM_H

#include <QWidget>

namespace Ui {
class CreateRunsForm;
}

class CreateRunsForm : public QWidget {
  Q_OBJECT

 public:
  explicit CreateRunsForm(QWidget *parent = nullptr);
  ~CreateRunsForm();

 private:
  Ui::CreateRunsForm *ui;
};

#endif  // CREATE_RUNS_FORM_H
