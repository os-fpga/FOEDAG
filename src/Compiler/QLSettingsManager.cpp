#include "QLSettingsManager.h"

#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include <QDirIterator>

#include <iostream>
#include <set>
#include <regex>
#include <filesystem>

#include <CRFileCryptProc.hpp>
#include "CompilerOpenFPGA_ql.h"
#include "Compiler/Compiler.h"
#include "Utils/FileUtils.h"
#include "Utils/LogUtils.h"
#include "Utils/StringUtils.h"
#include "MainWindow/Session.h"
#include "Main/WidgetFactory.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "QLDeviceManager.h"

using json = nlohmann::ordered_json;

extern FOEDAG::Session* GlobalSession;

namespace FOEDAG {

// singleton init
QLSettingsManager* QLSettingsManager::instance = NULL;

QLSettingsManager* QLSettingsManager::getInstance() {

  std::cout << "1" << std::endl;
  // creation
  if(instance == NULL) {

    instance = new QLSettingsManager();
  }

  std::cout << "2" << std::endl;

  if(instance->device_manager == nullptr) {

    instance->device_manager = QLDeviceManager::getInstance(true);
  }

  std::cout << "3" << std::endl;

  // init
  instance->parseJSONSettings();

  std::cout << "4" << std::endl;

  if( !((instance->settings_json).empty()) ) {

    std::string family              = instance->settings_json["general"]["device"]["family"]["default"];
    std::string foundry             = instance->settings_json["general"]["device"]["foundry"]["default"];
    std::string node                = instance->settings_json["general"]["device"]["node"]["default"];
    std::string voltage_threshold   = instance->settings_json["general"]["device"]["voltage_threshold"]["default"];
    std::string p_v_t_corner        = instance->settings_json["general"]["device"]["p_v_t_corner"]["default"];
    std::string layout              = instance->settings_json["general"]["device"]["layout"]["default"];
    
    std::cout << "\n" << "---------------------------------------------" << std::endl;
    std::cout << "current device_target from settings json:" << std::endl;
    std::cout << " >> [family]              " << family << std::endl;
    std::cout << " >> [foundry]             " << foundry << std::endl;
    std::cout << " >> [node]                " << node << std::endl;
    std::cout << " >> [voltage_threshold]   " << voltage_threshold << std::endl;
    std::cout << " >> [p_v_t_corner]        " << p_v_t_corner << std::endl;
    std::cout << " >> [layout]              " << layout << std::endl;
    std::cout << "---------------------------------------------\n" << std::endl;

    instance->device_manager->setCurrentDeviceTarget(family, foundry, node, voltage_threshold, p_v_t_corner, layout);
  }

  std::cout << "5" << std::endl;

  return instance;
}


QLSettingsManager::QLSettingsManager(QObject *parent)
    : QObject(parent) {}

QLSettingsManager::~QLSettingsManager() {
  if(settings_manager_container_widget != nullptr) {
    settings_manager_container_widget->deleteLater();
  }
}

QWidget* QLSettingsManager::createSettingsWidget() {

  std::cout << "6" << std::endl;

  if(settings_manager_container_widget != nullptr) {
    settings_manager_container_widget->deleteLater();
  }

  settings_manager_container_widget = new QTabWidget();
  settings_manager_container_widget->setWindowTitle("Task Settings");


  std::cout << "7" << std::endl;

  settings_manager_widget = new QWidget();

  QWidget* dlg = settings_manager_widget;

  dlg->setWindowTitle("Settings");

  QVBoxLayout* dlg_toplevellayout = new QVBoxLayout();
  QHBoxLayout* dlg_widgetslayout = new QHBoxLayout();
  QHBoxLayout* dlg_buttonslayout = new QHBoxLayout();
  dlg_toplevellayout->addLayout(dlg_widgetslayout); // first the settings stuff
  dlg_toplevellayout->addLayout(dlg_buttonslayout); // second a row of buttons for actions
  dlg->setLayout(dlg_toplevellayout);

  std::cout << "8" << std::endl;

  // json structure:
  //  category0
  //    subcategory0
  //      param0 {}
  //      param1 {}
  //    subcategory1
  //      param2 {}
  //      param3 {}
  // category1
  //    ... and so on.

  // 1. settings GUI representation, top-level -> Widget
  // 2. split layout into 
  //      (a) QListWidget (list of categories)
  //      (b) QStackedWidget (content of the categories)
  // 3. on selecting a category (QListWidget) -> corresponding widget which has the content of the category is visible
  // 4. each widget in the category widget (QStackedWidget) will be a QTabWidget
  //    each of the tabs in it represents a subcategory
  //    so, all the params in a subcategory will be displayed inside a 'tab' which is the 'subcategory widget'

  stackedWidget = new QStackedWidget();
  listWidget = new QListWidget();

  json rootJson = settings_json;
  if(!power_estimation_json.empty()) {
    rootJson.update(power_estimation_json);
  }

  std::cout << "9" << std::endl;

  for (auto [categoryId, categoryJson] : rootJson.items()) {

    if(categoryId == "Tasks") {
      // we don't use this section in our use-cases.
      continue;
    }

    if(categoryId == "general") {
      // we don't use this section here, it is represented by the QLDeviceManager.
      continue;
    }

    // container widget for each 'category' -> this will be each 'page' of the QStackedWidget
    // this container widget will contain all the 'subcategories' of the category -> hence a QTabWidget for each category
    QTabWidget* categoryWidget = new QTabWidget();
    categoryWidget->setProperty("settings_category_id", QString::fromStdString(categoryId));

    for (auto [subcategoryId, subcategoryJson] : categoryJson.items()) {

      if(categoryId == "general" && subcategoryId == "device") {
        continue;
      }

      // container widget for each 'subcategory'  -> 'page' widget inside the category QTabWidget
      // the container widget will contain all the 'parameters' of this subcategory -> hence a simple QWidget will do.
      // this should become a ScrollArea.
      QWidget* subcategoryWidget = new QWidget();
      QVBoxLayout* subcategoryWidgetlayout = new QVBoxLayout();
      subcategoryWidgetlayout->setAlignment(Qt::AlignTop);
      subcategoryWidget->setLayout(subcategoryWidgetlayout);
      subcategoryWidget->setProperty("settings_subcategory_id", QString::fromStdString(subcategoryId));


      for (auto [widgetId, widgetJson] : subcategoryJson.items()) {

          // std::cout << "widgetId: " << QString::fromStdString(widgetId).toStdString() << std::endl;
          // std::cout << "widgetType: " << QString::fromStdString(widgetJson["widgetType"].get<std::string>()).toStdString() << std::endl;

          // finally, each parameter becomes a widget according the type and properties.
          QWidget* containerWidget = new QWidget();
          QHBoxLayout* containerWidgetHBoxLayout = new QHBoxLayout();
          containerWidget->setLayout(containerWidgetHBoxLayout);
          containerWidget->setProperty("settings_json_id", QString::fromStdString(widgetId));
          containerWidget->setProperty("settings_json_value_widgetType", QString::fromStdString(widgetJson["widgetType"].get<std::string>()));


          QLabel* subWidgetLabel = new QLabel(QString::fromStdString(widgetId));
          QWidget* subWidget =
              FOEDAG::createWidget(widgetJson, QString::fromStdString(widgetId));
          
          containerWidgetHBoxLayout->addWidget(subWidgetLabel);
          containerWidgetHBoxLayout->addStretch();
          containerWidgetHBoxLayout->addWidget(subWidget);
          
          // the parameter 'container' widget is added into the 'subcategory widget' layout.
          subcategoryWidgetlayout->addWidget(containerWidget);
      }

      // subcategory widget ready -> this is a 'page' or 'tab' in QTabWidget, so add to the QTabWidget directly (no layout)
      categoryWidget->addTab(subcategoryWidget, QString::fromStdString(subcategoryId));

    }

    // category widget is ready -> this is a 'page' in the 'container' QStackedWidget
    stackedWidget->addWidget(categoryWidget);

    // correspondingly, add the category 'name' into the QListWidget
    QLabel* listItemWidgetLabel = new QLabel(QString::fromStdString(categoryId));
    listItemWidgetLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    listItemWidgetLabel->setLineWidth(1);
    QListWidgetItem* listItem = new QListWidgetItem();
    listItem->setSizeHint(listItemWidgetLabel->sizeHint());
    listWidget->addItem(listItem);
    listWidget->setItemWidget(listItem, listItemWidgetLabel);
    //new QListWidgetItem(QString::fromStdString(categoryId), listWidget);
  }

  std::cout << "10" << std::endl;
  
  // // Power JSON Settings also should be added into the Settings Menu
  // if(!power_estimation_json.empty()) {

  //     QTabWidget* categoryWidget = new QTabWidget(); // power
  //     categoryWidget->setProperty("settings_category_id", QString::fromStdString("power"));

  //     QWidget* subcategoryWidget = new QWidget(); // power_inputs
  //     QVBoxLayout* subcategoryWidgetlayout = new QVBoxLayout();
  //     subcategoryWidgetlayout->setAlignment(Qt::AlignTop);
  //     subcategoryWidget->setLayout(subcategoryWidgetlayout);
  //     subcategoryWidget->setProperty("settings_subcategory_id", QString::fromStdString("power_inputs"));

  //     for (auto [widgetId, widgetJson] : power_estimation_json["power_inputs"].items()) {

  //         QWidget* containerWidget = new QWidget();
  //         QHBoxLayout* containerWidgetHBoxLayout = new QHBoxLayout();
  //         containerWidget->setLayout(containerWidgetHBoxLayout);
  //         containerWidget->setProperty("settings_json_id", QString::fromStdString(widgetId));
  //         containerWidget->setProperty("settings_json_value_widgetType", QString::fromStdString(widgetJson["widgetType"].get<std::string>()));

  //         QLabel* label = new QLabel(QString::fromStdString(widgetId));
  //         std::string valuestring = std::to_string(widgetJson["default"].get<double>());
  //         QLineEdit* lineEdit = new QLineEdit(QString::fromStdString(valuestring));

  //         containerWidgetHBoxLayout->addWidget(label);
  //         containerWidgetHBoxLayout->addStretch();
  //         containerWidgetHBoxLayout->addWidget(lineEdit);

  //         subcategoryWidgetlayout->addWidget(containerWidget);
  //     }

  //     categoryWidget->addTab(subcategoryWidget, QString::fromStdString("power_inputs"));
  //     stackedWidget->addWidget(categoryWidget);
  //     new QListWidgetItem(QString::fromStdString("power"), listWidget);
  // }



  // when a 'category' in the QListView is selected, corresponding 'page' widget in the QStackedWidget should be shown.
  QObject::connect(listWidget, QOverload<int>::of(&QListWidget::currentRowChanged),
            stackedWidget, &QStackedWidget::setCurrentIndex);


  // container widget for all settings, add the QListView(left side), and then QStackedWidget(right side)
  dlg_widgetslayout->addWidget(listWidget);
  dlg_widgetslayout->addWidget(stackedWidget);

  listWidget->setCurrentRow(0);

  // make the buttons for the actions in the settings dialog
  QPushButton *button_loadfromjson = new QPushButton("Reload");
  button_loadfromjson->setToolTip("Reload everything from the JSON file");
  QPushButton *button_savetojson = new QPushButton("Save");
  button_savetojson->setToolTip("Save everything to the JSON file");
  QPushButton *button_cancel = new QPushButton("Cancel");
  button_cancel->setToolTip("Discard any modifications to the current session");
  QPushButton *button_save = new QPushButton("Apply");
  button_save->setToolTip("Keep any modifications for the current session");
  connect(button_save, SIGNAL(released()), this, SLOT(handleSaveButtonClicked()));
  
  dlg_buttonslayout->addWidget(button_loadfromjson);
  dlg_buttonslayout->addWidget(button_savetojson);
  dlg_buttonslayout->addStretch();
  dlg_buttonslayout->addWidget(button_cancel);
  dlg_buttonslayout->addWidget(button_save);

  std::cout << "11" << std::endl;

  //device_manager_widget = device_manager->createDeviceSelectionWidget();
  //std::cout << "12" << std::endl;
  //settings_manager_container_widget->addTab(device_manager_widget, QString::fromStdString("Device"));
  
  std::cout << "13" << std::endl;
  settings_manager_container_widget->addTab(settings_manager_widget, QString::fromStdString("Settings"));

  std::cout << "14" << std::endl;
  
  return settings_manager_container_widget;
}


void QLSettingsManager::saveJSONSettings() {

  // std::cout << "saveJSONSettings()" << std::endl;

  // translate the GUI into JSON again:
  json settings_json_updated(settings_json);
  std::filesystem::path settings_json_filepath_updated = std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectName() + ".updated.json");

