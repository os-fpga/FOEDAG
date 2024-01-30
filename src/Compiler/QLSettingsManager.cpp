#include "QLSettingsManager.h"

#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QLayout>
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

  if(instance == NULL) {

    instance = new QLSettingsManager();
  }

  if(instance->device_manager == nullptr) {

    instance->device_manager = QLDeviceManager::getInstance(true);
    instance->device_manager->settings_manager = instance;
  }

  instance->parseJSONSettings();

  if( !((instance->settings_json).empty()) ) {

    std::string family              = getStringValue("general", "device", "family");
    std::string foundry             = getStringValue("general", "device", "foundry");
    std::string node                = getStringValue("general", "device", "node");
    std::string voltage_threshold   = getStringValue("general", "device", "voltage_threshold");
    std::string p_v_t_corner        = getStringValue("general", "device", "p_v_t_corner");
    std::string layout              = getStringValue("general", "device", "layout");
    
    // std::cout << "\n" << "---------------------------------------------" << std::endl;
    // std::cout << "current device_target from settings json:" << std::endl;
    // std::cout << " >> [family]              " << family << std::endl;
    // std::cout << " >> [foundry]             " << foundry << std::endl;
    // std::cout << " >> [node]                " << node << std::endl;
    // std::cout << " >> [voltage_threshold]   " << voltage_threshold << std::endl;
    // std::cout << " >> [p_v_t_corner]        " << p_v_t_corner << std::endl;
    // std::cout << " >> [layout]              " << layout << std::endl;
    // std::cout << "---------------------------------------------\n" << std::endl;

    instance->device_manager->setCurrentDeviceTarget(family, foundry, node, voltage_threshold, p_v_t_corner, layout);
  }

  return instance;
}


QLSettingsManager::QLSettingsManager(QObject *parent)
    : QObject(parent) {}

QLSettingsManager::~QLSettingsManager() {
  if(settings_manager_widget != nullptr) {
    settings_manager_widget->deleteLater();
  }
}


void QLSettingsManager::reloadJSONSettings() {

  instance->parseJSONSettings();
}


void QLSettingsManager::parseSDCFilePath() {

  if( (instance->settings_json).empty() ) {
    // settings json is not parsed yet, nothing to do.
    return;
  }

  // ---------------------------------------------------------------- sdc_file ++
  // sdc_file can come from Settings JSON -or- automatically picked up if named: <project_name>.sdc
  // sdc_file path can be absolute or relative
  // >> note: if sdc file specified in the Settings, and not found, then we flag this as an error!
  // if relative path, heuristic to find the sdc_file:
  //   1. check project_path, to see if sdc_file exists, use that
  //   2. check tcl_script_dir_path (if driven by TCL script), to see if sdc_file exists, use that
  //   3. check current dir, to see if sdc_file exists exists, use that

  std::filesystem::path sdc_file_path;
  instance->sdc_file_path_from_json = false;

  // 1. check if an sdc file is specified in the json:
  if( !getStringValue("vpr", "filename", "sdc_file").empty() ) {

    sdc_file_path = 
        std::filesystem::path(getStringValue("vpr", "filename", "sdc_file"));

    instance->sdc_file_path_from_json = true;
  }
  // else, check for an sdc file with the naming convention (<project_name>.sdc)
  // note that, this path is always a relative path.
  else {

    sdc_file_path = 
      std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectName() + std::string(".sdc"));
  }

  // check if the path specified is absolute:
  if (sdc_file_path.is_absolute()) {
    // check if the file exists:
    if (!FileUtils::FileExists(sdc_file_path)) {
      // currently, we ignore it, if the sdc file path is not found.
      sdc_file_path.clear();
    }
  }
  // we have a relative path
  else {
    std::filesystem::path sdc_file_path_absolute;
    
    // 1. check project_path
    // 2. check tcl_script_dir_path (if driven by TCL script)
    // 3. check current_dir_path

    std::filesystem::path project_path = std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectPath());
    sdc_file_path_absolute = project_path / sdc_file_path;
    if(!FileUtils::FileExists(sdc_file_path_absolute)) {
      sdc_file_path_absolute.clear();
    }

    // 2. check tcl_script_dir_path
    if(sdc_file_path_absolute.empty()) {
      std::filesystem::path tcl_script_dir_path = getTCLScriptDirPath();
      if(!tcl_script_dir_path.empty()) {
        sdc_file_path_absolute = tcl_script_dir_path / sdc_file_path;
        if(!FileUtils::FileExists(sdc_file_path_absolute)) {
          sdc_file_path_absolute.clear();
        }
      }
    }

    // 3. check current working dir path
    if(sdc_file_path_absolute.empty()) {
      sdc_file_path_absolute = sdc_file_path;
      if(!FileUtils::FileExists(sdc_file_path_absolute)) {
        sdc_file_path_absolute.clear();
      }
    }

    

    // final: check if we have a valid sdc file path:
    if(!sdc_file_path_absolute.empty()) {
      // assign the absolute path to the sdc_file_path variable:
      sdc_file_path = sdc_file_path_absolute;
    }
    else {
      // currently, we ignore it, if the sdc file path is not found.
      sdc_file_path.clear();
    }
  }
  // relative file path processing done.

  // if we have a valid sdc_file_path at this point, pass it on to vpr:
  if(!sdc_file_path.empty()) {
    // std::cout << "sdc file available: " << sdc_file_path << std::endl;
    instance->sdc_file_path = sdc_file_path;
  }
  else {
    //std::cout << "sdc file not available." << std::endl;
  }
}


