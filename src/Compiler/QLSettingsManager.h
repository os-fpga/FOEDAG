
#include <QWidget>
#include <QString>
#include <QComboBox>
#include <QStackedWidget>
#include <QListWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <string>
#include <vector>
#include <set>
#include <filesystem>

#include "nlohmann_json/json.hpp"
#include "QLDeviceManager.h"

#ifndef QLSETTINGSMANAGER_H
#define QLSETTINGSMANAGER_H

using json = nlohmann::ordered_json;

namespace FOEDAG {

class QLSettingsManager : public QObject {
  Q_OBJECT

private:
  QLSettingsManager(QObject* parent = nullptr);

public:
  static QLSettingsManager* getInstance();
  static void reloadJSONSettings();
  static std::string getStringValue(std::string category, std::string subcategory, std::string parameter);
  static long double getLongDoubleValue(std::string category, std::string subcategory, std::string parameter);
  static std::string getStringToolTip(std::string category, std::string subcategory, std::string parameter);
  static const json* getJson(std::string category, std::string subcategory, std::string parameter);
  static const json* getJson(std::string category, std::string subcategory);
  static const json* getJson(std::string category);
  static std::filesystem::path getSDCFilePath();
  static std::filesystem::path getTCLScriptDirPath();
  static std::filesystem::path getCurrentDirPath();

  ~QLSettingsManager();

  QWidget* createSettingsWidget(bool newProjectMode);
  void updateSettingsWidget();
  void populateSettingsWidget();
 
  void updateJSONSettingsForDeviceTarget(QLDeviceTarget device_target);
  void parseJSONSettings();
  void parseSDCFilePath();
  bool areJSONSettingsChanged();
  bool saveJSONSettings();

  void handleApplyButtonClicked();
  void handleResetButtonClicked();
  void handleSettingsChanged();

signals:
  void settingsChanged();

public:
  static QLSettingsManager* instance;
  QLDeviceManager* device_manager = nullptr;

  json settings_json;
  std::filesystem::path settings_json_filepath;

  json power_estimation_json;
  std::filesystem::path power_estimation_json_filepath;

  json combined_json;

  std::filesystem::path sdc_file_path;
  bool sdc_file_path_from_json = false;

  // GUI elements and GUI related variables
  QWidget* settings_manager_widget = nullptr;
  QStackedWidget* stackedWidget; // each 'category' is a 'page'(represented by a QTabWidget) in the stackedWidget
  QListWidget* listWidget;   // each category is an entry in the listwidget

  QPushButton *button_reset;
  QPushButton *button_apply;
  QLabel* m_message_label;

  json settings_json_updated;
  json power_estimation_json_updated;
  std::vector<std::string> settings_json_change_list;
  std::vector<std::string> power_estimation_json_change_list;

  // GUI can operate in 'newProjectMode' or 'existingProjectMode'
  bool newProjectMode=false;
  json settings_json_newproject;
  json power_estimation_json_newproject;

};



}

#endif // QLSETTINGSMANAGER_H