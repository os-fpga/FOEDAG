/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "WidgetFactory.h"

#include <QApplication>
#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QMetaEnum>
#include <QTextStream>
#include <QTreeWidget>
#include <iostream>

#include "Foedag.h"
#include "Settings.h"
#include "Tasks.h"

using namespace FOEDAG;
using nlohmann::json_pointer;

// This provides a way to listen for value changes in a form created by
// widgetFactory. Note: Only checkbox changes are reported currently
Q_GLOBAL_STATIC(WidgetFactoryDependencyNotifier, factoryNotifier)
WidgetFactoryDependencyNotifier* WidgetFactoryDependencyNotifier::Instance() {
  return factoryNotifier();
}

static tclArgFnMap TclArgFnLookup;

// This lookup provides tcl setters/getters based off a QString. When settings
// widgets are generated, these setters are loaded if the associated widget json
// defines a tclArgKey that will be used in this lookup.
//
// To link your settings json with this lookup, add:
// "_META_": {
//     "isSetting": true,
//     "tclArgKey": "YOUR_UNIQUE_KEY"
// }
// and then add "YOUR_UNIQUE_KEY" and your set/get callbacks below

void FOEDAG::initTclArgFns() {
  addTclArgFns("Tasks_Synthesis", {FOEDAG::TclArgs_setSynthesisOptions,
                                   FOEDAG::TclArgs_getSynthesisOptions});
  addTclArgFns("Tasks_placement", {FOEDAG::TclArgs_setPlacementOptions,
                                   FOEDAG::TclArgs_getPlacementOptions});
  addTclArgFns("Tasks_packing", {FOEDAG::TclArgs_setPackingOptions,
                                 FOEDAG::TclArgs_getPackingOptions});
  addTclArgFns("Tasks_Simulate_rtl", {FOEDAG::TclArgs_setSimulateOptions_rtl,
                                      FOEDAG::TclArgs_getSimulateOptions_rtl});
  addTclArgFns("Tasks_Simulate_gate",
               {FOEDAG::TclArgs_setSimulateOptions_gate,
                FOEDAG::TclArgs_getSimulateOptions_gate});
  addTclArgFns("Tasks_Simulate_pnr", {FOEDAG::TclArgs_setSimulateOptions_pnr,
                                      FOEDAG::TclArgs_getSimulateOptions_pnr});
  addTclArgFns("Tasks_Simulate_bitstream",
               {FOEDAG::TclArgs_setSimulateOptions_bitstream,
                FOEDAG::TclArgs_getSimulateOptions_bitstream});
  addTclArgFns("Tasks_TimingAnalysis",
               {FOEDAG::TclArgs_setTimingAnalysisOptions,
                FOEDAG::TclArgs_getTimingAnalysisOptions});
  addTclArgFns("TclExample", {FOEDAG::TclArgs_setExampleArgs,
                              FOEDAG::TclArgs_getExampleArgs});
}

// Clear out the default TclArgFns, this is provided for downstreams clients to
// reset the lookup if they need
void FOEDAG::clearTclArgFns() { TclArgFnLookup.clear(); }

void FOEDAG::addTclArgFns(const std::string& tclArgKey, tclArgFns argFns) {
  TclArgFnLookup.insert({tclArgKey, argFns});
}

// returns a pair of tcl setters/getters from the TclArgFnLookup
tclArgFns FOEDAG::getTclArgFns(const QString& tclArgKey) {
  tclArgGetterFn getter = nullptr;
  tclArgSetterFn setter = nullptr;
  tclArgFns retVal = {setter, getter};

  auto result = TclArgFnLookup.find(tclArgKey.toStdString());
  if (result != TclArgFnLookup.end()) {
    retVal = result->second;
  }

  return retVal;
}

QString getStr(const json& jsonObj, const QString& key,
               const QString& defaultStr = "") {
  std::string val = jsonObj.value(key.toStdString(), defaultStr.toStdString());
  return QString::fromStdString(val);
};

static constexpr uint JsonPathRole = Qt::UserRole + 1;

// Local debug helpers
#define WIDGET_FACTORY_DEBUG false
auto WIDGET_DBG_PRINT = [](std::string printStr) {
  if (WIDGET_FACTORY_DEBUG) {
    std::cout << printStr << std::flush;
  }
};
auto DBG_PRINT_VAL_SET = [](const QObject* obj, const QString& userVal) {
  std::string dbgStr = std::string(obj->metaObject()->className()) +
                       ": setting user value -> " + userVal.toStdString() +
                       "\n";
  WIDGET_DBG_PRINT(dbgStr);
};
auto DBG_PRINT_JSON_PATCH = [](const QObject* obj, const std::string& valStr) {
  if (WIDGET_FACTORY_DEBUG) {
    std::cout << "Setting json[\"jsonPatch\"] = " << valStr << " for "
              << obj->objectName().toStdString() << "("
              << obj->metaObject()->className() << ")" << std::endl;
  }
};

// Local helper function to convert a nlohmann::json array of std::strings to
// QStringLists This assumes the json object contains an array of strings
QStringList JsonArrayToQStringList(const json& jsonArray) {
  QStringList strings;
  std::transform(jsonArray.begin(), jsonArray.end(),
                 std::back_inserter(strings), [](json val) -> QString {
                   return QString::fromStdString(val.get<std::string>());
                 });
  return strings;
}

void storeJsonPatch(QObject* obj, const json& patch) {
  DBG_PRINT_JSON_PATCH(obj, patch.dump());
  obj->setProperty("saveNeeded", true);
  obj->setProperty("changed", true);
  obj->setProperty("jsonPatch", QString::fromStdString(patch.dump()));
}

void storeTclArg(QObject* obj, const QString& argStr) {
  obj->setProperty("tclArg", argStr);
  WIDGET_DBG_PRINT("handleChange - Storing Tcl Arg:  " + argStr.toStdString() +
                   "\n");
}

template <typename T>
T getDefault(const json& jsonObj) {
  T val{};
  if (jsonObj.contains("default")) {
    val = jsonObj["default"].get<T>();
  }
  return val;
}

QString convertSpaces(const QString& str) {
  QString temp = str;
  return temp.replace(" ", WF_SPACE);
}
QString restoreSpaces(const QString& str) {
  QString temp = str;
  return temp.replace(WF_SPACE, " ");
}
QString convertNewLines(const QString& str) {
  QString temp = str;
  return temp.replace("\n", WF_NEWLINE);
}
QString restoreNewLines(const QString& str) {
  QString temp = str;
  return temp.replace(WF_NEWLINE, "\n");
}
QString convertDashes(const QString& str) {
  QString temp = str;
  return temp.replace("-", WF_DASH);
}
QString restoreDashes(const QString& str) {
  QString temp = str;
  return temp.replace(WF_DASH, "-");
}

