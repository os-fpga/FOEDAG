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

#include <QDebug>  // sma remove
#include <QJsonDocument>

using namespace FOEDAG;

// Local helper function to convert QJsonArrays to QStringLists
// This assumes the QJsonArray contains string values
QStringList QJsonArrayToQStringList(const QJsonArray& jsonArray) {
  QStringList strings;
  std::transform(jsonArray.begin(), jsonArray.end(),
                 std::back_inserter(strings),
                 [](QJsonValue val) -> QString { return val.toString(); });

  return strings;
}

QWidget* FOEDAG::createWidget(const QJsonObject& widgetJsonObj) {
  QWidget* retVal = nullptr;
  QWidget* createdObj = nullptr;

  if (widgetJsonObj.contains("widgetType")) {
    // Grab standard entries
    QString objName = widgetJsonObj.value("id").toString("");
    QString defaultVal = widgetJsonObj.value("default").toString("");

    // Get widget type and create respective widget if it's supported
    QString type = widgetJsonObj.value("widgetType").toString("");
    type = type.toLower();
    if (type == "input" || type == "lineedit") {
      // QLineEdit - "input" or "lineedit"

      createdObj = createLineEdit(objName, defaultVal);
    } else if (type == "dropdown" || type == "combobox") {
      // QComboBox - "dropdown" or "combobox"

      QJsonArray options = widgetJsonObj.value("options").toArray({});
      QStringList comboOptions = QJsonArrayToQStringList(options);

      createdObj = createComboBox(objName, comboOptions, defaultVal);
    } else if (type == "spinbox") {
      // QSpinBox - "spinbox"

      // QJsonValue.toInt/toDouble returns 0 if the type doesn't match
      // calling toVariant() first gives use normal c style int/double
      // truncation
      int minVal = widgetJsonObj.value("minVal").toVariant().toInt();
      int maxVal = widgetJsonObj.value("maxVal").toVariant().toInt();
      int stepVal = widgetJsonObj.value("stepVal").toVariant().toInt();
      int defaultVal = widgetJsonObj.value("default").toVariant().toInt();

      createdObj = createSpinBox(objName, minVal, maxVal, stepVal, defaultVal);
    } else if (type == "doublespinbox") {
      // QDoubleSpinBox - "doublespinbox"

      // QJsonValue.toInt/toDouble returns 0 if the type doesn't match
      // calling toVariant() first gives use normal c style int/double
      // truncation
      double minVal = widgetJsonObj.value("minVal").toVariant().toDouble();
      double maxVal = widgetJsonObj.value("maxVal").toVariant().toDouble();
      double stepVal = widgetJsonObj.value("stepVal").toVariant().toDouble();
      double defaultVal = widgetJsonObj.value("default").toVariant().toDouble();

      createdObj =
          createDoubleSpinBox(objName, minVal, maxVal, stepVal, defaultVal);
    } else if (type == "radiobuttons") {
      // QButtonGroup of QRadioButtons - "radiobuttons"

      QStringList options =
          QJsonArrayToQStringList(widgetJsonObj.value("options").toArray({}));
      QString defaultVal = widgetJsonObj.value("default").toString("");

      // Create radiobuttons in a QButtonGroup
      QButtonGroup* btnGroup =
          FOEDAG::createRadioButtons(objName, options, defaultVal);
      // ButtonGroups aren't real QWidgets so we need to add their child
      // radiobuttons to a container widget
      QWidget* container = new QWidget();
      container->setObjectName(objName + "_container");

      // Determine radiobutton layout direction
      QBoxLayout* containerLayout = nullptr;
      if ("horizontal" == widgetJsonObj.value("layout").toString("")) {
        containerLayout = new QHBoxLayout();
      } else {
        containerLayout = new QVBoxLayout();
      }

      // Add radiobuttons to container widget's layout
      container->setLayout(containerLayout);
      for (auto* btn : btnGroup->buttons()) {
        containerLayout->addWidget(btn);
      }

      createdObj = container;
    } else if (type == "radiobutton") {
      // not sure single radiobutton is needed
    } else if (type == "checkbox") {
      // QCheckBox - "checkbox"

      QString text = widgetJsonObj.value("text").toString("");

      // Determine checkstate
      QString checkState =
          widgetJsonObj.value("default").toString("unchecked").toLower();
      Qt::CheckState state = Qt::Unchecked;
      if (checkState == "checked") {
        state = Qt::Checked;
      } else if (checkState == "partiallychecked") {
        state = Qt::PartiallyChecked;
      }

      createdObj = createCheckBox(objName, text, state);
    }

    // Add a containing widget and label if "label" property was provided
    QString label = widgetJsonObj.value("label").toString();
    if (createdObj && !label.isEmpty()) {
      retVal = FOEDAG::createLabelWidget(label, createdObj);
    } else {
      retVal = createdObj;
    }
  }

  return retVal;
}

QWidget* FOEDAG::createWidget(const QString& widgetJsonStr) {
  QJsonDocument jsonDoc = QJsonDocument::fromJson(widgetJsonStr.toUtf8());
  QJsonObject jsonObj = jsonDoc.object();

  return FOEDAG::createWidget(jsonObj);
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
