#ifndef CREATE_FILESET_DIALOG_H
#define CREATE_FILESET_DIALOG_H

#include <QDialog>

namespace Ui {
class CreateFileSetDialog;
}

namespace FOEDAG {

enum FileSetType { FST_DESIGN, FST_CONSTR, FST_SIM };

class CreateFileSetDialog : public QDialog {
  Q_OBJECT

 public:
  explicit CreateFileSetDialog(QWidget *parent = nullptr);
  ~CreateFileSetDialog();

  void InitDialog(int itype);
  QString getDesignName() const;

 private slots:
  void on_m_btnCancel_clicked();
  void on_m_btnOK_clicked();

 private:
  Ui::CreateFileSetDialog *ui;
};
}  // namespace FOEDAG
#endif  // CREATE_DESIGN_DIALOG_H