// Set Value overloads for diff widgets
void setVal(QLineEdit* ptr, const QString& userVal) {
  ptr->setText(userVal);
  DBG_PRINT_VAL_SET(ptr, userVal);
}
void setVal(QTextEdit* ptr, const QString& userVal) {
  ptr->setPlainText(userVal);
  DBG_PRINT_VAL_SET(ptr, userVal);
}
void setVal(QComboBox* ptr, const QString& userVal) {
  ptr->setCurrentText(userVal);
  DBG_PRINT_VAL_SET(ptr, userVal);
}
void setVal(QSpinBox* ptr, int userVal) {
  ptr->setValue(userVal);
  DBG_PRINT_VAL_SET(ptr, QString::number(userVal));
}
void setVal(QDoubleSpinBox* ptr, double userVal) {
  ptr->setValue(userVal);
  DBG_PRINT_VAL_SET(ptr, QString::number(userVal));
}
void setVal(QButtonGroup* ptr, const QString& userVal) {
  if (userVal != "<unset>") {
    for (auto btn : ptr->buttons()) {
      if (btn->text() == userVal) {
        btn->setChecked(true);
      }
    }
    DBG_PRINT_VAL_SET(ptr, userVal);
  }
}
void setVal(QCheckBox* ptr, Qt::CheckState userVal) {
  ptr->setCheckState(userVal);

  DBG_PRINT_VAL_SET(ptr,
                    QMetaEnum::fromType<Qt::CheckState>().valueToKey(userVal));
}

// Checks the child widgets of settingsParentWidget and returns whether or not
// any of their tracked values have changed
bool needsSave(QWidget* settingsParentWidget) {
  bool saveNeeded = false;
  for (int i = 0; i < settingsParentWidget->layout()->count(); i++) {
    QWidget* settingsWidget =
        settingsParentWidget->layout()->itemAt(i)->widget();
    if (settingsWidget) {
      QObject* targetObject =
          qvariant_cast<QObject*>(settingsWidget->property("targetObject"));

      if (targetObject && targetObject->property("saveNeeded").toBool()) {
        saveNeeded = true;
      }
    }
  }
  return saveNeeded;
}

int confirmChanges(QWidget* parent) {
  QMessageBox confirm(parent);
  confirm.setWindowTitle("Save Changes?");
  confirm.setIcon(QMessageBox::Question);
  confirm.setText(
      "Settings in this category have been modified and you're "
      "about to change categories.");
  confirm.setInformativeText("Do you want to save these changes first?");
  confirm.setStandardButtons(QMessageBox::Save | QMessageBox::Discard |
                             QMessageBox::Cancel);
  confirm.setDefaultButton(QMessageBox::Save);
  return confirm.exec();
}

QDialog* FOEDAG::createTopSettingsDialog(
    json& widgetsJson, const QString& selectedCategoryTitle /* "" */) {
  QDialog* dlg = new QDialog(GlobalSession->MainWindow());
  dlg->setObjectName("MainSettingsDialog");
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->setWindowTitle("Settings");

  QHBoxLayout* mainHLayout = new QHBoxLayout();
  dlg->setLayout(mainHLayout);

  // Category Listing
  QVBoxLayout* categoryVLayout = new QVBoxLayout();
  mainHLayout->addLayout(categoryVLayout);
  categoryVLayout->setContentsMargins(0, 0, 0, 0);
  QTreeWidget* categoryTree = new QTreeWidget();
  categoryVLayout->addWidget(categoryTree);
  categoryTree->setHeaderHidden(true);
  categoryTree->setColumnCount(1);
  categoryTree->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  categoryTree->header()->setStretchLastSection(false);
  categoryTree->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

  // Settings Pane
  QWidget* settingsContainer = new QWidget();
  QVBoxLayout* settingsVLayout = new QVBoxLayout();
  settingsVLayout->setContentsMargins(
      QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin), 0, 0,
      0);  // Preserve default left padding
  mainHLayout->addWidget(settingsContainer);
  settingsContainer->setObjectName("SettingsWidgetContainer");
  settingsContainer->setLayout(settingsVLayout);

  // Find and store paths to json objects that define settings categories
  QStringList jsonPaths;
  auto findCb = [&jsonPaths](json& obj, const QString& path) {
    // Check if the object has meta data and is a non-hidden setting category
    if (obj.contains("_META_")) {
      if (obj["_META_"].value("isSetting", false) == true &&
          obj["_META_"].value("hidden", true) == false) {
        jsonPaths << path;
      }
    }
  };
  Settings::traverseJson(widgetsJson, findCb);

  // Setup and fill category tree
  QList<QTreeWidgetItem*> treeItems;
  QTreeWidgetItem* newSelection = nullptr;
  for (auto& childPath : jsonPaths) {
    int separatorIdx = childPath.lastIndexOf("/");
    QString title = "unset";
    if (separatorIdx > -1) {
      // Get node name from full path
      title = childPath.mid(separatorIdx + 1);
    }
    QTreeWidgetItem* item =
        new QTreeWidgetItem(categoryTree, QStringList() << title);
    item->setData(0, JsonPathRole, childPath);
    treeItems.append(item);

    // If this node matches the requested category title, mark it to be
    // selected
    if (title == selectedCategoryTitle) {
      newSelection = item;
    }
  }
  categoryTree->addTopLevelItems(treeItems);

  // Add handling for category tree selection changes
  // This will confirm any unsaved changes for the current category and then
  // load the settings for the newly selected category
  QObject::connect(
      categoryTree, &QTreeWidget::itemSelectionChanged,
      [categoryTree, settingsVLayout, &widgetsJson, settingsContainer, dlg]() {
        auto loadSelectionSettings = [dlg, &widgetsJson, settingsVLayout](
                                         QTreeWidgetItem* selectedItem) {
          // Get a pointer to this item's associated json
          QString ptrPath = selectedItem->data(0, JsonPathRole).toString();
          json::json_pointer jsonPtr(ptrPath.toStdString());

          // Create and add settings pane for the new selection
          if (!jsonPtr.empty()) {
            QWidget* settingsWidget = FOEDAG::createSettingsPane(ptrPath);
            settingsWidget->layout()->setContentsMargins(0, 0, 0, 0);
            settingsVLayout->addWidget(settingsWidget);

            // Connect this dialog to the ok/cancel buttons of the child
            // settings widget
            if (settingsWidget) {
              QDialogButtonBox* btnBox =
                  settingsWidget->findChild<QDialogButtonBox*>(DlgBtnBoxName);
              if (btnBox) {
                QObject::connect(btnBox, &QDialogButtonBox::accepted, dlg,
                                 &QDialog::accept, Qt::UniqueConnection);
                QObject::connect(btnBox, &QDialogButtonBox::rejected, dlg,
                                 &QDialog::reject, Qt::UniqueConnection);
              }
            }
          }
        };

        auto selected = categoryTree->selectedItems();
        if (selected.count() > 0) {
          // Find any existing settings widgets
          QRegularExpression regex(".*" + QString(SETTINGS_WIDGET_SUFFIX));
          auto settingsWidgets =
              settingsContainer->findChildren<QWidget*>(regex);
          // If a settings widget already exists
          if (settingsWidgets.count()) {
            for (QWidget* currWidget :
                 settingsWidgets)  // only 1 value expected in all scenarios
            {
              // Check for and confirm changes before switching categories
              bool changed = needsSave(currWidget);
              int response =
                  QMessageBox::Discard;  // assume not changes happened
              if (changed) {
                // warn the user they are navigating with unsaved changes and
                // give and option to save/discard
                response = confirmChanges(dlg);
              }

              // Save
              if (response == QMessageBox::Save) {
                // Programatically click the Apply btn on the child settings
                // dlg
                if (auto* btnBox = currWidget->findChild<QDialogButtonBox*>()) {
                  btnBox->button(QDialogButtonBox::Apply)->click();
                }
              }

              // Replace current widget with new category settings widget if
              // user saved or discarded the changes
              if (response != QMessageBox::Cancel) {
                // remove old widget
                currWidget->deleteLater();
                // load new widget
                loadSelectionSettings(selected[0]);
              }

              // no-op on QDialogButtonBox::Cancel
            }
          } else {
            // This will only fire if the dialog opened w/ no category selected
            loadSelectionSettings(selected[0]);
          }
        }
      });

  // Select a requested category otherwise pick the first entry
  if (newSelection != nullptr) {
    categoryTree->setCurrentItem(newSelection);
  } else {
    categoryTree->setCurrentItem(categoryTree->topLevelItem(0));
  }

  return dlg;
}

