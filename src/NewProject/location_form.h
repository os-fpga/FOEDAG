#ifndef LOCATIONFORM_H
#define LOCATIONFORM_H

#include <QWidget>

namespace Ui {
class locationForm;
}

namespace FOEDAG {

class locationForm : public QWidget {
  Q_OBJECT

 public:
  explicit locationForm(const QString &defaultPath = QString{},
                        QWidget *parent = nullptr);
  ~locationForm();

  QString getProjectName();
  QString getProjectPath();
  bool IsCreateDir();
  bool IsProjectNameExit();

 private slots:
  void on_m_btnBrowse_clicked();
  void on_m_checkBox_stateChanged(int arg1);
  void on_m_lineEditPname_textChanged(const QString &arg1);

 private:
  QString defaultDir() const;
  void updateLabel(int state, const QString &path, const QString &name);

 private:
  Ui::locationForm *ui;
};
}  // namespace FOEDAG
#endif  // LOCATIONFORM_H