std::string QLSettingsManager::getStringValue(std::string category, std::string subcategory, std::string parameter) {

  std::string value;

  json& json_ref = instance->combined_json;

  if( json_ref.contains(category) &&
      json_ref[category].contains(subcategory) &&
      json_ref[category][subcategory].contains(parameter) ) {

    if ( json_ref[category][subcategory][parameter].contains("userValue") ) {
      value = json_ref[category][subcategory][parameter]["userValue"].get<std::string>();
    }
    else if ( json_ref[category][subcategory][parameter].contains("default") ) {
      value = json_ref[category][subcategory][parameter]["default"].get<std::string>();
    }
    // else value is empty
  }

  return value;
}


long double QLSettingsManager::getLongDoubleValue(std::string category, std::string subcategory, std::string parameter) {

  long double value = 0;
  std::string str_value;

  json& json_ref = instance->combined_json;

  if( json_ref.contains(category) &&
      json_ref[category].contains(subcategory) &&
      json_ref[category][subcategory].contains(parameter) ) {

    if ( json_ref[category][subcategory][parameter].contains("userValue") ) {
      str_value = json_ref[category][subcategory][parameter]["userValue"].get<std::string>();
    }
    else if ( json_ref[category][subcategory][parameter].contains("default") ) {
      str_value = json_ref[category][subcategory][parameter]["default"].get<std::string>();
    }
    // else value is empty
  }

  if(!str_value.empty()) {

    try {
      value = std::stold(str_value);
    }
    catch (std::invalid_argument const &e) {
      std::cout << "[power json] Bad input: std::invalid_argument thrown" << std::endl;
      std::cout << "user entered: " << str_value << std::endl;
      value = 0;
    }
    catch (std::out_of_range const &e) {
      std::cout << "[power json] Integer overflow: std::out_of_range thrown" << std::endl;
      std::cout << "user entered: " << str_value << std::endl;
      value = 0;
    }
  }

  return value;
}


std::string QLSettingsManager::getStringToolTip(std::string category, std::string subcategory, std::string parameter) {

  std::string tooltip;

  json& json_ref = instance->combined_json;

  if( json_ref.contains(category) &&
      json_ref[category].contains(subcategory) &&
      json_ref[category][subcategory].contains(parameter) ) {

    if ( json_ref[category][subcategory][parameter].contains("tooltip") ) {
      tooltip = json_ref[category][subcategory][parameter]["tooltip"].get<std::string>();
    }
    else if ( json_ref[category][subcategory][parameter].contains("help") ) {
      tooltip = json_ref[category][subcategory][parameter]["help"].get<std::string>();
    }
    // else tooltip is empty
  }

  return tooltip;
}


const json* QLSettingsManager::getJson(std::string category, std::string subcategory, std::string parameter) {

  json& json_ref = instance->combined_json;
  if( json_ref.contains(category) &&
      json_ref[category].contains(subcategory) &&
      json_ref[category][subcategory].contains(parameter) &&
      !json_ref[category][subcategory][parameter].empty() ) {

    json* json_ptr = &(json_ref[category][subcategory][parameter]);
    return json_ptr;
  }

  return nullptr;
}


const json* QLSettingsManager::getJson(std::string category, std::string subcategory) {

  json& json_ref = instance->combined_json;
  if( json_ref.contains(category) &&
      json_ref[category].contains(subcategory) &&
      !json_ref[category][subcategory].empty() ) {

    json* json_ptr = &(json_ref[category][subcategory]);
    return json_ptr;
  }

  return nullptr;
}


const json* QLSettingsManager::getJson(std::string category) {

  json& json_ref = instance->combined_json;
  if( json_ref.contains(category) &&
      !json_ref[category].empty() ) {

    json* json_ptr = &(json_ref[category]);
    return json_ptr;
  }

  return nullptr;
}


std::filesystem::path QLSettingsManager::getSDCFilePath() {

  return instance->sdc_file_path;
}


std::filesystem::path QLSettingsManager::getTCLScriptDirPath() {

  std::filesystem::path tcl_script_dir_path;
  std::filesystem::path tcl_script_path = GlobalSession->CmdLine()->Script();

  std::error_code ec;
  if(!tcl_script_path.empty()) {

    std::filesystem::path tcl_script_path_c = std::filesystem::canonical(tcl_script_path, ec);
    if(!ec) {
      // path exists, and can be used
    }
    else {
      // no tcl script was used.
      tcl_script_path_c.clear();
    }
    
    // std::cout << "tcl_script_path()     : " << tcl_script_path_c << std::endl;
    
    if(!tcl_script_path_c.empty()) {
      tcl_script_dir_path = tcl_script_path_c.parent_path();
    }

  }
  else {
    // we were not executed via TCL script!
    // std::cout << "tcl_script_path()     : <none>" << std::endl;
  }

  return tcl_script_dir_path;
}


std::filesystem::path QLSettingsManager::getCurrentDirPath() {

  std::filesystem::path current_dir_path_c;

  std::error_code ec;

  current_dir_path_c = std::filesystem::canonical(".", ec);

  if(!ec) {
    // path exists, and can be used
  }
  else {
    // no tcl script was used.
    current_dir_path_c.clear();
  }

  // std::cout << "current_dir_path_c()     : " << current_dir_path_c.string() << std::endl;

  return current_dir_path_c;
}


QWidget* QLSettingsManager::createSettingsWidget(bool newProjectMode) {

  // whenever a new GUI Widget is created, we mark the mode of operation of this widget:
  this->newProjectMode = newProjectMode;

  // std::cout << "createSettingsWidget++, newProjectMode: " << newProjectMode << std::endl;

  if(settings_manager_widget != nullptr) {
    settings_manager_widget->deleteLater();
  }

  settings_manager_widget = new QWidget();
  settings_manager_widget->setWindowTitle("Task Settings");

  populateSettingsWidget();

  // std::cout << "createSettingsWidget--, newProjectMode: " << newProjectMode << std::endl;

  return settings_manager_widget;
}