QDialog* FOEDAG::createSettingsDialog(const QString& jsonPath,
                                      const QString& dialogTitle,
                                      const QString& objectNamePrefix /* "" */,
                                      const QString& tclArgs /* "" */) {
  QDialog* dlg = new QDialog();
  dlg->setObjectName(objectNamePrefix + "_SettingsDialog");
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->setWindowTitle(dialogTitle);
  QVBoxLayout* layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  dlg->setLayout(layout);

  FOEDAG::Settings* settings = GlobalSession->GetSettings();
  json::json_pointer jsonPtr(jsonPath.toStdString());

  QWidget* widget = nullptr;
  if (settings) {
    // Get widget parameters from json settings
    widget = FOEDAG::createSettingsPane(jsonPath);
  }

  if (widget) {
    layout->addWidget(widget);
    QDialogButtonBox* btnBox =
        widget->findChild<QDialogButtonBox*>(DlgBtnBoxName);
    if (btnBox) {
      QObject::connect(btnBox, &QDialogButtonBox::accepted, dlg,
                       &QDialog::accept);
      QObject::connect(btnBox, &QDialogButtonBox::rejected, dlg,
                       &QDialog::reject);
    }
  }

  return dlg;
}

// this will create a QWidget containing a widget generated by json pointed to
// in the settings system by jsonPath. A QDialogbutton box is included which can
// be introspected and tied into by higher level widgets that include this pane
QWidget* FOEDAG::createSettingsPane(const QString& jsonPath,
                                    tclArgSetterFn tclArgSetter /* nullptr */,
                                    tclArgGetterFn tclArgGetter /* nullptr */) {
  FOEDAG::Settings* settings = GlobalSession->GetSettings();
  json::json_pointer jsonPtr(jsonPath.toStdString());

  QWidget* widget = nullptr;
  if (settings) {
    QString settingName = "";
    QVBoxLayout* layout = nullptr;

    // Get widget parameters from json settings
    try {
      // nlohmann json doesn't support checking if a json ptr path is valid so
      // per the docs we need to try/catch these calls
      json& widgetsJson = settings->getJson().at(jsonPtr);

      // Get setting name from the jsonPath
      int separatorIdx = jsonPath.lastIndexOf("/");
      if (separatorIdx > -1) {
        // Get node name from full path
        settingName = jsonPath.mid(separatorIdx + 1);
      }

      // Get tcl getters/setters
      if (widgetsJson.contains("_META_")) {
        QString tclArgKey = QString::fromStdString(
            widgetsJson["_META_"].value("tclArgKey", ""));
        auto [setter, getter] = getTclArgFns(tclArgKey);
        if (tclArgSetter == nullptr) {
          tclArgSetter = setter;
        }
        if (tclArgGetter == nullptr) {
          tclArgGetter = getter;
        }
      }

      // Get any task settings that have been set via tcl commands
      QString tclArgs = "";
      if (tclArgGetter != nullptr) {
        tclArgs = QString::fromStdString(tclArgGetter());
      }

      // Create Settings Pane
      QWidget* containerWidget = new QWidget();
      layout = new QVBoxLayout();
      layout->setContentsMargins(0, 0, 0, 0);
      containerWidget->setLayout(layout);

      widget = FOEDAG::createSettingsWidget(widgetsJson, settingName, tclArgs);
    } catch (...) {
    }

    if (widget) {
      layout->addWidget(widget);
      // Find and attach to the created settings widget's QDialogButtonBox
      // This will check for changes in the json and save those
      QDialogButtonBox* btnBox =
          widget->findChild<QDialogButtonBox*>(DlgBtnBoxName);
      if (btnBox) {
        // Listen for clicks in the pane's QDialogButtonBox
        QObject::connect(
            btnBox, &QDialogButtonBox::clicked,
            [widget, settingName, jsonPtr, tclArgSetter, tclArgGetter, jsonPath,
             btnBox](QAbstractButton* button) {
              // If Save or Apply was clicked
              if (btnBox->buttonRole(button) == QDialogButtonBox::AcceptRole ||
                  btnBox->buttonRole(button) == QDialogButtonBox::ApplyRole) {
                if (widget) {
                  // Look up changed value json
                  QString patch = widget->property("userPatch").toString();
                  if (!patch.isEmpty()) {
                    // Create the parent json structure and add userPatch data
                    json cleanJson;
                    cleanJson[jsonPtr] = json::parse(
                        patch.toStdString());  // have to use [] to create the
                                               // entry, .at() gives a range
                                               // error

                    // Create user settings directory
                    QString userDir = Settings::getUserSettingsPath();
                    // A user setting dir only exists when a project has been
                    // loaded so ignore saving when there isn't a project
                    if (!userDir.isEmpty()) {
                      // Turn the json path into a unique(enough) settings file
                      // path to save the patch under
                      QString tempPath(jsonPath);
                      if (!tempPath.isEmpty() && tempPath[0] == '/') {
                        tempPath.remove(0, 1);
                      }
                      QFileInfo filepath(userDir + tempPath.replace("/", "_") +
                                         ".json");
                      QDir dir;
                      dir.mkpath(filepath.dir().path());

                      // Save settings for this specific Task category
                      QFile file(filepath.filePath());
                      if (file.open(QFile::WriteOnly)) {
                        QTextStream out(&file);
                        out << QString::fromStdString(cleanJson.dump());

                        WIDGET_DBG_PRINT(
                            "Saving Widget Settings: user values saved to " +
                            filepath.filePath().toStdString() + "\n\t" +
                            cleanJson.dump() + "\n");
                      }
                    } else {
                      WIDGET_DBG_PRINT(
                          "Saving Widget Settings: No user settings path, "
                          "skipping "
                          "save.\n");
                    }
                  }

                  // Set any tclArgList values for the given task
                  if (tclArgSetter != nullptr) {
                    QString tclArgs = widget->property("tclArgList").toString();
                    tclArgSetter(tclArgs.toStdString());
                  }
                }
              }
            });
      }
    }
  }

  return widget;
}