  json power_estimation_json_updated(power_estimation_json);
  std::filesystem::path power_estimation_json_filepath_updated = std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectName() + "_power" + ".updated.json");


  // root of all the settings is the stackedWidget, which contains one 'page' 
  // (page is a QTabWidget) for each category, loop through it for the categories:
  int category_count = stackedWidget->count();
  for (int category_index = 0; category_index < category_count; category_index++) {
    QTabWidget* category_widget = (QTabWidget*)stackedWidget->widget(category_index);
    std::string categoryId = category_widget->property("settings_category_id").toString().toStdString();
    // std::cout << "> categoryId: " << categoryId << std::endl;

    // each subcategory is one 'tab' (which is a QWidget) of the corresponding QTabWidget, 
    // loop through it for the subcategories:
    int subcategory_count = category_widget->count();
    for (int subcategory_index = 0; subcategory_index < subcategory_count; subcategory_index++) {
      QWidget* subcategory_widget = category_widget->widget(subcategory_index);
      std::string subcategoryId = subcategory_widget->property("settings_subcategory_id").toString().toStdString();
      // std::cout << ">>    subcategoryId: " << subcategoryId << std::endl;

      // handle device selection separately:
      if(categoryId == "general" && subcategoryId == "device") {
        settings_json_updated[categoryId][subcategoryId]["family"]["default"] = device_manager->device_target.device_variant.family;
        settings_json_updated[categoryId][subcategoryId]["foundry"]["default"] = device_manager->device_target.device_variant.foundry;
        settings_json_updated[categoryId][subcategoryId]["node"]["default"] = device_manager->device_target.device_variant.node;
        settings_json_updated[categoryId][subcategoryId]["voltage_threshold"]["default"] = device_manager->device_target.device_variant.voltage_threshold;
        settings_json_updated[categoryId][subcategoryId]["p_v_t_corner"]["default"] = device_manager->device_target.device_variant.p_v_t_corner;
        settings_json_updated[categoryId][subcategoryId]["layout"]["default"] = device_manager->device_target.device_variant_layout.name;
      }
      else {

        // each 'setting' is one 'container' widget inside the 'tab', loop through the widgets,
        // for each 'setting'
        // each 'container' widget contains: a QLabel, and a type specific QWidget (checkbox, combo, etc.)
        QList<QWidget*> container_widget_list = subcategory_widget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);

        for (int container_index = 0; container_index < container_widget_list.count(); container_index++) {
          QWidget* container_widget = container_widget_list[container_index];

          // get the property to check what kind of widget it is and its name:
          std::string widgetId = container_widget->property("settings_json_id").toString().toStdString();
          std::string widgetType = container_widget->property("settings_json_value_widgetType").toString().toStdString();
          // std::cout << "\n>>>      widgetId: " << widgetId << std::endl;
          // std::cout << ">>>      widgetType: " << widgetType << std::endl;

          // currently, we use only string values in json, but it may be one of the others as well:
          // std::string value_string;
          // int value_int;
          // double value_double;

          if(widgetType == std::string("input")) {

            QLineEdit* input_widget = container_widget->findChild<QLineEdit*>(QString(), Qt::FindChildrenRecursively);
            if(input_widget) {
              std::string value_string = (input_widget->text()).toStdString();
              // std::cout << ">>>      input_widget value: " << value_string << std::endl;

              if(categoryId == "power") {
                power_estimation_json_updated[categoryId][subcategoryId][widgetId]["default"] = value_string;
              }
              else {
                settings_json_updated[categoryId][subcategoryId][widgetId]["default"] = value_string;
              }
            }

          }
          else if(widgetType == std::string("checkbox")) {
            QCheckBox* checkbox_widget = container_widget->findChild<QCheckBox*>(QString(), Qt::FindChildrenRecursively);
            if(checkbox_widget) {
              std::string value_string = checkbox_widget->isChecked() ? "checked" : "unchecked";
              // std::cout << ">>>      checkbox_widget value: " << value_string << std::endl;
              if(categoryId == "power") {
                power_estimation_json_updated[categoryId][subcategoryId][widgetId]["default"] = value_string;
              }
              else {
                settings_json_updated[categoryId][subcategoryId][widgetId]["default"] = value_string;
              }
            }

          }
          else if(widgetType == std::string("dropdown")) {
            QComboBox* dropdown_widget = container_widget->findChild<QComboBox*>(QString(), Qt::FindChildrenRecursively);
            if(dropdown_widget) {
              std::string value_string = (dropdown_widget->currentText()).toStdString();
              // std::cout << ">>>      dropdown_widget value: " << value_string << std::endl;
              if(categoryId == "power") {
                power_estimation_json_updated[categoryId][subcategoryId][widgetId]["default"] = value_string;
              }
              else {
                settings_json_updated[categoryId][subcategoryId][widgetId]["default"] = value_string;
              }
            }
          }
          else {
            // unhandled widgetType!
            std::cout << ">>>      warning: unhandled widgetType: " << widgetType << std::endl;
          }
        }
      }

    }

  }

  // compare with original settings json, and if there are differences, we need to initiate
  // user confirmation, and then update and save json, replacing the original.

  json settings_json_patch = json::diff(settings_json, settings_json_updated);
  json power_estimation_json_patch = json::diff(power_estimation_json, power_estimation_json_updated);

  // std::cout << "settings_json_patch" << std::endl;
  // std::cout << std::setw(4) << settings_json_patch << std::endl;

  // std::cout << "power_estimation_json_patch" << std::endl;
  // std::cout << std::setw(4) << power_estimation_json_patch << std::endl;


  // the patch is a json array of "diffs", each diff being:
  // {
  //     "op": "replace",
  //     "path": "/power/power_outputs/debug/default", -> path to the item in the json
  //     "value": "unchecked" -> value changed
  // }
  std::vector<std::string> settings_json_change_list;
  std::vector<std::string> power_estimation_json_change_list;

  for (auto diff_element: settings_json_patch) {
    std::string path = diff_element["path"];
    std::vector<std::string> tokens;
    StringUtils::tokenize(path, "/", tokens);

    std::string original_value = settings_json[tokens[0]][tokens[1]][tokens[2]][tokens[3]].get<std::string>();
    std::string new_value = diff_element["value"];

    std::ostringstream json_change_stringstream;
    json_change_stringstream << tokens[0] << " > " << tokens[1] << " > " << tokens[2] << "\n";
    json_change_stringstream << "  from: " << original_value << "\n";
    json_change_stringstream << "  to:   " << new_value;

    settings_json_change_list.push_back(json_change_stringstream.str());
  }

  for (auto diff_element: power_estimation_json_patch) {
    std::string path = diff_element["path"];
    std::vector<std::string> tokens;
    StringUtils::tokenize(path, "/", tokens);

    std::string original_value = power_estimation_json[tokens[0]][tokens[1]][tokens[2]][tokens[3]].get<std::string>();
    std::string new_value = diff_element["value"];

    std::ostringstream json_change_stringstream;
    json_change_stringstream << tokens[0] << " > " << tokens[1] << " > " << tokens[2] << "\n";
    json_change_stringstream << "  from: " << original_value << "\n";
    json_change_stringstream << "  to:   " << new_value << "\n";

    power_estimation_json_change_list.push_back(json_change_stringstream.str());
  }

  // std::cout << "--------\n" << std::endl;
  // for(std::string change: settings_json_change_list) {
  //   std::cout << change << std::endl;
  //   std::cout << "--------\n" << std::endl;
  // }
  // for(std::string change: power_estimation_json_change_list) {
  //   std::cout << change << std::endl;
  //   std::cout << "--------\n" << std::endl;
  // }

  QDialog dialog;
  dialog.setWindowTitle("Settings Changes!");
  QVBoxLayout* dialogLayout = new QVBoxLayout();
  dialog.setLayout(dialogLayout);

  QListWidget* listOfChangesWidget = new QListWidget();
  listOfChangesWidget->setAlternatingRowColors(true);

  for(std::string change_item: settings_json_change_list) {
    listOfChangesWidget->addItem(QString::fromStdString(change_item));
  }
  
  for(std::string change_item: power_estimation_json_change_list) {
    listOfChangesWidget->addItem(QString::fromStdString(change_item));
  }

  QLabel* dialogLabel = new QLabel("\nPress OK to save the above changes into the Settings JSON\n");

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                  Qt::Horizontal);
  QObject::connect(buttons, &QDialogButtonBox::accepted,
                    &dialog, &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected,
                    &dialog, &QDialog::reject);

  dialogLayout->addWidget(listOfChangesWidget);
  dialogLayout->addStretch();
  dialogLayout->addWidget(dialogLabel);
  dialogLayout->addWidget(buttons);
  
  dialog.setModal(true);

  int result = dialog.exec();
  
  if (result == QDialog::Accepted)
  {
      // std::cout << "QDialog::Accepted" << std::endl;

      // std::ofstream settings_json_updated_ofstream(settings_json_filepath_updated.string());
      // settings_json_updated_ofstream << std::setw(4) << settings_json_updated << std::endl;

      // std::ofstream power_estimation_json_updated_ofstream(power_estimation_json_filepath_updated.string());
      // power_estimation_json_updated_ofstream << std::setw(4) << power_estimation_json_updated << std::endl;

      settings_json = settings_json_updated;
      std::ofstream settings_json_ofstream(settings_json_filepath.string());
      settings_json_ofstream << std::setw(4) << settings_json << std::endl;

      power_estimation_json = power_estimation_json_updated;
      std::ofstream power_estimation_json_ofstream(power_estimation_json_filepath.string());
      power_estimation_json_ofstream << std::setw(4) << power_estimation_json << std::endl;

  }
  else if (result == QDialog::Rejected)
  {
      // Do something on Cancel button click
      // std::cout << "QDialog::Rejected" << std::endl;
  }


}


