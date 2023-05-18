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

#include <QDebug>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>

#include "IPGenerate/IPCatalogBuilder.h"
#include "Main/WidgetFactory.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/DesignFileWatcher.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"

using namespace FOEDAG;
extern FOEDAG::Session* GlobalSession;

#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

QString getUserProjectPath(const QString& suffix) {
  static QString SEPARATOR = QString::fromStdString(
      std::string(1, std::filesystem::path::preferred_separator));
  QString path;
  QString projPath =
      GlobalSession->GetCompiler()->ProjManager()->getProjectPath();
  QString projName =
      GlobalSession->GetCompiler()->ProjManager()->getProjectName();

  // Only format for a suffix if one was provided
  QString suffixStr{};
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
    : paramsBox{new QGroupBox{"Parameters", this}},
      m_baseDirDefault{getUserProjectPath("IPs")},
      m_requestedIpName(requestedIpName),
      m_instanceValueArgs(instanceValueArgs) {
  this->setWindowTitle("Configure IP");
  this->setObjectName("IpConfigWidget");

  // Set the path related widgets' tooltips to whatever their text is so long
  // paths are easier to view
  QObject::connect(
      &outputPath, &QLineEdit::textChanged, this,
      [this](const QString& text) { outputPath.setToolTip(text); });

  // Main Layout
  QVBoxLayout* topLayout = new QVBoxLayout();
  topLayout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(topLayout);

  // Create container widget and QScrollArea so this widget can shrink
  QWidget* containerWidget = new QWidget();
  containerWidget->setObjectName("ipConfigContainerWidget");
  containerLayout = new QVBoxLayout();
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
  CreateParamFields(true);
  containerLayout->addWidget(paramsBox);

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

  // run with --json --json-template parameters to get default GUI
  if (!requestedIpName.isEmpty()) handleEditorChanged({}, nullptr);
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
  QObject::connect(&generateBtn, &QPushButton::clicked, this,
                   [this]() { Generate(true); });
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

void IpConfigWidget::CreateParamFields(bool generateMetaLabel) {
  QStringList tclArgList;
  json parentJson;
  // Loop through IPDefinitions stored in IPCatalog
  for (auto def : getDefinitions()) {
    // if this definition is for the requested IP
    if (m_requestedIpName.toStdString() == def->Name()) {
      // Store VLNV meta data for the requested IP
      m_meta = FOEDAG::getIpInfoFromPath(def->FilePath());

      if (generateMetaLabel) {
        // set default module name to the BuildName provided by the generate
        // script otherwise default to the to the VLNV name
        std::string build_name = def->BuildName();
        if (build_name.empty()) {
          build_name = m_meta.name;
        }
        moduleEdit.setText(QString::fromStdString(build_name));

        // Update meta label now that vlnv and module info is updated
        updateMetaLabel(m_meta);
      }

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
          childJson["disable"] = param->Disabled();
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
            childJson["optionsLookup"] = param->GetOptions();
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
          tclArgList << QString("-P%1 %2").arg(
              QString::fromStdString(param->Name()), valNoSpaces);
        }
      }
    }
  }

  // Use passed args if we are updating an IP instance
  if (!m_instanceValueArgs.isEmpty()) {
    tclArgList = m_instanceValueArgs;
  }

  containerLayout->removeWidget(paramsBox);
  paramsBox->deleteLater();
  paramsBox = new QGroupBox{"Parameters", this};
  containerLayout->insertWidget(1, paramsBox);

  if (parentJson.empty()) {
    // Add a note if no parameters were available
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(new QLabel("<em>This IP has no parameters</em>"));
    paramsBox->setLayout(layout);
  } else {
    // Create and add the child widget to our parent container
    auto form = createWidgetFormLayout(parentJson, tclArgList);
    paramsBox->setLayout(form);
  }

  QObject::connect(WidgetFactoryDependencyNotifier::Instance(),
                   &WidgetFactoryDependencyNotifier::editorChanged, this,
                   &IpConfigWidget::handleEditorChanged, Qt::UniqueConnection);
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
  for (const auto& [labelName, widget] : pairs) {
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

QMap<QVariant, QVariant> IpConfigWidget::saveProperties(bool& valid) const {
  QLayout* fieldsLayout = paramsBox->layout();
  QList<QObject*> settingsObjs =
      FOEDAG::getTargetObjectsFromLayout(fieldsLayout);
  QMap<QVariant, QVariant> properties{};

  for (QObject* obj : settingsObjs) {
    properties.insert(obj->property("customId"), obj->property("value"));
    if (obj->property("invalid").toBool()) valid = false;
  }
  return properties;
}

std::pair<std::string, std::string> IpConfigWidget::generateNewJson(bool& ok) {
  Compiler* compiler = GlobalSession->GetCompiler();
  auto generator = compiler->GetIPGenerator();
  std::filesystem::path baseDir{generator->GetTmpPath()};
  std::filesystem::path outFile = baseDir / moduleEdit.text().toStdString();
  QString outFileStr =
      QString::fromStdString(FileUtils::GetFullPath(outFile).string());
  Generate(false, outFileStr);

  std::string newJson{};
  std::filesystem::path executable{};

  for (IPInstance* inst : generator->IPInstances()) {
    if (inst->IPName() != m_requestedIpName.toStdString()) continue;

    // Create output directory
    const std::filesystem::path& out_path = inst->OutputFile();
    if (!std::filesystem::exists(out_path)) {
      std::filesystem::create_directories(out_path.parent_path());
    }

    const IPDefinition* def = inst->Definition();
    switch (def->Type()) {
      case IPDefinition::IPType::Other: {
        break;
      }
      case IPDefinition::IPType::LiteXGenerator: {
        executable = def->FilePath();
        std::filesystem::path jsonFile = generator->GetTmpCachePath(inst);
        // Create directory path if it doesn't exist otherwise the following
        // ofstream command will fail
        FileUtils::MkDirs(jsonFile.parent_path());
        std::ofstream jsonF(jsonFile);
        jsonF << "{" << std::endl;
        for (const auto& param : inst->Parameters()) {
          std::string value{};
          // The configure_ip command loses type info because we go from full
          // json meta data provided by the ip_catalog generators to a single
          // -Pname=val argument in a tcl command line. As such, we'll use the
          // ip catalog's definition for parameter type info
          auto catalogParam = generator->GetCatalogParam(inst, param.Name());
          if (catalogParam) {
            switch (catalogParam->GetType()) {
              case Value::Type::ParamIpVal: {
                value = param.GetSValue();
                auto type = ((IPParameter*)catalogParam)->GetParamType();
                if (type == IPParameter::ParamType::FilePath ||
                    type == IPParameter::ParamType::String) {
                  value = "\"" + value + "\"";
                }
                break;
              }
              case Value::Type::ParamString:
                value = param.GetSValue();
                value = "\"" + value + "\"";
                break;
              case Value::Type::ParamInt:
                value = param.GetSValue();
                break;
              case Value::Type::ConstInt:
                value = param.GetSValue();
            }
          }
          if (value.empty()) {
            ok = false;
            return {};
          }
          jsonF << "   \"" << param.Name() << "\": " << value << ","
                << std::endl;
        }
        jsonF << "   \"build_dir\": " << inst->OutputFile().parent_path() << ","
              << std::endl;
        jsonF << "   \"build_name\": " << inst->OutputFile().filename() << ","
              << std::endl;
        jsonF << "   \"build\": false," << std::endl;
        jsonF << "   \"json\": \"" << jsonFile.filename().string() << "\","
              << std::endl;
        jsonF << "   \"json_template\": false" << std::endl;
        jsonF << "}" << std::endl;
        jsonF.close();

        // Find path to litex enabled python interpreter
        std::filesystem::path pythonPath = IPCatalog::getPythonPath();
        if (pythonPath.empty()) {
          std::filesystem::path python3Path =
              FileUtils::LocateExecFile("python3");
          if (python3Path.empty()) {
            compiler->ErrorMessage(
                "IP Generate, unable to find python interpreter in local "
                "environment.\n");
            ok = false;
            return {};
          } else {
            pythonPath = python3Path;
            compiler->ErrorMessage(
                "IP Generate, unable to find python interpreter in local "
                "environment, using system copy '" +
                python3Path.string() +
                "'. Some IP Catalog features might not work with this "
                "interpreter.\n");
          }
        }

        StringVector args{executable.string(), "--json",
                          FileUtils::GetFullPath(jsonFile).string(),
                          "--json-template"};
        std::ostringstream help;
        auto exitStatus =
            FileUtils::ExecuteSystemCommand(pythonPath.string(), args, &help)
                .code;
        if (exitStatus != 0) {
          qWarning()
              << QString{"Command failed: %1 %2 with exit status %3"}.arg(
                     QString::fromStdString(pythonPath.string()),
                     QString::fromStdString(StringUtils::join(args, " ")),
                     QString::number(exitStatus));
          ok = false;
          return {};
        }
        newJson = help.str();
        break;
      }
    }
  }
  return {newJson, executable.string()};
}

void IpConfigWidget::genarateNewPanel(const std::string& newJson,
                                      const std::string& filePath) {
  Compiler* compiler = GlobalSession->GetCompiler();
  auto generator = compiler->GetIPGenerator();
  IPCatalogBuilder builder(compiler);
  if (!builder.buildLiteXIPFromJson(generator->Catalog(), filePath, newJson)) {
    qWarning() << "Failed to parse new json";
    return;
  }
  CreateParamFields(false);
}

void IpConfigWidget::restoreProperties(
    const QMap<QVariant, QVariant>& properties) {
  QList<QObject*> paramObjects =
      FOEDAG::getTargetObjectsFromLayout(paramsBox->layout());
  for (auto obj : paramObjects) {
    auto property = properties.value(obj->property("customId"), QVariant{});
    if (property.isValid()) {
      const QSignalBlocker blocker{obj};
      QLineEdit* lineEdit = qobject_cast<QLineEdit*>(obj);
      QCheckBox* checkBox = qobject_cast<QCheckBox*>(obj);
      QComboBox* comboBox = qobject_cast<QComboBox*>(obj);
      QSpinBox* spinBox = qobject_cast<QSpinBox*>(obj);
      QDoubleSpinBox* spinBoxD = qobject_cast<QDoubleSpinBox*>(obj);
      bool applyProperty{true};
      if (lineEdit) {
        auto prevValue = lineEdit->text();
        lineEdit->setText(property.toString());
        if (!lineEdit->hasAcceptableInput()) {
          lineEdit->setText(prevValue);
          property = prevValue;
        }
        if (prevValue != lineEdit->text()) {  // text changed
          FOEDAG::validateLineEdit(lineEdit);
        }
      } else if (checkBox) {
        checkBox->setChecked(property.toInt() == Qt::Checked);
      } else if (comboBox) {
        comboBox->setCurrentText(property.toString());
      } else if (spinBox) {
        spinBox->setValue(property.toInt());
      } else if (spinBoxD) {
        spinBoxD->setValue(property.toDouble());
      } else {
        applyProperty = false;
      }
      // this need to be done since signals blocked and 'value' property will be
      // empty.
      if (applyProperty) obj->setProperty("value", property);
    }
  }
}

void IpConfigWidget::showInvalidParametersWarning() {
  QMessageBox::warning(this, tr("Invalid Parameter Value"),
                       tr("Atleast one invalid (red) parameter value found. "
                          "Reevaluate parameters before generating the IP."),
                       QMessageBox::Ok);
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

void IpConfigWidget::handleEditorChanged(const QString& customId,
                                         QWidget* widget) {
  // block signal otherwice it will be cicled
  const QSignalBlocker sBlocker{WidgetFactoryDependencyNotifier::Instance()};

  // save currect values
  bool valid{true};
  QMap<QVariant, QVariant> properties = saveProperties(valid);
  if (!valid) {
    showInvalidParametersWarning();
    return;
  }

  // save currect values as json
  bool ok{true};
  const auto& [newJson, filePath] = generateNewJson(ok);
  if (ok) {
    // receive new json and rebuild gui
    genarateNewPanel(newJson, filePath);

    // restore values
    restoreProperties(properties);
  } else {
    showInvalidParametersWarning();
  }
}

void IpConfigWidget::Generate(bool addToProject, const QString& outputPath) {
  // Find settings fields in the parameter box layout
  QLayout* fieldsLayout = paramsBox->layout();
  QList<QObject*> settingsObjs =
      FOEDAG::getTargetObjectsFromLayout(fieldsLayout);

  // Build up a parameter string based off the current UI fields
  QString params{};

  bool invalidVals = false;
  for (QObject* obj : settingsObjs) {
    // Collect parameters of fields that haven't been disabled by dipendencies
    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (widget) {
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
      QString arg =
          QString(" -P%1=%2").arg(obj->property("customId").toString(), val);
      params += arg;

      // check if any values are invalid
      invalidVals |= obj->property("invalid").toBool();
    }
  }

  // Alert the user if one or more of the field validators is invalid
  if (invalidVals) {
    showInvalidParametersWarning();
  } else {
    // If all enabled fields are valid, configure and generate IP
    std::filesystem::path baseDir(m_baseDirDefault.toStdString());
    std::filesystem::path outFile = baseDir / moduleEdit.text().toStdString();
    QString outFileStr =
        outputPath.isEmpty()
            ? QString::fromStdString(FileUtils::GetFullPath(outFile).string())
            : outputPath;

    // Build up a cmd string to generate the IP
    QString cmd = "configure_ip " + this->m_requestedIpName + " -mod_name " +
                  moduleEdit.text() + " -version " +
                  QString::fromStdString(m_meta.version) + " " + params +
                  " -out_file " + outFileStr;
    if (addToProject)
      cmd += "\nipgenerate -modules " + moduleEdit.text() + "\n";
    else
      cmd += " -template";

    int returnVal{false};
    auto resultStr =
        GlobalSession->TclInterp()->evalCmd(cmd.toStdString(), &returnVal);
    if (returnVal != TCL_OK) {
      qWarning() << "Error: " << QString::fromStdString(resultStr);
    }

    if (addToProject) {
      AddIpToProject(cmd);
      emit ipInstancesUpdated();
    }
  }
}
