#ifndef CREATE_RUNS_FORM_H
#define CREATE_RUNS_FORM_H

#include <QWidget>

namespace Ui {
class CreateRunsForm;
}

namespace FOEDAG {

enum RunType { RT_SYNTH, RT_IMPLE };

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
