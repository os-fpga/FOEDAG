#ifndef CREATEFILEDIALOG_H
#define CREATEFILEDIALOG_H

#include <QDialog>

#include "source_grid.h"

namespace Ui {
class createFileDialog;
}

namespace FOEDAG {

class createFileDialog : public QDialog {
  Q_OBJECT

  enum FileType {
    Verilog,
    SystemVerilog,
    VHDL,
    Cpp,
    Pin,
    Sdc,
  };

 public:
  explicit createFileDialog(const QString &projectPath,
                            QWidget *parent = nullptr);
  ~createFileDialog();
  static bool verifyFileName(const QString &fileName, QWidget *parent);

  void initialDialog(int type);
 signals:
  void sig_updateGrid(FOEDAG::filedata fdata);

 private slots:
  void on_m_pushBtnOK_clicked();
  void on_m_pushBtnCancel_clicked();
  void on_m_comboxFileLocation_currentIndexChanged(int index);

 private:
  bool FileExists(const filedata &fData) const;
  static QString AppendExtension(const QString &fileName, const QString &ext);

 private:
  Ui::createFileDialog *ui;
  const QString m_projectPath;

  int m_type;
};
}  // namespace FOEDAG
#endif  // CREATEFILEDIALOG_H