QWidget* FOEDAG::createSettingsWidget(json& widgetsJson,
                                      const QString& objNamePrefix /* "" */,
                                      const QString& tclArgs /* "" */) {
  // Create a parent widget to contain all generated widgets
  QWidget* widget = new QWidget();

  widget->setObjectName(objNamePrefix + SETTINGS_WIDGET_SUFFIX);
  QVBoxLayout* VLayout = new QVBoxLayout();
  widget->setLayout(VLayout);

  // Create a QFormLayout containing the requested fields
  QFormLayout* form = createWidgetFormLayout(widgetsJson, tclArgs.split("-"));
  VLayout->addLayout(form);
  VLayout->addStretch();

  // Add ok/cancel/apply buttons
  QDialogButtonBox* btnBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
                           QDialogButtonBox::Apply);
  btnBox->setObjectName(DlgBtnBoxName);
  VLayout->addWidget(btnBox);

  // This will collect value changes for each widget/setting, save those
  // settings to a user file and add the changes to the passed in settings
  // This will also build up and store a tcl arg list for these
  // settings if the widgets have "arg" fields defined
  auto checkVals = [form, &widgetsJson, widget]() {
    bool save = false;
    QHash<QString, QString> patchHash;
    QString argsStr = "";
    for (int i = 0; i < form->rowCount(); i++) {
      QWidget* settingsWidget =
          form->itemAt(i, QFormLayout::FieldRole)->widget();
      if (settingsWidget) {
        QObject* targetObject =
            qvariant_cast<QObject*>(settingsWidget->property("targetObject"));

        if (targetObject) {
          if (targetObject->property("changed").toBool()) {
            save = true;
            QString settingsId =
                settingsWidget->property("settingsId").toString();
            QString patchStr = targetObject->property("jsonPatch").toString();

            WIDGET_DBG_PRINT("createSettingsWidget: saving value " +
                             settingsId.toStdString() + " -> " +
                             patchStr.toStdString() + "\n");
            patchHash[settingsId] = patchStr;

            // Clear the "saveNeeded" flag once we've handled it
            // This is needed for the top level settings dialog which warns
            // the user on value changes this change will help cover the
            // scenario when a user clicks "apply" and then navigates away
            // which originally still left the widgets in a "saveNeeded"
            // state
            targetObject->setProperty("saveNeeded", {});
          }

          QString tclArg = targetObject->property("tclArg").toString();

          if (tclArg != "") {
            argsStr += " " + tclArg;
          }
        }
      }
    }

    // Step through each child patch and apply it to the original settings json
    QHashIterator<QString, QString> patch(patchHash);
    json changes;
    while (patch.hasNext()) {
      patch.next();

      if (!patch.value().isEmpty()) {
        std::string patchIdStr = patch.key().toStdString();
        std::string patchStr = patch.value().toStdString();

        // Store clean json changes
        changes[patchIdStr].merge_patch(json::parse(patchStr));

        // Store json changes in the passed in widgetsJson
        widgetsJson[patchIdStr].merge_patch(json::parse(patchStr));
      }
    }

    argsStr = argsStr.simplified();
    widget->setProperty("tclArgList", argsStr);
    WIDGET_DBG_PRINT("createSettingsWidget: storing tclArgList -> " +
                     argsStr.toStdString() + "\n");

    // If there were changes to save, store all the json changes in a property
    // that can be retrieved and saved by whomever called this
    if (save) {
      widget->setProperty("userPatch", QString::fromStdString(changes.dump()));
    }
  };

  auto handleBtns = [btnBox, checkVals](QAbstractButton* button) {
    if (btnBox->buttonRole(button) == QDialogButtonBox::ApplyRole ||
        btnBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
      checkVals();
    }
  };

  // Manually fire checkVals after system values have been loaded. This
  // will protect against the main settings widget making the user
  // confirm/save their changes anytime they load a category and then
  // switch categories. This originally occured because each loaded value
  // needs to be tracked so it gets marked as a change, but it's not a
  // user change we want to track during a confirm. checkVals has logic to
  // clear the unsaved property once it's captured the change in json so
  // calling checkVals captures the system set values while clearing the
  // unsaved state
  checkVals();

  // Check and store value changes if the dialog is accepted
  QObject::connect(btnBox, &QDialogButtonBox::clicked, widget, handleBtns);

  return widget;
}

QFormLayout* FOEDAG::createWidgetFormLayout(
    json& widgetsJson, const QStringList& tclArgList /* {} */) {
  QFormLayout* form = new QFormLayout();
  form->setLabelAlignment(Qt::AlignRight);

  // Create and add the child widget to the form layout
  for (auto [widgetId, widgetJson] : widgetsJson.items()) {
    QWidget* subWidget = FOEDAG::createWidget(
        widgetJson, QString::fromStdString(widgetId), tclArgList);
    QString label = getStr(widgetJson, "label");

    if (subWidget != nullptr) {
      form->addRow(label, subWidget);

      // If a tooltip was passed, set it for the widget and its label
      QString tooltip = getStr(widgetJson, "tooltip");
      if (!tooltip.isEmpty()) {
        subWidget->setToolTip(tooltip);
        form->labelForField(subWidget)->setToolTip(tooltip);
      }
    }
  }
  return form;
}

