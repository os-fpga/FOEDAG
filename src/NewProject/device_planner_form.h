#ifndef DEVICEPLANNERFORM_H
#define DEVICEPLANNERFORM_H
#include <QStandardItemModel>
#include <QTableView>
#include <QWidget>

namespace Ui {
class devicePlannerForm;
}

class devicePlannerForm : public QWidget {
  Q_OBJECT

 public:
  explicit devicePlannerForm(QWidget *parent = nullptr);
  ~devicePlannerForm();

  QList<QString> getSelectedDevice() const;

 private slots:
  void onSeriestextChanged(const QString &arg1);
  void onFamilytextChanged(const QString &arg1);
  void onPackagetextChanged(const QString &arg1);

 private:
  Ui::devicePlannerForm *ui;

  QTableView *m_tableView;
  QStandardItemModel *m_model;
  QItemSelectionModel *m_selectmodel;

  void InitSeriesComboBox();
  void InitDeviceTableViewHead();
  void UpdateFamilyComboBox();
  void UpdatePackageComboBox();
  void UpdateDeviceTableView();
};

#endif  // DEVICEPLANNERFORM_H
