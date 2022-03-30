#ifndef RUNS_SUMMARY_FORM_H
#define RUNS_SUMMARY_FORM_H

#include <QWidget>

namespace Ui {
class RunsSummaryForm;
}
namespace FOEDAG {

class RunsSummaryForm : public QWidget {
  Q_OBJECT

 public:
  explicit RunsSummaryForm(QWidget *parent = nullptr);
  ~RunsSummaryForm();

  void setRunsCount(const int &synth, const int &imple);

 private:
  Ui::RunsSummaryForm *ui;
};
}  // namespace FOEDAG
#endif  // RUNS_SUMMARY_FORM_H