void QLSettingsManager::updateSettingsWidget() {

  // std::cout << "updateSettingsWidget()++, newProjectMode: " << newProjectMode << std::endl;

  if(settings_manager_widget != nullptr) {

    // cleanup inside:
    qDeleteAll(settings_manager_widget->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly));
    QLayout* layout = settings_manager_widget->layout();
    delete layout;

    // repopulate:
    populateSettingsWidget();
  }

  // std::cout << "updateSettingsWidget()--, newProjectMode: " << newProjectMode << std::endl;
}


void QLSettingsManager::populateSettingsWidget() {

  // std::cout << "populateSettingsWidget()++, newProjectMode: " << newProjectMode << std::endl;

  QWidget* dlg = settings_manager_widget;

  dlg->setWindowTitle("Settings");

  QVBoxLayout* dlg_toplevellayout = new QVBoxLayout();
  dlg->setLayout(dlg_toplevellayout);

  QVBoxLayout* dlg_titleDetailLayout = new QVBoxLayout();
  QLabel* dlg_titleLabel = new QLabel("Task Settings");
  QFont dlg_titleLabelFont;
  dlg_titleLabelFont.setWeight(QFont::Bold);
  dlg_titleLabelFont.setStyleHint(QFont::SansSerif);
  dlg_titleLabelFont.setPointSize(12);
  dlg_titleLabel->setFont(dlg_titleLabelFont);
  dlg_titleLabel->setWordWrap(true);

  QLabel* dlg_detailLabel = new QLabel("Set the parameter values for all the Tasks (synth, pack, place ...)");
  dlg_detailLabel->setWordWrap(true);

  dlg_titleDetailLayout->addWidget(dlg_titleLabel);
  dlg_titleDetailLayout->addSpacing(15);
  dlg_titleDetailLayout->addWidget(dlg_detailLabel);

  // json structure to be followed:
  //  category0
  //    subcategory0_0
  //      param0_0_0 {}
  //      param0_0_1 {}
  //    subcategory0_1
  //      param0_1_0 {}
  //      param0_1_1 {}
  // category1
  //    subcategory1_0
  //      param1_0_0 {}
  //      param1_0_1 {}
  //    ... and so on.
  // each param *must* specify its properties as well:
  // TODO document.

  // 1. settings GUI representation, top-level -> Widget
  // 2. split layout into 
  //      (a) QListWidget (list of categories)
  //      (b) QStackedWidget (content of the categories)
  // 3. on selecting a category (QListWidget) -> corresponding widget which has the content of the category is visible
  // 4. each widget in the category widget (QStackedWidget) will be a QTabWidget
  //    each of the tabs in it represents a subcategory
  //    so, all the params in a subcategory will be displayed inside a 'tab' which is the 'subcategory widget'

  QHBoxLayout* dlg_widgetslayout = new QHBoxLayout();
  stackedWidget = new QStackedWidget();
  listWidget = new QListWidget();
  // https://stackoverflow.com/a/48336420/3379867
  listWidget->setStyleSheet(
            "QListWidget {\
                color: #FFFFFF;\
                outline: 0;\
            }\
            QListWidget::item {\
                padding: 4px;\
                min-height: 30px;\
                font-size: 20px;\
                color: #000000;\
            }\
            QListWidget::item:selected {\
                background-color: #2ABf9E;\
                color: #000000;\
            }");

  json rootJson;

  if(!newProjectMode) {

    // if this is an existing opened project, then read the settings from the project's json settings
    // which are parsed already

    rootJson = settings_json;
    if(!power_estimation_json.empty()) {
      rootJson.update(power_estimation_json);
    }
  }
  else {

    // this is a new project use case, so settings_json and power_estimation_json are empty!
    // there are also no actual json files for a project
    // when this function is called, with isExistingProject=false, the caller ensures that
    // the *future* json settings are populated in the settings_json_newproject and power_estimation_json_newproject
    
    rootJson = settings_json_newproject;
    if(!power_estimation_json_newproject.empty()) {
      rootJson.update(power_estimation_json_newproject);
    }
  }

  for (auto [categoryId, categoryJson] : rootJson.items()) {

    if(categoryId == "Tasks") {
      // we don't use this section in our use-cases.
      continue;
    }

    // if(categoryId == "general") {
    //   // we don't use this section here, it is represented by the QLDeviceManager.
    //   continue;
    // }

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

          if(widgetJson.contains("hidden") &&
             widgetJson["hidden"].get<std::string>() == "true") {
              // do not render this widget in the settings
              continue;
          }

          std::string widgetType = widgetJson["widgetType"].get<std::string>();
          
          // std::cout << "widgetId: " << QString::fromStdString(widgetId).toStdString() << std::endl;
          // std::cout << "widgetType: " << widgetType << std::endl;

          // finally, each parameter becomes a widget according the type and properties.
          QWidget* containerWidget = new QWidget();
          containerWidget->setObjectName("containerwidget");
          containerWidget->setStyleSheet("QWidget#containerwidget { background-color: #f0f0f0; }");
          QHBoxLayout* containerWidgetHBoxLayout = new QHBoxLayout();
          containerWidget->setLayout(containerWidgetHBoxLayout);
          containerWidget->setProperty("settings_json_id", QString::fromStdString(widgetId));
          containerWidget->setProperty("settings_json_value_widgetType", QString::fromStdString(widgetType));

          // add a tooltip if it exists:
          std::string tooltip = getStringToolTip(categoryId, subcategoryId, widgetId);
          if (!tooltip.empty()) {
            QString tooltip_qstring = QString("<font>");
            tooltip_qstring += QString::fromStdString(tooltip);
            tooltip_qstring += QString("</font>");
            containerWidget->setToolTip(tooltip_qstring);
          }

          QLabel* subWidgetLabel = new QLabel(QString::fromStdString(widgetId));
          QWidget* subWidget =
              FOEDAG::createWidget(widgetJson, QString::fromStdString(widgetId));

          containerWidgetHBoxLayout->addWidget(subWidgetLabel,1);
          //containerWidgetHBoxLayout->addStretch();
          containerWidgetHBoxLayout->addWidget(subWidget,1);

          // subscribe to changes on any of the widgets:
          if( widgetType == std::string("input") || widgetType == std::string("filepath") ) {

            QLineEdit* input_widget = containerWidget->findChild<QLineEdit*>(QString(), Qt::FindChildrenRecursively);
            if(input_widget) {

              QObject::connect(input_widget, &QLineEdit::textChanged, 
                              [=](){ this->handleSettingsChanged(); });
            }
          }
          else if( widgetType == std::string("checkbox") ) {
            
            QCheckBox* checkbox_widget = containerWidget->findChild<QCheckBox*>(QString(), Qt::FindChildrenRecursively);
            if(checkbox_widget) {

              QObject::connect(checkbox_widget, &QCheckBox::clicked, 
                              [=](){ this->handleSettingsChanged(); });
            }
          }
          else if( widgetType == std::string("dropdown") ) {

            QComboBox* dropdown_widget = containerWidget->findChild<QComboBox*>(QString(), Qt::FindChildrenRecursively);
            if(dropdown_widget) {

              // https://stackoverflow.com/questions/31164574/qt5-signal-slot-syntax-w-overloaded-signal-lambda
              QObject::connect(dropdown_widget, qOverload<int>(&QComboBox::currentIndexChanged), 
                              [=](){ this->handleSettingsChanged(); });
            }
          }
          else {
            // unhandled widgetType!
            std::cout << ">>> warning: unhandled widgetType: " << widgetType << " widgetId:" << widgetId << std::endl;
          }
          
          
          // the parameter 'container' widget is added into the 'subcategory widget' layout.
          subcategoryWidgetlayout->addWidget(containerWidget);
      }

      // if this subcategory in the json is empty, then we should not have a widget for this in the GUI
      // so, there won't be any empty tabs which represent this subcategory
      if(subcategoryJson.size() == 0) {
        subcategoryWidget->deleteLater();
        continue;
      }

      // subcategory widget ready -> this is a 'page' or 'tab' in QTabWidget, so add to the QTabWidget directly (no layout)
      categoryWidget->addTab(subcategoryWidget, QString::fromStdString(subcategoryId));
    }

    // if this category in the json is empty, then we should not have a widget for this in the GUI
    // so, there won't be any entry in the QListWidget, which will show empty QTabWidget when selected!
    if(categoryJson.size() == 0) {
      categoryWidget->deleteLater();
      // std::cout << "categoryId: " << categoryId << " empty!" << std::endl;
      continue;
    }
    
    // also, if due to internal logic (e.g. "general" > "device") if the categoryWidget has
    // 0 tabs, then, don't add it to the GUI as well:
    if(categoryWidget->count() == 0) {
      categoryWidget->deleteLater();
      // std::cout << "categoryId: " << categoryId << " empty!" << std::endl;
      continue;
    }

    // category widget is ready -> this is a 'page' in the 'container' QStackedWidget
    stackedWidget->addWidget(categoryWidget);

    // correspondingly, add the category 'name' into the QListWidget
    // custom qwidget, as the qlistwidgetitem's widget in this way:
    // QLabel* listItemWidgetLabel = new QLabel(QString::fromStdString(categoryId));
    // listItemWidgetLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    // listItemWidgetLabel->setLineWidth(1);
    // QListWidgetItem* listItem = new QListWidgetItem();
    // listItem->setSizeHint(listItemWidgetLabel->sizeHint());
    // listWidget->addItem(listItem);
    // listWidget->setItemWidget(listItem, listItemWidgetLabel);
    // simple qlistwidgetitem, this way:
    new QListWidgetItem(QString::fromStdString(categoryId), listWidget);
  }

  // when a 'category' in the QListView is selected, corresponding 'page' widget in the QStackedWidget should be shown.
  QObject::connect(listWidget, QOverload<int>::of(&QListWidget::currentRowChanged),
            stackedWidget, &QStackedWidget::setCurrentIndex);

  // container widget for all settings, add the QListView(left side), and then QStackedWidget(right side)
  dlg_widgetslayout->addWidget(listWidget);
  dlg_widgetslayout->addWidget(stackedWidget);

  listWidget->setCurrentRow(0);


  QHBoxLayout* dlg_buttonslayout = nullptr;
  if(!newProjectMode) {
    dlg_buttonslayout = new QHBoxLayout();
    // make the buttons for the actions in the settings dialog
    button_reset = new QPushButton("Reset");
    button_reset->setToolTip("Discard any modifications, reload from current settings JSON");
    button_reset->setDisabled(true);
    QObject::connect(button_reset, &QPushButton::released, this, &QLSettingsManager::handleResetButtonClicked);
    button_apply = new QPushButton("Apply");
    button_apply->setToolTip("Apply the modifications to the current settings JSON");
    button_apply->setDisabled(true);
    QObject::connect(button_apply, &QPushButton::released, this, &QLSettingsManager::handleApplyButtonClicked);

    dlg_buttonslayout->addStretch();
    dlg_buttonslayout->addWidget(button_reset);
    dlg_buttonslayout->addWidget(button_apply);

    m_message_label = new QLabel();
    m_message_label->setWordWrap(true);
    // background/font color
    m_message_label->setAutoFillBackground(true);
    //m_message_label->setStyleSheet("QLabel { background-color : red; color : blue; }");
    QPalette m_message_label_palette;
    m_message_label_palette.setColor(QPalette::Window, Qt::white);
    m_message_label_palette.setColor(QPalette::WindowText, Qt::red);
    m_message_label->setPalette(m_message_label_palette);
    // frame
    m_message_label->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_message_label->setText("");
    m_message_label->hide();
  }

  dlg_toplevellayout->addLayout(dlg_titleDetailLayout);
  dlg_toplevellayout->addLayout(dlg_widgetslayout); // first the settings stuff
  if(!newProjectMode) {
    dlg_toplevellayout->addWidget(m_message_label);
    dlg_toplevellayout->addLayout(dlg_buttonslayout); // second a row of buttons for actions
  }

  // std::cout << "populateSettingsWidget()--, newProjectMode: " << newProjectMode << std::endl;
  
}


