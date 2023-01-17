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
#include "IpConfigurator/IpConfigWidget.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>

#include "Main/WidgetFactory.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "NewProject/ProjectManager/DesignFileWatcher.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Utils/FileUtils.h"

using namespace FOEDAG;
extern FOEDAG::Session* GlobalSession;

#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

static QString SEPARATOR = QString::fromStdString(
    std::string(1, std::filesystem::path::preferred_separator));

QString getUserProjectPath(const QString& suffix) {
  QString path;
  QString projPath =
      GlobalSession->GetCompiler()->ProjManager()->getProjectPath();
  QString projName =
      GlobalSession->GetCompiler()->ProjManager()->getProjectName();

  // Only format for a suffix if one was provided
  QString suffixStr = "";
  if (!suffix.isEmpty()) {
    suffixStr = "." + suffix;
  }

  if (!projPath.isEmpty() && !projName.isEmpty()) {
    path = projPath + SEPARATOR + projName + suffixStr;
  } else {
    path = "." + SEPARATOR + "noProject" + suffixStr;
  }

  return path;
}

IpConfigWidget::IpConfigWidget(QWidget* parent /*nullptr*/,
                               const QString& requestedIpName /* "" */,
                               const QString& moduleName /* "" */,
                               const QStringList& instanceValueArgs /*{}*/)
    : m_requestedIpName(requestedIpName),
      m_instanceValueArgs(instanceValueArgs) {
  this->setWindowTitle("Configure IP");
  this->setObjectName("IpConfigWidget");
  m_baseDirDefault = getUserProjectPath("IPs");

  // Set the path related widgets' tooltips to whatever their text is so long
  // paths are easier to view
  QObject::connect(
      &outputPath, &QLineEdit::textChanged,
      [this](const QString& text) { outputPath.setToolTip(text); });

  // Main Layout
  QVBoxLayout* topLayout = new QVBoxLayout();
  topLayout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(topLayout);

  // Create container widget and QScrollArea so this widget can shrink
  QWidget* containerWidget = new QWidget();
  containerWidget->setObjectName("ipConfigContainerWidget");
  QVBoxLayout* containerLayout = new QVBoxLayout();
  // layout must be set before adding to the scroll area
  // https://doc.qt.io/qt-6/qscrollarea.html#setWidget
  containerWidget->setLayout(containerLayout);
  QScrollArea* scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  scrollArea->setObjectName("ipConfigScrollArea");
  scrollArea->setWidget(containerWidget);
  topLayout->addWidget(scrollArea);

  // Add VLNV meta text description
  containerLayout->addWidget(&metaLabel);

  // Fill and add Parameters box
  CreateParamFields();
  containerLayout->addWidget(&paramsBox);

  // Add Output Box
  CreateOutputFields();
  containerLayout->addWidget(&outputBox);
  containerLayout->addStretch();
  // Update the module name if one was passed (this occurs during a
  // re-configure)
  if (!moduleName.isEmpty()) {
    moduleEdit.setText(moduleName);
  }

  // Add Dialog Buttons
  AddDialogControls(topLayout);

  // Update output path now that meta data has been loaded
  updateOutputPath();
}

