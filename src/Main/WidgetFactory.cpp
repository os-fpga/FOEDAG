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

#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMetaEnum>

using namespace FOEDAG;

const QString DlgBtnBoxName{"SettingsDialogButtonBox"};

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

template <typename T>
T getUserValOrDefault(const json& jsonObj) {
  T val = jsonObj["default"].get<T>();
  if (jsonObj.contains("userValue")) {
    val = jsonObj["userValue"].get<T>();
  }

  return val;
}

QDialog* FOEDAG::createSettingsDialog(
    json& widgetsJson, const QString& dialogTitle,
    const QString& objectNamePrefix /* "" */) {
  QDialog* dlg = new QDialog();
  dlg->setObjectName(objectNamePrefix + "_SettingsDialog");
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->setWindowTitle(dialogTitle);
  QVBoxLayout* layout = new QVBoxLayout();
  dlg->setLayout(layout);

  QWidget* task = FOEDAG::createSettingsWidget(widgetsJson, objectNamePrefix);
  if (task) {
    layout->addWidget(task);
    QDialogButtonBox* btnBox =
        task->findChild<QDialogButtonBox*>(DlgBtnBoxName);
    if (btnBox) {
      QObject::connect(btnBox, &QDialogButtonBox::accepted, dlg,
                       &QDialog::accept);
      QObject::connect(btnBox, &QDialogButtonBox::rejected, dlg,
                       &QDialog::reject);
    }
  }

  return dlg;
}

QWidget* FOEDAG::createSettingsWidget(json& widgetsJson,
                                      const QString& objNamePrefix /* "" */) {
  // Create a parent widget to contain all generated widgets
  QWidget* widget = new QWidget();

  widget->setObjectName(objNamePrefix + "SettingsWidget");
  QVBoxLayout* VLayout = new QVBoxLayout();
  widget->setLayout(VLayout);

  for (auto [widgetId, widgetJson] : widgetsJson.items()) {
    // Create and add the child widget to our parent container
    QWidget* subWidget =
        FOEDAG::createWidget(widgetJson, QString::fromStdString(widgetId));
    VLayout->addWidget(subWidget);
  }

  QDialogButtonBox* btnBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  btnBox->setObjectName(DlgBtnBoxName);
  VLayout->addWidget(btnBox);

  // This will collect value changes for each widget/setting, save those
  // settings to a user file and add the changes to the passed in settings
  auto checkVals = [VLayout, &widgetsJson]() {
    bool save = false;
    QHash<QString, QString> patchHash;
    for (int i = 0; i < VLayout->count(); i++) {
      QWidget* widget = VLayout->itemAt(i)->widget();
      if (widget) {
        QObject* targetObject =
            qvariant_cast<QObject*>(widget->property("targetObject"));

        if (targetObject && targetObject->property("changed").toBool()) {
          save = true;
          QString settingsId = widget->property("settingsId").toString();
          QString patchStr = targetObject->property("jsonPatch").toString();

          patchHash[settingsId] = patchStr;
        }
      }
    }

    // Step through each child patch and apply it to the original settings json
    QHashIterator<QString, QString> patch(patchHash);
    while (patch.hasNext()) {
      patch.next();

      if (!patch.value().isEmpty()) {
        std::string patchIdStr = patch.key().toStdString();
        std::string patchStr = patch.value().toStdString();

        widgetsJson[patchIdStr].merge_patch(json::parse(patchStr));
      }
    }

    if (save) {
      // sma save not yet implemented
      // settings->saveJsonFile("./saveTest.json");
    }
  };

  QObject::connect(btnBox, &QDialogButtonBox::accepted, widget, checkVals);

  return widget;
}