QWidget* FOEDAG::createWidget(const json& widgetJsonObj, const QString& objName,
                              const QStringList& args) {
  auto lookupStr = [](const QStringList& options, const QStringList& lookup,
                      const QString& option) -> QString {
    // Find the given option in the options array
    int idx = options.indexOf(option);
    QString value = option;
    if (idx > -1 && idx < lookup.count()) {
      // use the option index for a lookup if there are enough values
      value = lookup.at(idx);
    }
    return value;
  };

  // The requested widget or a container widget containing the requested widget
  QWidget* retVal = nullptr;

  // Ptr that will be stored in a property so the settings system knows where to
  // look when introspecting values. In cases like QRadioButton sets, this will
  // be a QObject instead of QWidget
  QObject* targetObject = nullptr;

  // The created widget that was requested if a label was requested, this will
  // be wrapped in a container widget
  // This ptr will usually match targetObject unless the createX function has
  // it's own container widget (like a collection of RadioButtons)
  QWidget* createdWidget = nullptr;

  // All widget types will be converted to and compared in lowercase to avoid
  // potential case issues in the json file
  QString type = getStr(widgetJsonObj, "widgetType").toLower();

  // Turn the arg list into a hash
  QHash<QString, QString> argPairs;
  for (auto argEntry : args) {
    QStringList tokens = argEntry.split(' ');
    QString argTag = tokens[0];

    // Remove leading -
    if (!argTag.isEmpty() && argTag[0] == '-') {
      argTag = argTag.remove(0, 1);
    }

    argPairs[argTag];  // implicitly create an entry in case the arg is a
                       // switch w/o a parameter

    if (tokens.count() > 1) {
      // store parameter if it was passed with the argument
      argPairs[argTag] = tokens[1];
    }
  }

  // See if this widget has an argument associated with it
  QString arg = getStr(widgetJsonObj, "arg");

  // Check if an arg associated with this widget was passed
  QString argVal;
  bool tclArgPassed = false;
  if (arg != "" && argPairs.contains(arg)) {
    tclArgPassed = true;
    argVal = argPairs[arg];
  }

  if (!type.isEmpty()) {
    // // Grab standard entries
    // QString objName = getStr(widgetJsonObj, "id");

    // Get widget type and create respective widget if it's supported
    if (type == "input" || type == "lineedit" || type == "filepath") {
      // QLineEdit - "input" or "lineedit"
      QString sysDefaultVal =
          QString::fromStdString(getDefault<std::string>(widgetJsonObj));

      // Callback to handle value changes
      std::function<void(QLineEdit*, const QString&)> handleChange =
          [arg](QLineEdit* ptr, const QString& val) {
            QString userVal = ptr->text();
            json changeJson;
            changeJson["userValue"] = userVal.toStdString();
            storeJsonPatch(ptr, changeJson);

            ptr->setProperty("tclArg", {});  // clear previous vals
            // store a tcl arg/value string if an arg was provided
            if (arg != "") {
              userVal = convertAll(userVal);
              QString argStr = "-" + arg + " " + userVal;
              storeTclArg(ptr, argStr);
            }
          };

      // Create our widget
      auto ptr = createLineEdit(objName, sysDefaultVal, handleChange);
      createdWidget = ptr;

      // Apply a validator if one was requested
      QString validator = getStr(widgetJsonObj, "validator", "").toLower();
      // if min/max are given, then the validator will set it's range as well
      const QString UNSET = "_unset_";
      QString minVal = getStr(widgetJsonObj, "validatorMin", UNSET);
      QString maxVal = getStr(widgetJsonObj, "validatorMax", UNSET);

      if (validator == "int") {
        auto val = new QIntValidator(ptr);
        if (minVal != UNSET) {
          val->setBottom(minVal.toInt());
        }
        if (maxVal != UNSET) {
          val->setTop(maxVal.toInt());
        }
        ptr->setValidator(val);
      } else if (validator == "double") {
        auto val = new QDoubleValidator(ptr);
        if (minVal != UNSET) {
          val->setBottom(minVal.toFloat());
        }
        if (maxVal != UNSET) {
          val->setTop(maxVal.toFloat());
        }
        ptr->setValidator(val);
      } else if (validator == "regex") {
        ptr->setValidator(new QRegExpValidator(ptr));
      }

      // Update field look based off validator results
      QObject::connect(ptr, &QLineEdit::textChanged, [ptr]() {
        QPalette palette;
        // assume property is valid until we find otherwise
        ptr->setProperty("invalid", {});
        // Change text to red if the input is invalid
        if (ptr->hasAcceptableInput()) {
          palette.setColor(QPalette::Text, Qt::black);
        } else {
          palette.setColor(QPalette::Text, Qt::red);
          // Mark field as invalid for downstream logic
          ptr->setProperty("invalid", true);
        }
        ptr->setPalette(palette);
      });

      if (tclArgPassed) {
        // convert any spaces to a replaceable tag so the arg is 1 token
        argVal = restoreAll(argVal);
        setVal(ptr, argVal);
      } else if (widgetJsonObj.contains("userValue")) {
        // Load and set user value
        QString userVal = QString::fromStdString(
            widgetJsonObj["userValue"].get<std::string>());
        setVal(ptr, userVal);

        DBG_PRINT_VAL_SET(ptr, userVal);
      }

      if (type == "filepath") {
        // Create a container widget so we can add a file browse btn
        QWidget* container = new QWidget();
        container->setObjectName(objName + "_container");
        // Create H layout with our original lineedit
        QHBoxLayout* layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        container->setLayout(layout);
        layout->addWidget(createdWidget);

        // Add a file browse button
        QToolButton* btn = new QToolButton();
        btn->setText("...");
        layout->addWidget(btn);

        // Launch a file dialog on click
        QObject::connect(btn, &QToolButton::pressed, [container, ptr]() {
          QFileDialog* dlg =
              new QFileDialog(container, "Select Path", ptr->text());
          dlg->setFileMode(QFileDialog::AnyFile);

          // We don't know if this field expects a file or directory and QT
          // doesn't provide an option for both so this will switch the dialog
          // type on the fly when a folder or file is selected
          QObject::connect(dlg, &QFileDialog::currentChanged,
                           [dlg](const QString& str) {
                             QFileInfo info(str);
                             if (info.isFile()) {
                               dlg->setFileMode(QFileDialog::ExistingFile);
                             } else if (info.isDir()) {
                               dlg->setFileMode(QFileDialog::Directory);
                             }
                           });

          // Update lineedit
          if (dlg->exec()) {
            QStringList fileNames = dlg->selectedFiles();
            if (fileNames.count()) {
              ptr->setText(fileNames[0]);
            }
          }
        });

        // filepath is a non-standard case, copy another type if looking for
        // an example to use
        targetObject = createdWidget;
        createdWidget = container;
      } else {
        targetObject = createdWidget;
      }
    } else if (type == "textedit" || type == "textbox") {
      // QTextEdit
      QString sysDefaultVal =
          QString::fromStdString(getDefault<std::string>(widgetJsonObj));

      // Callback to handle value changes
      std::function<void(QTextEdit*, const QString&)> handleChange =
          [arg](QTextEdit* ptr, const QString& val) {
            QString userVal = ptr->toPlainText();

            json changeJson;
            changeJson["userValue"] = userVal.toStdString();
            storeJsonPatch(ptr, changeJson);

            ptr->setProperty("tclArg", {});  // clear previous vals
            // store a tcl arg/value string if an arg was provided
            if (arg != "") {
              userVal = convertAll(userVal);
              QString argStr = "-" + arg + " " + userVal;
              storeTclArg(ptr, argStr);
            }
          };

      // Create our widget
      auto ptr = createTextEdit(objName, sysDefaultVal, handleChange);
      createdWidget = ptr;

      if (tclArgPassed) {
        // convert any spaces to a replaceable tag so the arg is 1 token
        argVal = restoreAll(argVal);
        setVal(ptr, argVal);
      } else if (widgetJsonObj.contains("userValue")) {
        // Load and set user value
        QString userVal = QString::fromStdString(
            widgetJsonObj["userValue"].get<std::string>());
        setVal(ptr, userVal);

        DBG_PRINT_VAL_SET(ptr, userVal);
      }

      targetObject = createdWidget;
    } else if (type == "dropdown" || type == "combobox") {
      // QComboBox - "dropdown" or "combobox"
      QString sysDefaultVal =
          QString::fromStdString(getDefault<std::string>(widgetJsonObj));

      QStringList comboOptions =
          JsonArrayToQStringList(widgetJsonObj.value("options", json::array()));
      QStringList comboLookup = JsonArrayToQStringList(
          widgetJsonObj.value("optionsLookup", json::array()));

      // Callback to handle value changes
      std::function<void(QComboBox*, const QString&)> handleChange =
          [arg, comboOptions, comboLookup, lookupStr](QComboBox* ptr,
                                                      const QString& val) {
            json changeJson;
            QString userVal = ptr->currentText();
            changeJson["userValue"] = userVal.toStdString();
            storeJsonPatch(ptr, changeJson);

            ptr->setProperty("tclArg", {});  // clear previous vals
            // store a tcl arg/value string if an arg was provided
            if (!arg.isEmpty()) {
              QString argStr = "-" + arg + " " +
                               lookupStr(comboOptions, comboLookup, userVal);
              storeTclArg(ptr, argStr);
            }
          };

      // Determine if this combobox should add <unset> option
      bool addUnset = widgetJsonObj.value("addUnset", addUnsetDefault);

      // Create Widget
      auto ptr = createComboBox(objName, comboOptions, sysDefaultVal, addUnset,
                                handleChange);
      createdWidget = ptr;

      if (tclArgPassed) {
        // Do a reverse lookup to convert the tcl value to a display value
        setVal(ptr, lookupStr(comboLookup, comboOptions, argVal));
      } else if (widgetJsonObj.contains("userValue")) {
        // Load and set user value
        QString userVal = QString::fromStdString(
            widgetJsonObj["userValue"].get<std::string>());
        setVal(ptr, userVal);
      }

      targetObject = createdWidget;
    } else if (type == "spinbox") {
      // QSpinBox - "spinbox"
      int minVal = widgetJsonObj.value("minVal", 0);
      int maxVal =
          widgetJsonObj.value("maxVal", std::numeric_limits<int>::max());
      int stepVal = widgetJsonObj.value("stepVal", 1);
      int sysDefaultVal = getDefault<int>(widgetJsonObj);

      // Callback to handle value changes
      std::function<void(QSpinBox*, const int&)> handleChange =
          [arg](QSpinBox* ptr, const int& val) {
            json changeJson;
            changeJson["userValue"] = ptr->value();
            storeJsonPatch(ptr, changeJson);

            ptr->setProperty("tclArg", {});  // clear previous vals
            // store a tcl arg/value string if an arg was provided
            if (arg != "") {
              QString argStr = "-" + arg + " " + QString::number(ptr->value());
              storeTclArg(ptr, argStr);
            }
          };

      // Create Widget
      auto ptr = createSpinBox(objName, minVal, maxVal, stepVal, sysDefaultVal,
                               handleChange);
      createdWidget = ptr;

      if (tclArgPassed) {
        bool valid = false;
        double argInt = argVal.toInt(&valid);
        if (valid) {
          setVal(ptr, argInt);
        }
      } else if (widgetJsonObj.contains("userValue")) {
        // Load and set user value
        int userVal = widgetJsonObj["userValue"].get<int>();
        setVal(ptr, userVal);
      }

      targetObject = createdWidget;
    } else if (type == "doublespinbox") {
      // QDoubleSpinBox - "doublespinbox"
      double minVal = widgetJsonObj.value("minVal", 0.0);
      double maxVal =
          widgetJsonObj.value("maxVal", std::numeric_limits<double>::max());
      double stepVal = widgetJsonObj.value("stepVal", 1.0);
      double sysDefaultVal = getDefault<double>(widgetJsonObj);

      // Callback to handle value changes
      std::function<void(QDoubleSpinBox*, const double&)> handleChange =
          [arg](QDoubleSpinBox* ptr, const double& val) {
            json changeJson;
            changeJson["userValue"] = ptr->value();
            storeJsonPatch(ptr, changeJson);

            ptr->setProperty("tclArg", {});  // clear previous vals
            // store a tcl arg/value string if an arg was provided
            if (arg != "") {
              QString argStr = "-" + arg + " " + QString::number(ptr->value());
              storeTclArg(ptr, argStr);
            }
          };

      // Create Widget
      auto ptr = createDoubleSpinBox(objName, minVal, maxVal, stepVal,
                                     sysDefaultVal, handleChange);
      createdWidget = ptr;

      if (tclArgPassed) {
        bool valid = false;
        double argDouble = argVal.toDouble(&valid);
        if (valid) {
          setVal(ptr, argDouble);
        }
      } else if (widgetJsonObj.contains("userValue")) {
        // Load and set user value
        double userVal = widgetJsonObj["userValue"].get<double>();
        setVal(ptr, userVal);
      }

      targetObject = createdWidget;
    } else if (type == "radiobuttons") {
      // QButtonGroup of QRadioButtons - "radiobuttons"
      QString sysDefaultVal =
          QString::fromStdString(getDefault<std::string>(widgetJsonObj));
      QStringList options =
          JsonArrayToQStringList(widgetJsonObj.value("options", json::array()));
      QStringList optionsLookup = JsonArrayToQStringList(
          widgetJsonObj.value("optionsLookup", json::array()));

      // Callback to handle value changes
      std::function<void(QRadioButton*, QButtonGroup*, const bool&)>
          handleChange = [arg, lookupStr, options, optionsLookup](
                             QRadioButton* btnPtr, QButtonGroup* btnGroup,
                             const bool& checked) {
            json changeJson;
            changeJson["userValue"] = btnPtr->text().toStdString();
            storeJsonPatch(btnGroup, changeJson);

            btnGroup->setProperty("tclArg", {});  // clear previous vals
            // store a tcl arg/value string if an arg was provided
            if (arg != "") {
              QString argStr =
                  "-" + arg + " " +
                  lookupStr(options, optionsLookup, btnPtr->text());
              btnGroup->setProperty("tclArg", argStr);
              WIDGET_DBG_PRINT("radiobutton handleChange - Storing Tcl Arg:  " +
                               argStr.toStdString() + "\n");
            }
          };

      // Create radiobuttons in a QButtonGroup
      QButtonGroup* btnGroup = FOEDAG::createRadioButtons(
          objName, options, sysDefaultVal, handleChange);

      // Get the current tcl or user value
      QString userVal = "<unset>";
      if (tclArgPassed) {
        // Do a reverse lookup to convert the tcl value to a display value
        userVal = lookupStr(optionsLookup, options, argVal);
      } else if (widgetJsonObj.contains("userValue")) {
        // Load and set user value
        userVal = QString::fromStdString(
            widgetJsonObj["userValue"].get<std::string>());
      }
      setVal(btnGroup, userVal);

      // ButtonGroups aren't real QWidgets so we need to add their child
      // radiobuttons to a container widget
      QWidget* container = new QWidget();
      container->setObjectName(objName + "_container");

      // Determine radiobutton layout direction
      QBoxLayout* containerLayout = nullptr;
      if ("horizontal" == getStr(widgetJsonObj, "layout")) {
        containerLayout = new QHBoxLayout();
      } else {
        containerLayout = new QVBoxLayout();
      }

      // Add radiobuttons to container widget's layout
      container->setLayout(containerLayout);
      for (auto* btn : btnGroup->buttons()) {
        containerLayout->addWidget(btn);
      }

      // RadioButtons is a non-standard case, copy another type if looking for
      // an example to use
      targetObject = btnGroup;
      createdWidget = container;
    } else if (type == "checkbox") {
      // QCheckBox - "checkbox"
      auto stringToCheckState = [](const QString& stateStr) -> Qt::CheckState {
        Qt::CheckState state = Qt::Unchecked;
        QString checkStateStr = stateStr.toLower();
        QStringList checkedStrs = {"1", "true", "checked"};
        QStringList uncheckedStrs = {"0", "false", "unchecked"};

        if (checkedStrs.contains(checkStateStr.toLower())) {
          state = Qt::Checked;
        } else if (uncheckedStrs.contains(checkStateStr.toLower())) {
          state = Qt::Unchecked;
        } else if (checkStateStr.toLower() == "partiallychecked") {
          state = Qt::PartiallyChecked;
        }
        return state;
      };

      // Determine Widget Details
      QString text = getStr(widgetJsonObj, "text");
      QString sysDefaultVal =
          QString::fromStdString(getDefault<std::string>(widgetJsonObj))
              .toLower();
      Qt::CheckState state = stringToCheckState(sysDefaultVal);

      // Callback to handle value changes
      std::function<void(QCheckBox*, const int&)> handleChange =
          [arg](QCheckBox* ptr, const int& val) {
            json changeJson;
            changeJson["userValue"] =
                QMetaEnum::fromType<Qt::CheckState>().valueToKey(val);
            storeJsonPatch(ptr, changeJson);

            ptr->setProperty("tclArg", {});  // clear previous vals
            // store a switch style tcl arg if this is checked
            if (arg != "" && ptr->checkState() == Qt::Checked) {
              ptr->setProperty("tclArg", "-" + arg);
              WIDGET_DBG_PRINT("checkbox handleChange - Storing Tcl Arg:  -" +
                               arg.toStdString() + "\n");
            }
            emit WidgetFactoryDependencyNotifier::Instance()->checkboxChanged(
                ptr->property("customId").toString(), ptr);
          };

      // Create Widget
      auto ptr = createCheckBox(objName, text, state, handleChange);
      createdWidget = ptr;

      if (tclArgPassed) {
        // usually just the boolean arg is passed w/o true/false, but
        // depending on the source, you might get a value as well
        // so we assume checked unless a value of 0 or false is seen
        if (argVal == "0" || argVal.toLower() == "false") {
          setVal(ptr, Qt::Unchecked);
        } else {
          setVal(ptr, Qt::Checked);
        }
      } else if (widgetJsonObj.contains("userValue")) {
        // Load and set user value
        QString userVal = QString::fromStdString(
            widgetJsonObj["userValue"].get<std::string>());
        setVal(ptr, stringToCheckState(userVal));
      }

      targetObject = createdWidget;
    } else {
      // TODO @skyler-rs nov2022 add error to logging
      // std::cout << "Type: " << type.toStdString() << " not recognized"
      //           << std::endl;
    }

    if (createdWidget) {
      // Only calling this for the container which will allow the widget to
      // stretch, label is no longer provided here as labels are now added via
      // QFormLayout at a different time
      retVal = FOEDAG::createContainerWidget(createdWidget);

      // Store a pointer to the primary widget incase we wrapped it in a
      // container widget with a label
      retVal->setProperty("targetObject", QVariant::fromValue(targetObject));
      // Store the ID so we know what value to update in the settings
      retVal->setProperty("settingsId", objName);

      if (targetObject) {
        // Allow caller to set a customId for their own uses after widget
        // creation
        QString id = getStr(widgetJsonObj, "customId", "_NO_CUSTOM_ID_SET_");
        targetObject->setProperty("customId", id);

        // Store dependency info in the widget properties for introspection
        // by other features
        auto deps = JsonArrayToQStringList(
            widgetJsonObj.value("bool_dependencies", json::array()));
        if (deps.count()) {
          // dependencies returns a list for future functionality, but for now
          // we are only checking the first bool as additional logic and
          // design choices are required to support multiple dependency fields
          targetObject->setProperty("bool_dependency", deps[0]);
        }
      }
    }
  }

  return retVal;
}

