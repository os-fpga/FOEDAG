#ifndef RUNS_OPTIONS_FORM_H
#define RUNS_OPTIONS_FORM_H

#include <QWidget>

namespace Ui {
class RunsOptionsForm;
}
namespace FOEDAG {

class RunsOptionsForm : public QWidget {
  Q_OBJECT

 public:
  explicit RunsOptionsForm(QWidget *parent = nullptr);
  ~RunsOptionsForm();

 private:
  Ui::RunsOptionsForm *ui;
};
}  // namespace FOEDAG
#endif  // RUNS_OPTIONS_FORM_H