QWidget* FOEDAG::createWidget(const json& widgetJsonObj,
                              const QString& objName) {
  auto getStr = [](const json& jsonObj, const QString& key,
                   const QString& defaultStr = "") {
    std::string val =
        jsonObj.value(key.toStdString(), defaultStr.toStdString());
    return QString::fromStdString(val);
  };

  // The requested widget or a container widget containing the widget requested
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

  if (!type.isEmpty()) {
    // // Grab standard entries
    // QString objName = getStr(widgetJsonObj, "id");

    // Get widget type and create respective widget if it's supported
    if (type == "input" || type == "lineedit") {
      // QLineEdit - "input" or "lineedit"

      // QString defaultVal = getStr(widgetJsonObj, "default");
      QString defaultVal = QString::fromStdString(
          getUserValOrDefault<std::string>(widgetJsonObj));

      auto ptr = createLineEdit(objName, defaultVal);
      createdWidget = ptr;

      // Add value change tracking
      auto initialVal = ptr->text();
      QObject::connect(ptr, &QLineEdit::textChanged,
                       [widgetJsonObj, ptr, initialVal](const QString& val) {
                         bool changed = (val != initialVal);
                         ptr->setProperty("changed", changed);
                         if (changed) {
                           // store value changes as a json string in the
                           // widget's property system
                           json changeJson;
                           changeJson["userValue"] = ptr->text().toStdString();
                           ptr->setProperty(
                               "jsonPatch",
                               QString::fromStdString(changeJson.dump()));
                         }
                       });

      targetObject = createdWidget;
    } else if (type == "dropdown" || type == "combobox") {
      // QComboBox - "dropdown" or "combobox"
      QString defaultVal = QString::fromStdString(
          getUserValOrDefault<std::string>(widgetJsonObj));
      QStringList comboOptions =
          JsonArrayToQStringList(widgetJsonObj["options"]);
      auto ptr = createComboBox(objName, comboOptions, defaultVal);
      createdWidget = ptr;

      // Add value change tracking
      auto initialVal = ptr->currentIndex();
      QObject::connect(
          ptr, qOverload<int>(&QComboBox::currentIndexChanged),
          [ptr, initialVal](int val) {
            //  changeJson.emplace( "userValue", ptr->currentText() );
            bool changed = (val != initialVal);
            ptr->setProperty("changed", changed);
            if (changed) {
              // store value changes as a json string in the widget's property
              // system
              json changeJson;
              changeJson["userValue"] = ptr->currentText().toStdString();
              ptr->setProperty("jsonPatch",
                               QString::fromStdString(changeJson.dump()));
            }
          });

      targetObject = createdWidget;
    } else if (type == "spinbox") {
      // QSpinBox - "spinbox"
      int minVal = widgetJsonObj.value("minVal", 0);
      int maxVal =
          widgetJsonObj.value("maxVal", std::numeric_limits<int>::max());
      int stepVal = widgetJsonObj.value("stepVal", 1);
      // int defaultVal = widgetJsonObj.value("default", 0);
      int defaultVal = getUserValOrDefault<int>(widgetJsonObj);
      auto ptr = createSpinBox(objName, minVal, maxVal, stepVal, defaultVal);
      createdWidget = ptr;

      // Add value change tracking
      auto initialVal = ptr->value();
      QObject::connect(ptr, qOverload<int>(&QSpinBox::valueChanged),
                       [ptr, initialVal](int val) {
                         bool changed = (val != initialVal);
                         ptr->setProperty("changed", changed);
                         if (changed) {
                           // store value changes as a json string in the
                           // widget's property system
                           json changeJson;
                           changeJson["userValue"] = ptr->value();
                           ptr->setProperty(
                               "jsonPatch",
                               QString::fromStdString(changeJson.dump()));
                         }
                       });

      targetObject = createdWidget;
    } else if (type == "doublespinbox") {
      // QDoubleSpinBox - "doublespinbox"
      double minVal = widgetJsonObj.value("minVal", 0.0);
      double maxVal =
          widgetJsonObj.value("maxVal", std::numeric_limits<double>::max());
      double stepVal = widgetJsonObj.value("stepVal", 1.0);
      double defaultVal = getUserValOrDefault<double>(widgetJsonObj);

      auto ptr =
          createDoubleSpinBox(objName, minVal, maxVal, stepVal, defaultVal);
      createdWidget = ptr;

      // Add value change tracking
      auto initialVal = ptr->value();
      QObject::connect(ptr, qOverload<double>(&QDoubleSpinBox::valueChanged),
                       [ptr, initialVal](int val) {
                         bool changed = (val != initialVal);
                         ptr->setProperty("changed", changed);
                         if (changed) {
                           // store value changes as a json string in the
                           // widget's property system
                           json changeJson;
                           changeJson["userValue"] = ptr->value();
                           ptr->setProperty(
                               "jsonPatch",
                               QString::fromStdString(changeJson.dump()));
                         }
                       });

      targetObject = createdWidget;
    } else if (type == "radiobuttons") {
      // QButtonGroup of QRadioButtons - "radiobuttons"
      QString defaultVal = QString::fromStdString(
          getUserValOrDefault<std::string>(widgetJsonObj));
      QStringList options = JsonArrayToQStringList(widgetJsonObj["options"]);

      // Create radiobuttons in a QButtonGroup
      QButtonGroup* btnGroup =
          FOEDAG::createRadioButtons(objName, options, defaultVal);
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

      // record initial val for change tracking later
      auto initialVal = btnGroup->checkedId();

      // Add radiobuttons to container widget's layout
      container->setLayout(containerLayout);
      for (auto* btn : btnGroup->buttons()) {
        containerLayout->addWidget(btn);
        // Add value change tracking
        QObject::connect(
            btn, &QRadioButton::toggled, [btn, btnGroup, initialVal](bool val) {
              bool changed = (btnGroup->checkedId() != initialVal);
              btnGroup->setProperty("changed", changed);
              if (changed) {
                // store value changes as a json string in the widget's property
                // system
                json changeJson;
                changeJson["userValue"] = btn->text().toStdString();
                btnGroup->setProperty(
                    "jsonPatch", QString::fromStdString(changeJson.dump()));
              }
            });
      }

      // RadioButtons is a non-standard case, copy another type if looking for
      // an example to use
      targetObject = btnGroup;
      createdWidget = container;
    } else if (type == "checkbox") {
      // QCheckBox - "checkbox"
      QString text = getStr(widgetJsonObj, "text");

      // Determine checkstate
      QString defaultVal = QString::fromStdString(
                               getUserValOrDefault<std::string>(widgetJsonObj))
                               .toLower();
      Qt::CheckState state = Qt::Unchecked;
      if (defaultVal == "checked") {
        state = Qt::Checked;
      } else if (defaultVal == "partiallychecked") {
        state = Qt::PartiallyChecked;
      }

      // Add value change tracking
      auto ptr = createCheckBox(objName, text, state);
      createdWidget = ptr;
      auto initialVal = ptr->checkState();
      QObject::connect(
          ptr, &QCheckBox::stateChanged, [ptr, initialVal](int val) {
            bool changed = (val != initialVal);
            ptr->setProperty("changed", changed);
            if (changed) {
              // store value changes as a json string in the widget's property
              // system
              json changeJson;
              changeJson["userValue"] =
                  QMetaEnum::fromType<Qt::CheckState>().valueToKey(val);
              ptr->setProperty("jsonPatch",
                               QString::fromStdString(changeJson.dump()));
            }
          });

      targetObject = createdWidget;
    }

    if (createdWidget) {
      // Add a containing widget and label if "label" property was provided
      QString label = getStr(widgetJsonObj, "label");
      if (!label.isEmpty()) {
        retVal = FOEDAG::createLabelWidget(label, createdWidget);
      } else {
        retVal = createdWidget;
      }

      // Store a pointer to the primary widget incase we wrapped it in a
      // container widget with a label
      retVal->setProperty("targetObject", QVariant::fromValue(targetObject));
      // Store the ID so we know what value to update in the settings
      retVal->setProperty("settingsId", objName);
    }
  }

  return retVal;
}

