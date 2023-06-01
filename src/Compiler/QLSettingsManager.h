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

public:
  static QLSettingsManager* getInstance();
  ~QLSettingsManager();

 private:
  QLSettingsManager(QObject *parent = nullptr);


 public:
 QWidget* createSettingsWidget();
 void updateSettingsWidget();
 void populateSettingsWidget();
 
 void updateJSONSettingsForDeviceTarget(QLDeviceTarget device_target);
 void parseJSONSettings();
 bool areJSONSettingsChanged();
 bool saveJSONSettings();

 static void reloadJSONSettings();
 static std::string getStringValue(std::string category, std::string subcategory, std::string parameter);
 static std::string getStringToolTip(std::string category, std::string subcategory, std::string parameter);
 static const json* getJson(std::string category, std::string subcategory, std::string parameter);
 static const json* getJson(std::string category, std::string subcategory);
 static const json* getJson(std::string category);

 public slots:
 void handleApplyButtonClicked();
 void handleResetButtonClicked();
 void handleSettingsChanged();

 public:
 static QLSettingsManager* instance;
 QLDeviceManager* device_manager = nullptr;

 json settings_json;
 std::filesystem::path settings_json_filepath;

 json power_estimation_json;
 std::filesystem::path power_estimation_json_filepath;

 json combined_json;

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



};



}

#endif // QLSETTINGSMANAGER_H