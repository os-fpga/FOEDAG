#ifndef SUMMARYFORM_H
#define SUMMARYFORM_H

#include <QWidget>

namespace Ui {
class summaryForm;
}

class summaryForm : public QWidget {
  Q_OBJECT

 public:
  explicit summaryForm(QWidget *parent = nullptr);
  ~summaryForm();

  void setprojectname(QString strname, int itype);
  void setsourcecount(int source, int constr);
  void setdeviceinfo(QString strseries, QString device);

 private:
  Ui::summaryForm *ui;
};

#endif  // SUMMARYFORM_H
