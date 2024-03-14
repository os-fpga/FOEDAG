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

  QList<QString> getSelectedDevice() const;
  void updateUi(ProjectManager *pm) override;
  QString customLayoutFile() const;
  void setCustomLayoutPath(const QString &path);

 private slots:
  void onSeriestextChanged(const QString &arg1);
  void onFamilytextChanged(const QString &arg1);
  void onPackagetextChanged(const QString &arg1);

  void on_pushButton_clicked();

 private:
  Ui::devicePlannerForm *ui;

  QTableView *m_tableView;
  QStandardItemModel *m_model;
  QItemSelectionModel *m_selectmodel;
  std::filesystem::path m_deviceFile{};
  QString m_customLayoutPath{};

  void InitSeriesComboBox();
  void InitDeviceTableViewHead();
  void UpdateFamilyComboBox();
  void UpdatePackageComboBox();
  void UpdateDeviceTableView();
  void UpdateSelection(const QModelIndex &index);
};
}  // namespace FOEDAG
#endif  // DEVICEPLANNERFORM_H