void IpConfigWidget::AddDialogControls(QBoxLayout* layout) {
  // Dialog Buttons
  // Originally used QDialogButtonBox as this was a dlg. w/o cancel, buttonBox
  // isn't necessary, but it doesn't hurt to have it and might still provide
  // more standard rendering for location/alignment/etc
  QDialogButtonBox* btns = new QDialogButtonBox(/*QDialogButtonBox::Cancel*/);
  btns->setObjectName("IpConfigWidget_QDialogButtonBox");
  btns->setContentsMargins(0, 0, 5, 5);
  layout->addWidget(btns);
  generateBtn.setText("Generate IP");
  btns->addButton(&generateBtn, QDialogButtonBox::ButtonRole::ActionRole);

  // Create our tcl command to generate the IP when the Generate IP button
  // is clicked
  QObject::connect(&generateBtn, &QPushButton::clicked, this, [this]() {
    // Find settings fields in the parameter box layout
    QLayout* fieldsLayout = paramsBox.layout();
    QList<QObject*> settingsObjs =
        FOEDAG::getTargetObjectsFromLayout(fieldsLayout);

    // Build up a parameter string based off the current UI fields
    QString params = "";

    bool invalidVals = false;
    for (QObject* obj : settingsObjs) {
      // Collect parameters of fields that haven't been disabled by dipendencies
      QWidget* widget = qobject_cast<QWidget*>(obj);
      if (widget && widget->isEnabled()) {
        // Typically widgetFactory widgets can have their value introspected
        // with ->property("tclArg") however the widget factory stores those
        // values on change and some fields like comboboxes don't register a
        // change if the first value is set as the requested value since nothing
        // changes in that scenario. As a result we'll manually build the arg
        // string to ensure all values of interest are captured

        // Convert value to string based off widget type
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget);
        QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget);
        QComboBox* comboBox = qobject_cast<QComboBox*>(widget);
        QAbstractSpinBox* spinBox = qobject_cast<QAbstractSpinBox*>(widget);
        QString val{};
        // note: qobject_cast returns null on failed conversion so the above
        // casts are basically a runtime type check
        if (lineEdit) {
          val = lineEdit->text();
        } else if (checkBox) {
          val = checkBox->isChecked() ? "1" : "0";
        } else if (comboBox) {
          val = comboBox->currentText();
        } else if (spinBox) {
          val = spinBox->text();
        }

        // convert spaces in value to WidgetFactory space tag so the arg list
        // doesn't break
        val.replace(" ", WF_SPACE);

        // build arg string in the form of -P<paramName>=<value>
        QString arg = QString(" -P%1=%2")
                          .arg(obj->property("customId").toString())
                          .arg(val);
        params += arg;

        // check if any values are invalid
        invalidVals |= obj->property("invalid").toBool();
      }
    }

    // Alert the user if one or more of the field validators is invalid
    if (invalidVals) {
      QMessageBox::warning(
          this, tr("Invalid Parameter Value"),
          tr("Current parameters are invalid. IP Generation will be skipped."),
          QMessageBox::Ok);
    } else {
      // If all enabled fields are valid, configure and generate IP
      std::filesystem::path baseDir(m_baseDirDefault.toStdString());
      std::filesystem::path outFile = baseDir / moduleEdit.text().toStdString();
      QString outFileStr =
          QString::fromStdString(FileUtils::GetFullPath(outFile).string());

      // Build up a cmd string to generate the IP
      QString cmd = "configure_ip " + this->m_requestedIpName + " -mod_name " +
                    moduleEdit.text() + " -version " +
                    QString::fromStdString(m_meta.version) + " " + params +
                    " -out_file " + outFileStr;
      cmd += "\nipgenerate -modules " + moduleEdit.text() + "\n";

      GlobalSession->TclInterp()->evalCmd(cmd.toStdString());

      AddIpToProject(cmd);
      emit ipInstancesUpdated();
    }
  });
}

void IpConfigWidget::AddIpToProject(const QString& cmd) {
  FOEDAG::Compiler* compiler = nullptr;
  ProjectManager* projManager = nullptr;
  if (GlobalSession && (compiler = GlobalSession->GetCompiler()) &&
      (projManager = compiler->ProjManager())) {
    auto getUniqueString = [](const QString& ipConfigCmd) -> QString {
      // Use the configure command's first half(ipName and module name) as a
      // unique id to determine if this IP configuration has already been stored
      QStringList cmdParts = ipConfigCmd.split("-version");
      if (cmdParts.length() > 0) {
        QString id = cmdParts[0];
        return id;
      } else {
        return QString{};
      }
    };

    QString cmdId = getUniqueString(cmd);
    // Helper function to see if a given cmd's id from getUniqueString matches
    // the IP we are about to add
    auto isMatch = [cmdId, getUniqueString](const std::string& str) {
      QString cmdStr = getUniqueString(QString::fromStdString(str));
      return (cmdStr == cmdId);
    };

    // Get current instance commands
    auto cmds = projManager->ipInstanceCmdList();
    // Remove any entires that match this new cmd
    cmds.erase(std::remove_if(cmds.begin(), cmds.end(), isMatch), cmds.end());
    // Add new instance command to the command list
    cmds.push_back(cmd.toStdString());
    // Store the updated instance list
    projManager->setIpInstanceCmdList(cmds);
    // Update file watchers since new ip folders have probably been added
    DesignFileWatcher::Instance()->updateDesignFileWatchers(projManager);
  }
}