QWidget* FOEDAG::createWidget(const QString& widgetJsonStr,
                              const QString& objName, const QStringList& args) {
  return createWidget(json::parse(widgetJsonStr.toStdString()), objName, args);
}

QWidget* FOEDAG::createContainerWidget(QWidget* widget,
                                       const QString& label /* QString() */) {
  // Create a container widget w/ an H layout
  QWidget* retVal = new QWidget();
  QHBoxLayout* HLayout = new QHBoxLayout();
  HLayout->setContentsMargins(0, 0, 0, 0);
  retVal->setLayout(HLayout);

  // Add widget to a container and add a QLabel if label text was passed
  if (widget) {
    retVal->setObjectName(widget->objectName() + "_container");
    if (!label.isEmpty()) {
      HLayout->addWidget(new QLabel(label));
    }
    HLayout->addWidget(widget);
  }

  return retVal;
}

QComboBox* FOEDAG::createComboBox(
    const QString& objectName, const QStringList& options,
    const QString& selectedValue, bool addUnset,
    std::function<void(QComboBox*, const QString&)> onChange) {
  QComboBox* widget = new QComboBox();
  widget->setObjectName(objectName);
  widget->insertItems(0, options);
  if (addUnset) {
    widget->addItem("<unset>");
    widget->setCurrentText("<unset>");
  }
  widget->setCurrentText(selectedValue);

  if (onChange != nullptr) {
    // onChange needs the widget so we capture that in a closure we
    // can then pass to the normal qt handler
    std::function<void(const QString&)> changeCb =
        [onChange, widget](const QString& newText) {
          onChange(widget, newText);
        };
    QObject::connect(widget, &QComboBox::currentTextChanged, changeCb);
  }

  return widget;
}