void QLSettingsManager::updateJSONSettingsForDeviceTarget(QLDeviceTarget device_target) {

  // this is called from QLDeviceManager (GUI), when user changes/sets the device_target!
  // opened project -> change of device_target
  // new project -> set the device_target (no json file(s) yet!).

  // std::cout << "updateJSONSettingsForDeviceTarget()++, newProjectMode: " << newProjectMode << std::endl;

  std::string family;
  std::string foundry;
  std::string node;
  std::string voltage_threshold;
  std::string p_v_t_corner;
  std::string layout;

  std::string family_updated              = device_target.device_variant.family;
  std::string foundry_updated             = device_target.device_variant.foundry;
  std::string node_updated                = device_target.device_variant.node;
  std::string voltage_threshold_updated   = device_target.device_variant.voltage_threshold;
  std::string p_v_t_corner_updated        = device_target.device_variant.p_v_t_corner;
  std::string layout_updated              = device_target.device_variant_layout.name;

  if(newProjectMode) {
    // there is no JSON file existing (yet) - just update the settings_json_newproject object
    // current device : from the settings_json_newproject
    // new device : from device_target passed in

    // current_device:
    if(!settings_json_newproject.empty()) {
      family              = settings_json_newproject["general"]["device"]["family"]["userValue"];
      foundry             = settings_json_newproject["general"]["device"]["foundry"]["userValue"];
      node                = settings_json_newproject["general"]["device"]["node"]["userValue"];
      voltage_threshold   = settings_json_newproject["general"]["device"]["voltage_threshold"]["userValue"];
      p_v_t_corner        = settings_json_newproject["general"]["device"]["p_v_t_corner"]["userValue"];
      layout              = settings_json_newproject["general"]["device"]["layout"]["userValue"];
    }
  }
  else {
    // existing project mode
    // current device : from the settings json values
    // new device : from device_target passed in

    // current_device:
    family                = getStringValue("general", "device", "family");
    foundry               = getStringValue("general", "device", "foundry");
    node                  = getStringValue("general", "device", "node");
    voltage_threshold     = getStringValue("general", "device", "voltage_threshold");
    p_v_t_corner          = getStringValue("general", "device", "p_v_t_corner");
    layout                = getStringValue("general", "device", "layout");
  }

  // check changes, update the device accordingly, and if required 'reset' the settings from template

  // if the family/foundry/node are not changed, then we don't need to 'reset' the settings from the
  // template JSON, we only update the changed values and save the current JSON.
  if(family == family_updated &&
     foundry == foundry_updated &&
     node == node_updated) {

    if(newProjectMode) {
      
      if(!settings_json_newproject.empty()) {
        settings_json_newproject["general"]["device"]["family"]["userValue"] = family_updated;
        settings_json_newproject["general"]["device"]["foundry"]["userValue"] = foundry_updated;
        settings_json_newproject["general"]["device"]["node"]["userValue"] = node_updated;
        settings_json_newproject["general"]["device"]["voltage_threshold"]["userValue"] = voltage_threshold_updated;
        settings_json_newproject["general"]["device"]["p_v_t_corner"]["userValue"] = p_v_t_corner_updated;
        settings_json_newproject["general"]["device"]["layout"]["userValue"] = layout_updated;
      }

      // this call is not needed as nothing in the settings actually changed except device selection change.
      // updateSettingsWidget();
    }
    else {
      // update the device_target in the settings_json object:
      settings_json["general"]["device"]["family"]["userValue"] = family_updated;
      settings_json["general"]["device"]["foundry"]["userValue"] = foundry_updated;
      settings_json["general"]["device"]["node"]["userValue"] = node_updated;
      settings_json["general"]["device"]["voltage_threshold"]["userValue"] = voltage_threshold_updated;
      settings_json["general"]["device"]["p_v_t_corner"]["userValue"] = p_v_t_corner_updated;
      settings_json["general"]["device"]["layout"]["userValue"] = layout_updated;

      // save the updated settings_json with the target device info:
      std::ofstream settings_json_ofstream(settings_json_filepath.string());
      settings_json_ofstream << std::setw(4) << settings_json << std::endl;

      // this call is not needed as nothing in the settings actually changed except device selection change.
      //updateSettingsWidget();
    }

    return;
  }

  // if the device-type has changed (family/foundry/node) then the Settings may no longer be compatible
  // and we have to 'reset' the settings from the template JSON for the new device-type

  std::filesystem::path root_device_data_dir_path = 
      GlobalSession->Context()->DataPath();
  
  std::filesystem::path device_data_dir_path = root_device_data_dir_path / family_updated / foundry_updated / node_updated;

  std::filesystem::path settings_json_template_filepath = device_data_dir_path / "settings_template.json";

  std::filesystem::path power_json_template_filepath = device_data_dir_path / "power_template.json";

  if(newProjectMode) {

    // new project with specified device_target use-case

    // we cannot copy the json files, as we don't know where to copy/save them yet.
    // this can only be done once the project directory is created.

    // however: we can populate the settings GUI, using the template file,
    // and when the project creation is actually done by FOEDAG, then we anyway save the files:

    // read in the template json into the *future* _json_updated variables:
    if(FileUtils::FileExists(settings_json_template_filepath)) {
      std::ifstream settings_json_ifstream(settings_json_template_filepath.string());
      settings_json_newproject = json::parse(settings_json_ifstream);
    }
    else {
      settings_json_newproject = json::object();
    }

    if(FileUtils::FileExists(power_json_template_filepath)) {
        std::ifstream power_estimation_json_f(power_json_template_filepath.string());
        power_estimation_json_newproject = json::parse(power_estimation_json_f);
    }
    else {
      power_estimation_json_newproject = json::object();
    }

    // ensure that we keep the new device selected as the target device in the JSON object:
    if(!settings_json_newproject.empty()) {
      // update the device_target in the template json:
      settings_json_newproject["general"]["device"]["family"]["userValue"] = family_updated;
      settings_json_newproject["general"]["device"]["foundry"]["userValue"] = foundry_updated;
      settings_json_newproject["general"]["device"]["node"]["userValue"] = node_updated;
      settings_json_newproject["general"]["device"]["voltage_threshold"]["userValue"] = voltage_threshold_updated;
      settings_json_newproject["general"]["device"]["p_v_t_corner"]["userValue"] = p_v_t_corner_updated;
      settings_json_newproject["general"]["device"]["layout"]["userValue"] = layout_updated;
    }

    updateSettingsWidget();
  }
  else {

    // existing opened project -> change device_target scenario

    // copy and replace the json settings files according to the new device selected
    // and then update fresh template json with the new device selected

    std::error_code ec;

    std::filesystem::copy_file(settings_json_template_filepath,
                               settings_json_filepath,
                               std::filesystem::copy_options::overwrite_existing,
                               ec);
    if(ec) {
      // error
      std::cout << std::string("failed to copy: ") + settings_json_template_filepath.string() << std::endl;
      return;
    }

    std::filesystem::copy_file(power_json_template_filepath,
                               power_estimation_json_filepath,
                               std::filesystem::copy_options::overwrite_existing,
                               ec);
    if(ec) {
      // error
      std::cout << std::string("failed to copy: ") + power_json_template_filepath.string() << std::endl;
      return;
    }

    // re-parse the JSON Settings now
    parseJSONSettings();

    // update the device_target in the template json:
    settings_json["general"]["device"]["family"]["userValue"] = family_updated;
    settings_json["general"]["device"]["foundry"]["userValue"] = foundry_updated;
    settings_json["general"]["device"]["node"]["userValue"] = node_updated;
    settings_json["general"]["device"]["voltage_threshold"]["userValue"] = voltage_threshold_updated;
    settings_json["general"]["device"]["p_v_t_corner"]["userValue"] = p_v_t_corner_updated;
    settings_json["general"]["device"]["layout"]["userValue"] = layout_updated;

    // save the updated settings_json with the target device info:
    std::ofstream settings_json_ofstream(settings_json_filepath.string());
    settings_json_ofstream << std::setw(4) << settings_json << std::endl;

    // update *existing* GUI widget to reflect the new json data
    updateSettingsWidget();
  }

  // std::cout << "updateJSONSettingsForDeviceTarget()++, newProjectMode: " << newProjectMode << std::endl;
}


