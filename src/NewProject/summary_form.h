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
  void setProjectSettings(bool on);

  void setProjectName(const QString &proName, const QString &proType);
  void setSourceCount(const int &srcCount, const int constrCount, int simCount);
  void setDeviceInfo(const QStringList &listDevItem);
  void setCustomLayoutFile(const QString &file);

 private:
  Ui::summaryForm *ui;
  bool m_projectSettings{false};
};
}  // namespace FOEDAG
#endif  // SUMMARYFORM_H
