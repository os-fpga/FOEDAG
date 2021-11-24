#ifndef CREATE_DESIGN_DIALOG_H
#define CREATE_DESIGN_DIALOG_H

#include <QDialog>

namespace Ui {
class CreateDesignDialog;
}

namespace FOEDAG {

class CreateDesignDialog : public QDialog {
  Q_OBJECT

 public:
  explicit CreateDesignDialog(QWidget *parent = nullptr);
  ~CreateDesignDialog();

 private slots:
  void on_m_btnCancel_clicked();
  void on_m_btnOK_clicked();

 private:
  Ui::CreateDesignDialog *ui;
};
}  // namespace FOEDAG
#endif  // CREATE_DESIGN_DIALOG_H