QWidget* createWidget(const QString& widgetJsonStr,
                      const QString& objName = "") {
  return createWidget(json::parse(widgetJsonStr.toStdString()), objName);
}

QWidget* FOEDAG::createLabelWidget(const QString& label, QWidget* widget) {
  // Create a container widget w/ an H layout
  QWidget* retVal = new QWidget();
  QHBoxLayout* HLayout = new QHBoxLayout();
  retVal->setLayout(HLayout);

  // Add a label and our widget to the container
  if (widget) {
    retVal->setObjectName(widget->objectName() + "_container");
    // SMA this label might need a size policy to keep it from splitting the
    // layout size
    HLayout->addWidget(new QLabel(label));
    HLayout->addWidget(widget);
  }

  return retVal;
}

QComboBox* FOEDAG::createComboBox(const QString& objectName,
                                  const QStringList& options,
                                  const QString& selectedValue) {
  QComboBox* widget = new QComboBox();
  widget->setObjectName(objectName);
  widget->insertItems(0, options);

  // Select a default if selectedValue was set
  int idx = options.indexOf(selectedValue);
  if (idx > -1) {
    widget->setCurrentIndex(idx);
  }

  return widget;
}

QLineEdit* FOEDAG::createLineEdit(const QString& objectName,
                                  const QString& text) {
  QLineEdit* widget = new QLineEdit();
  widget->setObjectName(objectName);

  widget->setText(text);

  return widget;
}