bool QLSettingsManager::areJSONSettingsChanged() {

  // std::cout << "areJSONSettingsChanged(), newProjectMode: " << newProjectMode << std::endl;

  if(newProjectMode) {

    // initialize to the "newproject" settings_json
    settings_json_updated = settings_json_newproject;

    // initialize to the "newproject" power_estimation_json
    power_estimation_json_updated = power_estimation_json_newproject;
  }
  else {

    // initialize to the current settings_json
    settings_json_updated = settings_json;

    // initialize to the current power_estimation_json
    power_estimation_json_updated = power_estimation_json;
  }

  // loop through the GUI elements, and check the updates below.
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
        settings_json_updated[categoryId][subcategoryId]["family"]["userValue"] = device_manager->device_target.device_variant.family;
        settings_json_updated[categoryId][subcategoryId]["foundry"]["userValue"] = device_manager->device_target.device_variant.foundry;
        settings_json_updated[categoryId][subcategoryId]["node"]["userValue"] = device_manager->device_target.device_variant.node;
        settings_json_updated[categoryId][subcategoryId]["voltage_threshold"]["userValue"] = device_manager->device_target.device_variant.voltage_threshold;
        settings_json_updated[categoryId][subcategoryId]["p_v_t_corner"]["userValue"] = device_manager->device_target.device_variant.p_v_t_corner;
        settings_json_updated[categoryId][subcategoryId]["layout"]["userValue"] = device_manager->device_target.device_variant_layout.name;
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

          if(widgetType == std::string("input") || widgetType == std::string("filepath")) {

            QLineEdit* input_widget = container_widget->findChild<QLineEdit*>(QString(), Qt::FindChildrenRecursively);
            if(input_widget) {
              std::string value_string = (input_widget->text()).toStdString();
              // std::cout << ">>>      input_widget value: " << value_string << std::endl;

              if(categoryId == "power") {
                power_estimation_json_updated[categoryId][subcategoryId][widgetId]["userValue"] = value_string;
              }
              else {
                settings_json_updated[categoryId][subcategoryId][widgetId]["userValue"] = value_string;
              }
            }
          }
          else if(widgetType == std::string("checkbox")) {

            QCheckBox* checkbox_widget = container_widget->findChild<QCheckBox*>(QString(), Qt::FindChildrenRecursively);
            if(checkbox_widget) {
              std::string value_string = checkbox_widget->isChecked() ? "checked" : "unchecked";
              // std::cout << ">>>      checkbox_widget value: " << value_string << std::endl;
              if(categoryId == "power") {
                power_estimation_json_updated[categoryId][subcategoryId][widgetId]["userValue"] = value_string;
              }
              else {
                settings_json_updated[categoryId][subcategoryId][widgetId]["userValue"] = value_string;
              }
            }
          }
          else if(widgetType == std::string("dropdown")) {

            QComboBox* dropdown_widget = container_widget->findChild<QComboBox*>(QString(), Qt::FindChildrenRecursively);
            if(dropdown_widget) {
              std::string value_string = (dropdown_widget->currentText()).toStdString();
              // std::cout << ">>>      dropdown_widget value: " << value_string << std::endl;
              if(categoryId == "power") {
                power_estimation_json_updated[categoryId][subcategoryId][widgetId]["userValue"] = value_string;
              }
              else {
                settings_json_updated[categoryId][subcategoryId][widgetId]["userValue"] = value_string;
              }
            }
          }
          else {
            // unhandled widgetType!
            std::cout << ">>> warning: unhandled widgetType: " << widgetType << " widgetId:" << widgetId << std::endl;
          }
        }
      }
    }
  }


  // for existing project only
  if(!newProjectMode) {

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

    // populate the list of changes:
    settings_json_change_list.clear();
    power_estimation_json_change_list.clear();

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

    // debug:
    // std::cout << "--------\n" << std::endl;
    // for(std::string change: settings_json_change_list) {
    //   std::cout << change << std::endl;
    //   std::cout << "--------\n" << std::endl;
    // }
    // for(std::string change: power_estimation_json_change_list) {
    //   std::cout << change << std::endl;
    //   std::cout << "--------\n" << std::endl;
    // }

    // no changes:
    if(settings_json_change_list.empty() && power_estimation_json_change_list.empty()) {
      return false;
    }
    else {
      return true;
    }
  }
  // for existing project only

  return true;
}


