#ifndef DEVICEPLANNERFORM_H
#define DEVICEPLANNERFORM_H
#include <QStandardItemModel>
#include <QTableView>
#include <QWidget>
#include <filesystem>

#include "SettingsGuiInterface.h"

namespace Ui {
class devicePlannerForm;
}

namespace FOEDAG {

class devicePlannerForm : public QWidget, public SettingsGuiInterface {
  Q_OBJECT

 public:
  explicit devicePlannerForm(const std::filesystem::path &deviceFile,
                             QWidget *parent = nullptr);
  ~devicePlannerForm() override;

  QString selectedDeviceName() const;
  QList<QString> getSelectedDevice() const;
  void updateUi(ProjectManager *pm) override;

 private slots:
  void onSeriestextChanged(const QString &arg1);
  void onFamilytextChanged(const QString &arg1);
  void onPackagetextChanged(const QString &arg1);

  void on_pushButtonCreate_clicked();
  void updateEditDeviceButtons();
  void removeDevice();
  void editDevice();

 private:
  Ui::devicePlannerForm *ui;

  QTableView *m_tableView;
  QStandardItemModel *m_model;
  QItemSelectionModel *m_selectmodel;
  std::filesystem::path m_deviceFile{};
  QStringList m_originalDeviceList{};

  void InitSeriesComboBox();
  void InitDeviceTableViewHead();
  void UpdateFamilyComboBox();
  void UpdatePackageComboBox();
  void UpdateDeviceTableView();
  void UpdateSelection(const QModelIndex &index);
  void init();
  QStringList getOriginalDeviceList() const;
};
}  // namespace FOEDAG
#endif  // DEVICEPLANNERFORM_H
