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

class CompilerOpenFPGA_ql;

class QLSettingsManager : public QObject {
  Q_OBJECT

public:
  static QLSettingsManager* getInstance(CompilerOpenFPGA_ql *compiler);
  ~QLSettingsManager();

 private:
  QLSettingsManager(CompilerOpenFPGA_ql *compiler, QObject *parent = nullptr);


 public:
 QWidget* createSettingsWidget();
 void parseJSONSettings();
 void saveJSONSettings();

 public slots:
 void handleSaveButtonClicked();

 public:
 static QLSettingsManager* instance;
 QLDeviceManager* device_manager = nullptr;
 CompilerOpenFPGA_ql* compiler;

 json settings_json;
 std::filesystem::path settings_json_filepath;

 json power_estimation_json;
 std::filesystem::path power_estimation_json_filepath;

 // GUI elements
 QTabWidget* settings_manager_container_widget = nullptr;
 QWidget* device_manager_widget = nullptr; // caution: this is owned by QLDeviceManager.
 QWidget* settings_manager_widget = nullptr;
 QStackedWidget* stackedWidget; // each 'category' is a 'page'(represented by a QTabWidget) in the stackedWidget
 QListWidget* listWidget;   // each category is an entry in the listwidget

};



}

#endif // QLSETTINGSMANAGER_H