bool QLSettingsManager::saveJSONSettings() {

  // std::cout << "saveJSONSettings(), newProjectMode: " << newProjectMode << std::endl;

  bool savedNewJsonChanges = false;

  if(settings_manager_widget == nullptr) {

    // if we don't have a GUI, then there is nothing to save here.
    // settings_json_updated and power_estimation_json_updated will always be empty.
    // std::cout << "saveJSONSettings(), non-GUI mode, do nothing." << std::endl;
    return savedNewJsonChanges;
  }

  // For GUI mode of operation, scan through the settings widget, and ensure that
  // we are saving the changed settings, if any.
  // this will ensure we have the user updated settings from GUI in the xxxx_json_updated objects.
  bool userMadeChangesToSettingsJSON = false;
  userMadeChangesToSettingsJSON = areJSONSettingsChanged();

  // Futher, depending on the mode:
  // newProject -> save user changes without confirming with the user, because the settings came from a template anyway.
  // existingProject -> ask user for confirmation and then save to JSON, because the settings came from a project specific JSON


  // existing project only:
  // if !newProjectMode *and* there are changes from the user, prompt user to confirm the changes to be saved:
  int userDialogConfirmationResult = QDialog::Rejected;
  if(!newProjectMode && userMadeChangesToSettingsJSON) {

    // std::cout << "saveJSONSettings(), !newProjectMode && userMadeChangesToSettingsJSON" << std::endl;

    // for an existing project, check if there are any changes, and then update into the json files
    // replacing the current json files.
    // ask user to confirm the changes, before saving into JSON

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

      userDialogConfirmationResult = dialog.exec();
  }


  // if newProjectMode
  // -or-
  // if existingProjectMode *and* user has confirmed changes
  //   -> save the updated settings into the JSON filepath
  if( (newProjectMode) ||
      (!newProjectMode && userDialogConfirmationResult == QDialog::Accepted) ) {

    // std::cout << "saveJSONSettings(), going to save the new JSON!!" << std::endl;

    // if the settings json values are empty, ignore them.
    if(!settings_json_updated.empty()) {

      // std::cout << "saveJSONSettings(), !settings_json_updated.empty()" << std::endl;

      settings_json = settings_json_updated;
      std::ofstream settings_json_ofstream(settings_json_filepath.string());
      settings_json_ofstream << std::setw(4) << settings_json << std::endl;
    }
    else {
      // std::cout << "saveJSONSettings(), settings_json_updated: empty" << std::endl;
    }

    if(!power_estimation_json_updated.empty()) {

      // std::cout << "saveJSONSettings(), !power_estimation_json_updated.empty()" << std::endl;

      power_estimation_json = power_estimation_json_updated;
      std::ofstream power_estimation_json_ofstream(power_estimation_json_filepath.string());
      power_estimation_json_ofstream << std::setw(4) << power_estimation_json << std::endl;
    }
    else {
      // std::cout << "saveJSONSettings(), power_estimation_json_updated: empty" << std::endl;
    }

    savedNewJsonChanges = true;
  }
  else {

    // not saving updated JSON files:
    savedNewJsonChanges = false;
  }

  return savedNewJsonChanges;
}