QLineEdit* FOEDAG::createLineEdit(
    const QString& objectName, const QString& text,
    std::function<void(QLineEdit*, const QString&)> onChange) {
  QLineEdit* widget = new QLineEdit();
  widget->setObjectName(objectName);
  widget->setText(text);

  if (onChange != nullptr) {
    // onChange needs the widget so we capture that in a closure we
    // can then pass to the normal qt handler
    std::function<void(const QString&)> changeCb =
        [onChange, widget](const QString& newText) {
          onChange(widget, newText);
        };
    QObject::connect(widget, &QLineEdit::textChanged, changeCb);
  }

  return widget;
}

QTextEdit* FOEDAG::createTextEdit(
    const QString& objectName, const QString& text,
    std::function<void(QTextEdit*, const QString&)> onChange) {
  QTextEdit* widget = new QTextEdit();
  widget->setObjectName(objectName);
  widget->setPlainText(text);

  if (onChange != nullptr) {
    // onChange needs the widget so we capture that in a closure we
    // can then pass to the normal qt handler
    std::function<void()> changeCb = [onChange, widget]() {
      onChange(widget, widget->toPlainText());
    };
    QObject::connect(widget, &QTextEdit::textChanged, changeCb);
  }

  return widget;
}

QDoubleSpinBox* FOEDAG::createDoubleSpinBox(
    const QString& objectName, double minVal, double maxVal, double stepVal,
    double defaultVal,
    std::function<void(QDoubleSpinBox*, const double&)> onChange) {
  QDoubleSpinBox* widget = new QDoubleSpinBox();
  widget->setObjectName(objectName);

  widget->setMinimum(minVal);
  widget->setMaximum(maxVal);
  widget->setSingleStep(stepVal);
  widget->setValue(defaultVal);

  if (onChange != nullptr) {
    // onChange needs the widget so we capture that in a closure we
    // can then pass to the normal qt handler
    std::function<void(const double&)> changeCb =
        [onChange, widget](const double& newVal) { onChange(widget, newVal); };
    QObject::connect(widget, qOverload<double>(&QDoubleSpinBox::valueChanged),
                     changeCb);
  }

  return widget;
}

