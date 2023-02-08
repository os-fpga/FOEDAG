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

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QRadioButton>
#include <QTextEdit>

#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

using tclArgSetterFn = std::function<void(const std::string&)>;
using tclArgGetterFn = std::function<std::string()>;
using tclArgFns = std::pair<tclArgSetterFn, tclArgGetterFn>;
using tclArgFnMap = std::map<std::string, tclArgFns>;

#define SETTINGS_WIDGET_SUFFIX "SettingsWidget"
#define DlgBtnBoxName "SettingsDialogButtonBox"
#define WF_SPACE "_TclArgSpace_"
#define WF_NEWLINE "_TclArgNewLine_"
#define WF_DASH "_TclArgDash_"

namespace FOEDAG {
constexpr bool addUnsetDefault{false};
QString convertAll(const QString& str);
QString restoreAll(const QString& str);
void initTclArgFns();
void clearTclArgFns();
void addTclArgFns(const std::string& tclArgKey, tclArgFns argFns);
tclArgFns getTclArgFns(const QString& tclArgKey);
QDialog* createTopSettingsDialog(json& widgetsJson,
                                 const QString& selectedCategoryTitle = "");
QDialog* createSettingsDialog(const QString& jsonPath,
                              const QString& dialogTitle,
                              const QString& objectNamePrefix = "",
                              const QString& tclArgs = "");
QWidget* createSettingsPane(const QString& jsonPath,
                            tclArgSetterFn tclArgSetter = nullptr,
                            tclArgGetterFn tclArgGetter = nullptr);
QWidget* createSettingsWidget(json& widgetsJson,
                              const QString& objNamePrefix = "",
                              const QString& tclArgs = "");

QFormLayout* createWidgetFormLayout(json& widgetsJson,
                                    const QStringList& tclArgList = {});
QWidget* createWidget(const json& widgetJsonObj, const QString& objName = "",
                      const QStringList& args = QStringList());
QWidget* createWidget(const QString& widgetJsonStr, const QString& objName = "",
                      const QStringList& args = QStringList());
QWidget* createLabelWidget(const QString& label, QWidget* widget);
QWidget* createContainerWidget(QWidget* widget,
                               const QString& label = QString());
QComboBox* createComboBox(
    const QString& objectName, const QStringList& options,
    const QString& selectedValue = "", bool addUnset = addUnsetDefault,
    std::function<void(QComboBox*, const QString&)> onChange = nullptr);
QLineEdit* createLineEdit(
    const QString& objectName, const QString& text = "",
    std::function<void(QLineEdit*, const QString&)> onChange = nullptr);
QTextEdit* createTextEdit(
    const QString& objectName, const QString& text = "",
    std::function<void(QTextEdit*, const QString&)> onChange = nullptr);
QDoubleSpinBox* createDoubleSpinBox(
    const QString& objectName, double minVal, double maxVal, double stepVal,
    double defaultVal,
    std::function<void(QDoubleSpinBox*, const double&)> onChange = nullptr);
QSpinBox* createSpinBox(
    const QString& objectName, int minVal, int maxVal, int stepVal,
    int defaultVal,
    std::function<void(QSpinBox*, const int&)> onChange = nullptr);
QButtonGroup* createRadioButtons(
    const QString& objectName, const QStringList& nameList,
    const QString& selectedValue = "",
    std::function<void(QRadioButton*, QButtonGroup*, const bool&)> onChange =
        nullptr);
QCheckBox* createCheckBox(
    const QString& objectName, const QString& text, Qt::CheckState checked,
    std::function<void(QCheckBox*, const int&)> onChange = nullptr);
QList<QObject*> getTargetObjectsFromLayout(QLayout* layout);

class WidgetFactoryDependencyNotifier : public QObject {
  Q_OBJECT

 public:
  static WidgetFactoryDependencyNotifier* Instance();

 signals:
  void checkboxChanged(const QString& customId, QCheckBox* widget);
};

}  // namespace FOEDAG

#endif