QDoubleSpinBox* FOEDAG::createDoubleSpinBox(const QString& objectName,
                                            double minVal, double maxVal,
                                            double stepVal, double defaultVal) {
  QDoubleSpinBox* widget = new QDoubleSpinBox();
  widget->setObjectName(objectName);

  widget->setMinimum(minVal);
  widget->setMaximum(maxVal);
  widget->setSingleStep(stepVal);
  // SMA this will currently not set a value if a user passes a valid -1
  if (defaultVal != -1) {
    widget->setValue(defaultVal);
  }

  return widget;
}

QSpinBox* FOEDAG::createSpinBox(const QString& objectName, int minVal,
                                int maxVal, int stepVal, int defaultVal) {
  QSpinBox* widget = new QSpinBox();
  widget->setObjectName(objectName);

  widget->setMinimum(minVal);
  widget->setMaximum(maxVal);
  widget->setSingleStep(stepVal);
  // SMA this will currently not set a value if a user passes a valid -1
  if (defaultVal != -1) {
    widget->setValue(defaultVal);
  }

  return widget;
}

QRadioButton* FOEDAG::createRadioButton(const QString& objectName,
                                        const QString& text) {
  QRadioButton* widget = new QRadioButton();
  widget->setObjectName(objectName);

  widget->setText(text);

  return widget;
}

QButtonGroup* FOEDAG::createRadioButtons(const QString& objectName,
                                         const QStringList& nameList,
                                         const QString& selectedValue) {
  QButtonGroup* widget = new QButtonGroup();
  widget->setObjectName(objectName);

  for (const QString& name : nameList) {
    QRadioButton* radioBtn = new QRadioButton();
    radioBtn->setObjectName(objectName + "_" + name);
    radioBtn->setText(name);
    if (name == selectedValue) {
      radioBtn->setChecked(true);
    }
    widget->addButton(radioBtn);
  }

  return widget;
}

QCheckBox* FOEDAG::createCheckBox(const QString& objectName,
                                  const QString& text, Qt::CheckState checked) {
  QCheckBox* widget = new QCheckBox();
  widget->setObjectName(objectName);

  widget->setText(text);
  widget->setCheckState(checked);

  return widget;
}
