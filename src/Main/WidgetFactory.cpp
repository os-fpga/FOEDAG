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
#include <iostream>

using namespace FOEDAG;

const QString DlgBtnBoxName{"SettingsDialogButtonBox"};

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
  obj->setProperty("changed", true);
  obj->setProperty("jsonPatch", QString::fromStdString(patch.dump()));
}

template <typename T>
T getDefault(const json& jsonObj) {
  T val;
  if (jsonObj.contains("default")) {
    val = jsonObj["default"].get<T>();
  }
  return val;
}

QDialog* FOEDAG::createSettingsDialog(json& widgetsJson,
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

  QWidget* widget =
      FOEDAG::createSettingsWidget(widgetsJson, objectNamePrefix, tclArgs);
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

QWidget* FOEDAG::createSettingsWidget(json& widgetsJson,
                                      const QString& objNamePrefix /* "" */,
                                      const QString& tclArgs /* "" */) {
  // Create a parent widget to contain all generated widgets
  QWidget* widget = new QWidget();

  widget->setObjectName(objNamePrefix + SETTINGS_WIDGET_SUFFIX);
  QVBoxLayout* VLayout = new QVBoxLayout();
  widget->setLayout(VLayout);

  // Create and add the child widget to our parent container
  QStringList tclArgList = tclArgs.split("-");
  for (auto [widgetId, widgetJson] : widgetsJson.items()) {
    QWidget* subWidget = FOEDAG::createWidget(
        widgetJson, QString::fromStdString(widgetId), tclArgList);
    VLayout->addWidget(subWidget);
  }

  // Add ok/cancel buttons
  QDialogButtonBox* btnBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  btnBox->setObjectName(DlgBtnBoxName);
  VLayout->addWidget(btnBox);

  // This will collect value changes for each widget/setting, save those
  // settings to a user file and add the changes to the passed in settings
  // This will also build up and store a tcl arg list for these
  // settings if the widgets have "arg" fields defined
  auto checkVals = [VLayout, &widgetsJson, widget]() {
    bool save = false;
    QHash<QString, QString> patchHash;
    QString argsStr = "";
    for (int i = 0; i < VLayout->count(); i++) {
      QWidget* settingsWidget = VLayout->itemAt(i)->widget();
      if (settingsWidget) {
        QObject* targetObject =
            qvariant_cast<QObject*>(settingsWidget->property("targetObject"));

        if (targetObject && targetObject->property("changed").toBool()) {
          save = true;
          QString settingsId =
              settingsWidget->property("settingsId").toString();
          QString patchStr = targetObject->property("jsonPatch").toString();

          WIDGET_DBG_PRINT("createSettingsWidget: saving value " +
                           settingsId.toStdString() + " -> " +
                           patchStr.toStdString() + "\n");
          patchHash[settingsId] = patchStr;

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

  // Check and store value changes if the dialog is accepted
  QObject::connect(btnBox, &QDialogButtonBox::accepted, widget, checkVals);

  return widget;
}

QWidget* FOEDAG::createWidget(const json& widgetJsonObj, const QString& objName,
                              const QStringList& args) {
  auto getStr = [](const json& jsonObj, const QString& key,
                   const QString& defaultStr = "") {
    std::string val =
        jsonObj.value(key.toStdString(), defaultStr.toStdString());
    return QString::fromStdString(val);
  };

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

  // Turn the arg list into a hash
  QHash<QString, QString> argPairs;
  for (auto argEntry : args) {
    QStringList tokens = argEntry.split(' ');
    argPairs[tokens[0]];  // implicitly create an entry in case the arg is a
                          // switch w/o a parameter
    if (tokens.count() > 1) {
      // store parameter if it was passed with the argument
      argPairs[tokens[0]] = tokens[1];
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
    if (type == "input" || type == "lineedit") {
      // QLineEdit - "input" or "lineedit"
      QString sysDefaultVal =
          QString::fromStdString(getDefault<std::string>(widgetJsonObj));

      // Callback to handle value changes
      std::function<void(QLineEdit*, const QString&)> handleChange =
          [](QLineEdit* ptr, const QString& val) {
            json changeJson;
            changeJson["userValue"] = ptr->text().toStdString();
            storeJsonPatch(ptr, changeJson);
          };

      // Create our widget
      auto ptr = createLineEdit(objName, sysDefaultVal, handleChange);
      createdWidget = ptr;

      // Load and set user value
      if (widgetJsonObj.contains("userValue")) {
        QString userVal = QString::fromStdString(
            widgetJsonObj["userValue"].get<std::string>());
        ptr->setText(userVal);

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
            if (arg != "" && userVal != "<unset>") {
              QString argStr = "-" + arg + " " +
                               lookupStr(comboOptions, comboLookup, userVal);
              ptr->setProperty("tclArg", argStr);
              WIDGET_DBG_PRINT("combobox handleChange - Storing Tcl Arg:  " +
                               argStr.toStdString() + "\n");
            }
          };

      // Create Widget
      auto ptr =
          createComboBox(objName, comboOptions, sysDefaultVal, handleChange);
      createdWidget = ptr;

      if (tclArgPassed) {
        // Do a reverse lookup to convert the tcl value to a display value
        ptr->setCurrentText(lookupStr(comboLookup, comboOptions, argVal));
      } else if (widgetJsonObj.contains("userValue")) {
        // Load and set user value
        QString userVal = QString::fromStdString(
            widgetJsonObj["userValue"].get<std::string>());
        ptr->setCurrentText(userVal);

        DBG_PRINT_VAL_SET(ptr, userVal);
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
          [](QSpinBox* ptr, const int& val) {
            json changeJson;
            changeJson["userValue"] = ptr->value();

            storeJsonPatch(ptr, changeJson);
          };

      // Create Widget
      auto ptr = createSpinBox(objName, minVal, maxVal, stepVal, sysDefaultVal,
                               handleChange);
      createdWidget = ptr;

      // Load and set user value
      if (widgetJsonObj.contains("userValue")) {
        int userVal = widgetJsonObj["userValue"].get<int>();
        ptr->setValue(userVal);

        DBG_PRINT_VAL_SET(ptr, QString::number(userVal));
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
          [](QDoubleSpinBox* ptr, const double& val) {
            json changeJson;
            changeJson["userValue"] = ptr->value();

            storeJsonPatch(ptr, changeJson);
          };

      // Create Widget
      auto ptr = createDoubleSpinBox(objName, minVal, maxVal, stepVal,
                                     sysDefaultVal, handleChange);
      createdWidget = ptr;

      // Load and set user value
      if (widgetJsonObj.contains("userValue")) {
        double userVal = widgetJsonObj["userValue"].get<double>();
        ptr->setValue(userVal);

        DBG_PRINT_VAL_SET(ptr, QString::number(userVal));
      }

      targetObject = createdWidget;
    } else if (type == "radiobuttons") {
      // QButtonGroup of QRadioButtons - "radiobuttons"
      QString sysDefaultVal =
          QString::fromStdString(getDefault<std::string>(widgetJsonObj));
      QStringList options =
          JsonArrayToQStringList(widgetJsonObj.value("options", json::array()));

      // Callback to handle value changes
      std::function<void(QRadioButton*, QButtonGroup*, const bool&)>
          handleChange = [](QRadioButton* btnPtr, QButtonGroup* btnGroup,
                            const bool& checked) {
            json changeJson;
            changeJson["userValue"] = btnPtr->text().toStdString();
            storeJsonPatch(btnGroup, changeJson);
          };

      // Create radiobuttons in a QButtonGroup
      QButtonGroup* btnGroup = FOEDAG::createRadioButtons(
          objName, options, sysDefaultVal, handleChange);

      // Load and set user value
      if (widgetJsonObj.contains("userValue")) {
        QString userVal = QString::fromStdString(
            widgetJsonObj["userValue"].get<std::string>());

        for (auto btn : btnGroup->buttons()) {
          if (btn->text() == userVal) {
            btn->setChecked(true);
          }
        }

        DBG_PRINT_VAL_SET(btnGroup, userVal);
      }

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
        if (stateStr.toLower() == "checked") {
          state = Qt::Checked;
        } else if (stateStr.toLower() == "partiallychecked") {
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
          };

      // Create Widget
      auto ptr = createCheckBox(objName, text, state, handleChange);
      createdWidget = ptr;

      // For boolean values like checkbox, the presence of an arg
      // means the value is checked
      if (tclArgPassed) {
        ptr->setChecked(Qt::Checked);
      } else if (widgetJsonObj.contains("userValue")) {
        // Load and set user value
        QString userVal = QString::fromStdString(
            widgetJsonObj["userValue"].get<std::string>());
        ptr->setCheckState(stringToCheckState(userVal));

        DBG_PRINT_VAL_SET(ptr, userVal);
      }

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

QWidget* FOEDAG::createWidget(const QString& widgetJsonStr,
                              const QString& objName, const QStringList& args) {
  return createWidget(json::parse(widgetJsonStr.toStdString()), objName, args);
}

QWidget* FOEDAG::createLabelWidget(const QString& label, QWidget* widget) {
  // Create a container widget w/ an H layout
  QWidget* retVal = new QWidget();
  QHBoxLayout* HLayout = new QHBoxLayout();
  HLayout->setContentsMargins(0, 0, 0, 0);
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

QComboBox* FOEDAG::createComboBox(
    const QString& objectName, const QStringList& options,
    const QString& selectedValue,
    std::function<void(QComboBox*, const QString&)> onChange) {
  QComboBox* widget = new QComboBox();
  widget->setObjectName(objectName);
  widget->insertItems(0, options);
  widget->addItem("<unset>");
  widget->setCurrentText("<unset>");
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
