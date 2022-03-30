#ifndef SELECT_DESIGN_TYPE_FORM_H
#define SELECT_DESIGN_TYPE_FORM_H

#include <QWidget>

namespace Ui {
class SelectDesignTypeForm;
}

namespace FOEDAG {

enum DesignRunType { DRT_SYNTH, DRT_IMPLE, DRT_BOTH };

class SelectDesignTypeForm : public QWidget {
  Q_OBJECT

 public:
  explicit SelectDesignTypeForm(QWidget *parent = nullptr);
  ~SelectDesignTypeForm();

  int getDesignType() const;

 private:
  Ui::SelectDesignTypeForm *ui;
};
}  // namespace FOEDAG
#endif  // SELECT_DESIGN_TYPE_FORM_H
