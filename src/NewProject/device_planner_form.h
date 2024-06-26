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

struct Filters {
  QString family;
  QString series;
  QString package;
};

class devicePlannerForm : public QWidget, public SettingsGuiInterface {
  Q_OBJECT

 public:
  explicit devicePlannerForm(const std::filesystem::path &deviceFile,
                             const std::function<void(const QString &)> &logger,
                             QWidget *parent = nullptr);
  ~devicePlannerForm() override;

  void CreateDevice_TclTest();
  void EditDevice_TclTest();

  QString selectedDeviceName() const;
  QList<QString> getSelectedDevice() const;
  void updateUi(ProjectManager *pm) override;

  static void CreateDevice(
      const QStringList &deviceList, const QStringList &allDevices,
      const QString &selectedDevice, const std::filesystem::path &deviceFile,
      std::function<void(const QString &)> onSuccess,
      const std::function<void(const QString &msg)> &logger, QWidget *parent);
  static QStringList getOriginalDeviceList(
      const std::filesystem::path &deviceFile);

 private slots:
  void onSeriestextChanged(const QString &arg1);
  void onFamilytextChanged(const QString &arg1);
  void onPackagetextChanged(const QString &arg1);

  void createDevice();
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
  void init(const Filters &filter);
  Filters currentFilter() const;
  std::function<void(const QString &)> m_logger{nullptr};
};
}  // namespace FOEDAG
#endif  // DEVICEPLANNERFORM_H