void QLSettingsManager::parseJSONSettings() {

  std::filesystem::path project_path = std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectPath());

  if(project_path.empty()) {
    // project has not been initialized yet, do nothing:
    return;
  }

  // settings json filename follows the <project_name>
  std::string settings_json_filename = GlobalSession->GetCompiler()->ProjManager()->projectName() + ".json";


  // heuristic to find the settings json (same rules for the power_estimation json as well):
  // 1. check project_path, to see if <project_name>.json exists, use that
  // 2. check project_path/.., to see if <project_name>.json exists, use that
  //    - this is true when, project was created using TCL, but we are opening that via GUI
  //    - we know that, TCL script should be run only from its containing dir, 
  //      so the generated 'project_dir' will always be one level below it,
  //      and the project will use the JSON files from the TCL script dir, which is one level above (./..)
  // 3. check tcl_script_dir_path (if driven by TCL script), to see if <project_name>.json exists, use that
  // 4. check current dir, to see if <project_name>.json exists, use that

  // 1. check project_path
  settings_json_filepath = project_path / settings_json_filename;
  if(!FileUtils::FileExists(settings_json_filepath)) {
    settings_json_filepath.clear();
  }

  // 2. check project_path/..
  settings_json_filepath = project_path/ ".." / settings_json_filename;
  if(!FileUtils::FileExists(settings_json_filepath)) {
    settings_json_filepath.clear();
  }

  // 3. check tcl_script_dir_path
  if(settings_json_filepath.empty()) {
    std::filesystem::path tcl_script_dir_path = getTCLScriptDirPath();
    if(!tcl_script_dir_path.empty()) {
      settings_json_filepath = tcl_script_dir_path / settings_json_filename;
      if(!FileUtils::FileExists(settings_json_filepath)) {
        settings_json_filepath.clear();
      }
    }
  }

  // 4. check current working dir path
  if(settings_json_filepath.empty()) {
    settings_json_filepath = settings_json_filename;
    if(!FileUtils::FileExists(settings_json_filepath)) {
      settings_json_filepath.clear();
    }
  }

  // 4. convert to canonical path:
  // if(!settings_json_filepath.empty()) {
  //   std::error_code ec;
  //   settings_json_filepath = std::filesystem::canonical(settings_json_filepath, ec);
  //   if(!ec) settings_json_filepath.clear();
  // }

  // final: check we have a valid settings json:
  if(!settings_json_filepath.empty()) {
    try {
        // std::cout << "settings_json_filepath" << settings_json_filepath << std::endl;
        std::ifstream settings_json_ifstream(settings_json_filepath.string());
        settings_json = json::parse(settings_json_ifstream);
    }
    catch (json::parse_error& e) {
        // output exception information
        std::cout << "message: " << e.what() << '\n'
                  << "exception id: " << e.id << '\n'
                  << "byte position of error: " << e.byte << std::endl;
        settings_json = json::object();
    }
  }
  else {
    // we may reach this point (while creating a new project, for example) where it is valid.
    // ignore this condition.
  }



  // parse the power_estimation_json
  std::string power_estimation_json_filename = GlobalSession->GetCompiler()->ProjManager()->projectName() + "_power" + ".json";

  // 1. check project_path
  power_estimation_json_filepath = project_path / power_estimation_json_filename;
  if(!FileUtils::FileExists(power_estimation_json_filepath)) {
    power_estimation_json_filepath.clear();
  }

  // 2. check tcl_script_dir_path
  if(power_estimation_json_filepath.empty()) {
    std::filesystem::path tcl_script_dir_path = getTCLScriptDirPath();
    if(!tcl_script_dir_path.empty()) {
      power_estimation_json_filepath = tcl_script_dir_path / power_estimation_json_filename;
      if(!FileUtils::FileExists(power_estimation_json_filepath)) {
        power_estimation_json_filepath.clear();
      }
    }
  }

  // 3. check current working dir path
  if(power_estimation_json_filepath.empty()) {
    power_estimation_json_filepath = power_estimation_json_filename;
    if(!FileUtils::FileExists(power_estimation_json_filepath)) {
      power_estimation_json_filepath.clear();
    }
  }

  // 4. convert to canonical path:
  // if(!power_estimation_json_filepath.empty()) {
  //   std::error_code ec;
  //   power_estimation_json_filepath = std::filesystem::canonical(power_estimation_json_filepath, ec);
  //   if(!ec) power_estimation_json_filepath.clear();
  // }

  // final: check we have a valid settings json:
  if(!power_estimation_json_filepath.empty()) {
    try {
        // std::cout << "power_estimation_json_filepath" << power_estimation_json_filepath << std::endl;
        std::ifstream power_estimation_json_ifstream(power_estimation_json_filepath.string());
        power_estimation_json = json::parse(power_estimation_json_ifstream);
    }
    catch (json::parse_error& e) {
        // output exception information
        std::cout << "message: " << e.what() << '\n'
                  << "exception id: " << e.id << '\n'
                  << "byte position of error: " << e.byte << std::endl;
        power_estimation_json = json::object();
    }
  }
  else {
    // ignore power estimation, if not available.
    //std::cout << "[warning] no Power Estimation JSON file found to use!" << std::endl;
  }


  // combine both into single json for easy access internally (no merge):
  combined_json = settings_json;
  if(!power_estimation_json.empty()) {
    combined_json.update(power_estimation_json);
  }


  // parse SDC File Path:
  parseSDCFilePath();
}


