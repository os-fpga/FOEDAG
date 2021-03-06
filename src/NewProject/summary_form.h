#ifndef SUMMARYFORM_H
#define SUMMARYFORM_H

#include <QWidget>

namespace Ui {
class summaryForm;
}

namespace FOEDAG {

class summaryForm : public QWidget {
  Q_OBJECT

 public:
  explicit summaryForm(QWidget *parent = nullptr);
  ~summaryForm();

  void setProjectName(const QString &proName, const QString &proType);
  void setSourceCount(const int &srcCount, const int constrCount);
  void setDeviceInfo(const QStringList &listDevItem);

 private:
  Ui::summaryForm *ui;
};
}  // namespace FOEDAG
#endif  // SUMMARYFORM_H