void QLSettingsManager::parseJSONSettings() {

  std::string settings_json_filename = GlobalSession->GetCompiler()->ProjManager()->projectName() + ".json";
  settings_json_filepath = std::filesystem::path(settings_json_filename);
  std::cout << "settings_json_filepath: " << settings_json_filepath.string() << std::endl;
  if(FileUtils::FileExists(settings_json_filepath)) {
    std::ifstream settings_json_ifstream(settings_json_filepath.string());
    settings_json = json::parse(settings_json_ifstream);
  }
  else {}


  bool power_estimation_json_exists = false;

  power_estimation_json_filepath =
      std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectPath())/std::string(GlobalSession->GetCompiler()->ProjManager()->projectName() + "_power.json");
  if (FileUtils::FileExists(power_estimation_json_filepath)) {
      power_estimation_json_exists = true;
  }
  else {
      power_estimation_json_filepath = 
      std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectPath())/std::filesystem::path("..")/std::string(GlobalSession->GetCompiler()->ProjManager()->projectName() + "_power.json");
      if (FileUtils::FileExists(power_estimation_json_filepath)) {
        power_estimation_json_exists = true;
      }
  }

  if(power_estimation_json_exists == true) {
      std::ifstream power_estimation_json_f(power_estimation_json_filepath.string());
      power_estimation_json = json::parse(power_estimation_json_f);
  }
}


void QLSettingsManager::handleSaveButtonClicked() {
  saveJSONSettings();
}

} // namespace FOEDAG