void QLSettingsManager::handleApplyButtonClicked() {

  // existing project : on apply, show confirmation dialog
  // with all changes, if user confirms, save settings
  // into existing json and refresh the GUI using the new json.
  if(!newProjectMode) {
    bool savedNewJsonChanges = saveJSONSettings();
    if(savedNewJsonChanges) {
      updateSettingsWidget();
      emit settingsChanged();
    }
  }
  else {
    // nothing to do
  }
}


void QLSettingsManager::handleResetButtonClicked() {

  // existing pr
  if(!newProjectMode) {
    parseJSONSettings();
    updateSettingsWidget();
  }
  else {

  }
}

void QLSettingsManager::handleSettingsChanged() {

  // on any change to settings, this is called.
  // derive a diff, and if any changes, enable Reset and Apply buttons.
  // if no changes, disable Reset and Apply buttons.

  // existing project only
  if(!newProjectMode) {

    if(areJSONSettingsChanged()) {
      
      // enable the buttons as there are unsaved changes
      button_reset->setDisabled(false);
      button_apply->setDisabled(false);
      m_message_label->setText("Settings have unsaved changes!");
      // m_message_label->setText("Settings have changes!\n\n"
      //                          "- Click 'Apply' to save to JSON\n"
      //                          "- Click 'Reset' to reset to original\n\n"
      //                          "Task Settings changes will not be effective until 'Apply' is executed!");
      m_message_label->show();
    }
    else {
      
      // disable the buttons as there are no changes
      button_reset->setDisabled(true);
      button_apply->setDisabled(true);
      m_message_label->setText("");
      m_message_label->hide();
    }
  }
  // else, newProjectMode - do nothing: as we don't show changes over a template to user.
}

} // namespace FOEDAG