QSpinBox* FOEDAG::createSpinBox(
    const QString& objectName, int minVal, int maxVal, int stepVal,
    int defaultVal, std::function<void(QSpinBox*, const int&)> onChange) {
  QSpinBox* widget = new QSpinBox();
  widget->setObjectName(objectName);

  widget->setMinimum(minVal);
  widget->setMaximum(maxVal);
  widget->setSingleStep(stepVal);
  widget->setValue(defaultVal);

  if (onChange != nullptr) {
    // onChange needs the widget so we capture that in a closure we
    // can then pass to the normal qt handler
    std::function<void(const int&)> changeCb =
        [onChange, widget](const int& newVal) { onChange(widget, newVal); };
    QObject::connect(widget, qOverload<int>(&QSpinBox::valueChanged), changeCb);
  }

  return widget;
}

QButtonGroup* FOEDAG::createRadioButtons(
    const QString& objectName, const QStringList& nameList,
    const QString& selectedValue,
    std::function<void(QRadioButton*, QButtonGroup*, const bool&)> onChange) {
  QButtonGroup* widget = new QButtonGroup();
  widget->setObjectName(objectName);

  // Create QRadiobuttons and add to QButtonGroup
  for (const QString& name : nameList) {
    QRadioButton* radioBtn = new QRadioButton();
    radioBtn->setObjectName(objectName + "_" + name);

    radioBtn->setText(name);
    if (name == selectedValue) {
      radioBtn->setChecked(true);
    }

    if (onChange != nullptr) {
      // onChange needs the widget so we capture that in a closure we
      // can then pass to the normal qt handler
      std::function<void(const int&)> changeCb = [onChange, radioBtn,
                                                  widget](const bool& newVal) {
        onChange(radioBtn, widget, newVal);
      };
      QObject::connect(radioBtn, &QRadioButton::toggled, changeCb);
    }

    widget->addButton(radioBtn);
  }

  return widget;
}

QCheckBox* FOEDAG::createCheckBox(
    const QString& objectName, const QString& text, Qt::CheckState checked,
    std::function<void(QCheckBox*, const int&)> onChange) {
  QCheckBox* widget = new QCheckBox();
  widget->setObjectName(objectName);

  widget->setText(text);
  widget->setCheckState(checked);

  if (onChange != nullptr) {
    // onChange needs the widget so we capture that in a closure we
    // can then pass to the normal qt handler
    std::function<void(const int&)> changeCb =
        [onChange, widget](const int& newVal) { onChange(widget, newVal); };
    QObject::connect(widget, &QCheckBox::stateChanged, changeCb);
  };

  return widget;
}

// Searches a given layout for widgets created by widget factory
// these widgets have additional meta data stored in properties
// that can be used in dynamically generated UIs
QList<QObject*> FOEDAG::getTargetObjectsFromLayout(QLayout* layout) {
  QList<QObject*> settingsObjs;
  if (layout) {
    for (int i = 0; i < layout->count(); i++) {
      QWidget* settingsWidget = layout->itemAt(i)->widget();
      if (settingsWidget) {
        QObject* targetObject =
            qvariant_cast<QObject*>(settingsWidget->property("targetObject"));
        if (targetObject) {
          settingsObjs << targetObject;
        }
      }
    }
  }

  return settingsObjs;
}

QString FOEDAG::convertAll(const QString& str) {
  return convertSpaces(convertNewLines(convertDashes(str)));
}

QString FOEDAG::restoreAll(const QString& str) {
  return restoreSpaces(restoreNewLines(restoreDashes(str)));
}
