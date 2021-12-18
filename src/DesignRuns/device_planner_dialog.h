#ifndef DEVICE_PLANNER_DIALOG_H
#define DEVICE_PLANNER_DIALOG_H

#include <QDialog>

#include "NewProject/device_planner_form.h"

namespace Ui {
class DevicePlannerDialog;
}

namespace FOEDAG {

class DevicePlannerDialog : public QDialog {
  Q_OBJECT

 public:
  explicit DevicePlannerDialog(QWidget *parent = nullptr);
  ~DevicePlannerDialog();

  QString getSelectedDevice();

 private slots:
  void on_m_btnCancel_clicked();
  void on_m_btnOK_clicked();

 private:
  Ui::DevicePlannerDialog *ui;
  devicePlannerForm *m_deviceForm;
};
}  // namespace FOEDAG
#endif  // DEVICE_PLANNER_DIALOG_H
