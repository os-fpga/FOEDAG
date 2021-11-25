#ifndef ADD_SIMULATION_FORM_H
#define ADD_SIMULATION_FORM_H

#include <QWidget>

namespace Ui {
class AddSimulationForm;
}

class AddSimulationForm : public QWidget {
  Q_OBJECT

 public:
  explicit AddSimulationForm(QWidget *parent = nullptr);
  ~AddSimulationForm();

 private:
  Ui::AddSimulationForm *ui;
};

#endif  // ADD_SIMULATION_FORM_H