void IpConfigWidget::CreateParamFields() {
  QStringList tclArgList;
  json parentJson;
  // Loop through IPDefinitions stored in IPCatalog
  for (auto def : getDefinitions()) {
    // if this definition is for the requested IP
    if (m_requestedIpName.toStdString() == def->Name()) {
      // Store VLNV meta data for the requested IP
      m_meta = FOEDAG::getIpInfoFromPath(def->FilePath());

      // set default module name to the BuildName provided by the generate
      // script otherwise default to the to the VLNV name
      std::string build_name = def->BuildName();
      if (build_name.empty()) {
        build_name = m_meta.name;
      }
      moduleEdit.setText(QString::fromStdString(build_name));

      // Update meta label now that vlnv and module info is updated
      updateMetaLabel(m_meta);

      // Build widget factory json for each parameter
      for (auto paramVal : def->Parameters()) {
        if (paramVal->GetType() == Value::Type::ParamIpVal) {
          IPParameter* param = static_cast<IPParameter*>(paramVal);
          json childJson;
          // Add P to the arg for configure_ip format: -P{ARG_NAME}
          childJson["arg"] = "P" + param->Name();
          // use the param name as a customId for dependency checking
          childJson["customId"] = param->Name();
          childJson["label"] = param->GetTitle();
          childJson["tooltip"] = param->GetDescription();
          childJson["bool_dependencies"] = param->GetDependencies();
          std::string defaultValue = param->GetSValue();

          // Determine what type of widget we need for this parameter
          if (param->GetParamType() == IPParameter::ParamType::Bool) {
            // Use Checkboxes for Bools
            childJson["widgetType"] = "checkbox";
          } else if (param->GetParamType() ==
                     IPParameter::ParamType::FilePath) {
            childJson["widgetType"] = "filepath";
          } else if (param->GetOptions().size()) {
            // Use Comboboxes if "options" field exists
            childJson["widgetType"] = "combobox";
            childJson["options"] = param->GetOptions();
            childJson["addUnset"] = false;
          } else if (param->GetRange().size() > 1) {
            // Use QLineedit w/ a validator if "range" field exists
            auto range = param->GetRange();
            if (range.size() == 2) {
              auto paramType = param->GetParamType();
              childJson["widgetType"] = "input";
              childJson["validatorMin"] = range[0];
              childJson["validatorMax"] = range[1];

              // Add range info to parameter title
              std::string rangeStr = " <span style=\"color:grey;\">[" +
                                     range[0] + ", " + range[1] + "]</span>";
              childJson["label"] = param->GetTitle() + rangeStr;

              if (paramType == IPParameter::ParamType::Int) {
                childJson["validator"] = "int";
              } else if (paramType == IPParameter::ParamType::Float) {
                childJson["validator"] =
                    "double";  // Qt only provides a double validator
              } else {
                // TODO @skyler-rs nov2022 add error msg when logging is avail
                // Range option only supports float and int types
              }
            } else {
              // TODO @skyler-rs nov2022 add error msg when logging is avail
              // only 2 values expected, rest will be ignored
            }
          } else {
            childJson["widgetType"] = "input";
          }

          parentJson[param->Name()] = childJson;

          // replaces spaces in value so arg list doesn't break
          QString valNoSpaces =
              QString::fromStdString(defaultValue).replace(" ", WF_SPACE);

          // Create a list of tcl defaults that will be passed to createWidget
          tclArgList << QString("-P%1 %2")
                            .arg(QString::fromStdString(param->Name()))
                            .arg(valNoSpaces);
        }
      }
    }
  }

  // Use passed args if we are updating an IP instance
  if (!m_instanceValueArgs.isEmpty()) {
    tclArgList = m_instanceValueArgs;
  }

  if (parentJson.empty()) {
    // Add a note if no parameters were available
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(new QLabel("<em>This IP has no parameters</em>"));
    paramsBox.setLayout(layout);
  } else {
    // Create and add the child widget to our parent container
    auto form = createWidgetFormLayout(parentJson, tclArgList);
    paramsBox.setLayout(form);
  }

  // createWidgetFormLayout sequentially creates widgets from parentJson, which
  // means that after first creation, a dependency might have been created and
  // set before a dependent widget existed possibly leaving that dependent
  // widget in an incorrect state (ex: a dependent widget might be enabled even
  // tho the widget it depends on was false at initialization by created after)
  // checkDepenencies manually updates the fields to capture the correct state
  // after form creation
  checkDependencies();

  // After the fields are set to valid init states, we can then use
  // handleCheckBoxChanged to update any runtime dependency changes
  QObject::connect(WidgetFactoryDependencyNotifier::Instance(),
                   &WidgetFactoryDependencyNotifier::checkboxChanged, this,
                   &IpConfigWidget::handleCheckBoxChanged);
}

