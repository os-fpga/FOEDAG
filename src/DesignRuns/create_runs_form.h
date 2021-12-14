#ifndef CREATE_RUNS_FORM_H
#define CREATE_RUNS_FORM_H

#include <QWidget>
#include "runs_grid.h"

namespace Ui {
class CreateRunsForm;
}

namespace FOEDAG {

class CreateRunsForm : public QWidget {
  Q_OBJECT

 public:
  explicit CreateRunsForm(QWidget *parent = nullptr);
  ~CreateRunsForm();

  void InitForm(int itype);

 private:
  Ui::CreateRunsForm *ui;
};
}  // namespace FOEDAG
#endif  // CREATE_RUNS_FORM_H
