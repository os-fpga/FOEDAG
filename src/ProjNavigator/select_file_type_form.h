#ifndef SELECT_FILE_TYPE_FORM_H
#define SELECT_FILE_TYPE_FORM_H

#include <QWidget>

namespace Ui {
class SelectFileTypeForm;
}
namespace FOEDAG {

class SelectFileTypeForm : public QWidget {
  Q_OBJECT

 public:
  explicit SelectFileTypeForm(QWidget *parent = nullptr);
  ~SelectFileTypeForm();

  void setSelectedType(int itype);
  int getSelectedType();

 private:
  Ui::SelectFileTypeForm *ui;
};
}  // namespace FOEDAG
#endif  // SELECT_FILE_TYPE_FORM_H