void IpConfigWidget::CreateOutputFields() {
  QFormLayout* form = new QFormLayout(&outputBox);
  form->setLabelAlignment(Qt::AlignRight);

  // Set objectNames for future testing targets
  moduleEdit.setObjectName("IpConfigurator_moduleLineEdit");
  outputPath.setObjectName("IpConfigurator_outputPathLineEdit");

  // Make the output directory field read only
  outputPath.setReadOnly(true);
  outputPath.setStyleSheet(
      QString("QLineEdit{ background-color: %1; }")
          .arg(QWidget::palette()
                   .color(QPalette::Disabled, QPalette::Base)
                   .name()));

  // Create a list of label/widget pairs
  std::vector<std::pair<std::string, QWidget*>> pairs = {
      {"Module Name", &moduleEdit}, {"Output Dir", &outputPath}};

  // Loop through pairs and add them to layout
  for (auto [labelName, widget] : pairs) {
    form->addRow(QString::fromStdString(labelName), widget);
  }

  // Update the output dir when module name changes
  QObject::connect(&moduleEdit, &QLineEdit::textChanged, this,
                   &IpConfigWidget::updateOutputPath);

  // add the layout to the output group box
  outputBox.setLayout(form);
}

void IpConfigWidget::updateMetaLabel(VLNV info) {
  // Create a descriptive sentence that lists all the VLNV info
  QString verStr = QString::fromStdString(info.version).replace("_", ".");
  QString action = "Configuring";
  if (!m_instanceValueArgs.empty()) {
    // if params were passed then we are reconfiguring an existing instance
    action = "Reconfiguring instance \"" + moduleEdit.text() + "\" for ";
  }
  std::string text = "<em>" + action.toStdString() + " " + info.name + " (" +
                     verStr.toStdString() + ")" + " from " + info.vendor +
                     "'s " + info.library + " library</em>";
  metaLabel.setTextFormat(Qt::RichText);
  metaLabel.setText(QString::fromStdString(text));
  metaLabel.setWordWrap(true);
}

// Returns the IPDefinitions stored in the current IPGenerator's IPCatalog
std::vector<FOEDAG::IPDefinition*> IpConfigWidget::getDefinitions() {
  FOEDAG::Compiler* compiler = nullptr;
  FOEDAG::IPGenerator* ipgen = nullptr;
  FOEDAG::IPCatalog* ipcatalog = nullptr;
  std::vector<FOEDAG::IPDefinition*> defs;

  // This just checks at each getter step to make sure no nulls are returned
  if (GlobalSession && (compiler = GlobalSession->GetCompiler()) &&
      (ipgen = compiler->GetIPGenerator()) && (ipcatalog = ipgen->Catalog())) {
    defs = ipcatalog->Definitions();
  }

  return defs;
}

void IpConfigWidget::updateOutputPath() {
  // Create and add vlnv path to base IPs directory
  std::filesystem::path baseDir(m_baseDirDefault.toStdString());
  std::filesystem::path vlnvPath =
      baseDir / m_meta.vendor / m_meta.library / m_meta.name / m_meta.version;

  // Add the module wrapper
  std::filesystem::path outPath = vlnvPath / moduleEdit.text().toStdString();

  // Update the output path text
  QString outStr =
      QString::fromStdString(FileUtils::GetFullPath(outPath).string());
  outputPath.setText(outStr);

  // Disable the generate button if the module name is empty
  generateBtn.setEnabled(!moduleEdit.text().isEmpty());
}

void IpConfigWidget::handleCheckBoxChanged(const QString& customId,
                                           QCheckBox* checkbox) {
  QList<QObject*> paramObjects =
      FOEDAG::getTargetObjectsFromLayout(paramsBox.layout());
  // Step through all fields
  for (auto obj : paramObjects) {
    // If this field depends on the checkbox that just toggled
    if (obj->property("bool_dependency").toString() == customId) {
      QWidget* widget = qobject_cast<QWidget*>(obj);
      if (widget) {
        // Update the dependent widget's enabled state
        widget->setEnabled(checkbox->isChecked());
      }
    }
  }
}

void IpConfigWidget::checkDependencies() {
  QList<QObject*> paramObjects =
      FOEDAG::getTargetObjectsFromLayout(paramsBox.layout());
  // Step through all fields
  for (auto updateWidget : paramObjects) {
    // Check if this field depends on another field
    auto targetId = updateWidget->property("bool_dependency").toString();
    // don't bother searching if this field doesn't have a dependency
    if (!targetId.isEmpty()) {
      // Loop through the fields again to see if any of them are the target
      // field
      for (auto widget : paramObjects) {
        auto customId = widget->property("customId").toString();
        if (customId == targetId) {
          // Update the enable state of the dependent field if we found the
          // dependency match, currenlty we only support boolean dependencies so
          // we assume the field we depend on is a QCheckBox
          QWidget* dependentWidget = qobject_cast<QWidget*>(updateWidget);
          QCheckBox* checkbox = qobject_cast<QCheckBox*>(widget);
          if (dependentWidget && checkbox) {
            dependentWidget->setEnabled(checkbox->isChecked());
          }
          // only depending on one widget currently so we can bail this loop on
          // first successful result
          break;
        }
      }
    }
  }
}
