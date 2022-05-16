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

#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include <QtWidgets>

#include "../third_party/nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

namespace FOEDAG {

QDialog* createSettingsDialog(json& widgetsJson, const QString& dialogTitle,
                              const QString& objectNamePrefix = "");
QWidget* createSettingsWidget(json& widgetsJson,
                              const QString& objNamePrefix = "");

QWidget* createWidget(const json& widgetJsonObj, const QString& objName = "");
QWidget* createWidget(const QString& widgetJsonStr,
                      const QString& objName = "");
QWidget* createLabelWidget(const QString& label, QWidget* widget);
QComboBox* createComboBox(const QString& objectName, const QStringList& options,
                          const QString& selectedValue = "");
QLineEdit* createLineEdit(const QString& objectName, const QString& text = "");
QDoubleSpinBox* createDoubleSpinBox(const QString& objectName, double minVal,
                                    double maxVal, double stepVal,
                                    double defaultVal);
QSpinBox* createSpinBox(const QString& objectName, int minVal, int maxVal,
                        int stepVal, int defaultVal = -1);
QRadioButton* createRadioButton(const QString& objectName, const QString& text);
QButtonGroup* createRadioButtons(const QString& objectName,
                                 const QStringList& nameList,
                                 const QString& selectedValue = "");
QCheckBox* createCheckBox(const QString& objectName, const QString& text,
                          Qt::CheckState checked);

}  // namespace FOEDAG

#endif