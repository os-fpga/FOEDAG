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
#include "IpConfigurator/IpConfigDlg.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "Main/WidgetFactory.h"
#include "MainWindow/Session.h"
#include "MainWindow/main_window.h"
#include "NewProject/ProjectManager/project_manager.h"

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

IpConfigDlg::IpConfigDlg(QWidget* parent /*nullptr*/,
                         QString requestedIpName /* "" */,
                         QString moduleName /* "" */,
                         QStringList instanceValueArgs /*{}*/)
    : m_requestedIpName(requestedIpName),
      m_instanceValueArgs(instanceValueArgs) {
  this->setWindowTitle("Configure IP");
  this->setObjectName("IpConfigDlg");
  m_baseDirDefault = getUserProjectPath("IPs");

  // Set the path related widgets' tooltips to whatever their text is so long
  // paths are easier to view
  QObject::connect(
      &outputPath, &QLineEdit::textChanged,
      [this](const QString& text) { outputPath.setToolTip(text); });

  // Main Layout
  QVBoxLayout* topLayout = new QVBoxLayout();
  this->setLayout(topLayout);

  // Add VLNV meta text description
  topLayout->addWidget(&metaLabel);

  // Fill and add Parameters box
  CreateParamFields();
  topLayout->addWidget(&paramsBox);

  // Add Output Box
  CreateOutputFields();
  topLayout->addWidget(&outputBox);
  // Update the module name if one was passed (this occurs during a
  // re-configure)
  if (!moduleName.isEmpty()) {
    moduleEdit.setText(moduleName);
  }

  // Add Dialog Buttons
  topLayout->addStretch();
  AddDialogControls(topLayout);

  // Update output path now that meta data has been loaded
  updateOutputPath();
}

void IpConfigDlg::AddDialogControls(QBoxLayout* layout) {
  // Dialog Buttons
  QDialogButtonBox* btns = new QDialogButtonBox(QDialogButtonBox::Cancel);
  btns->setObjectName("IpConfigDlg_QDialogButtonBox");
  layout->addWidget(btns);
  QPushButton* generateBtn = new QPushButton("Generate IP", this);
  btns->addButton(generateBtn, QDialogButtonBox::ButtonRole::ActionRole);

  // Create our tcl command to generate the IP when the Generate IP button
  // is clicked
  QObject::connect(generateBtn, &QPushButton::clicked, this, [this]() {
    // Find settings fields in the parameter box layout
    QLayout* fieldsLayout = paramsBox.layout();
    QList<QObject*> settingsObjs =
        FOEDAG::getTargetObjectsFromLayout(fieldsLayout);

    // Build up a parameter string based off the current UI fields
    QString params = "";
    for (QObject* obj : settingsObjs) {
      // Add a space before each param except the first
      if (!params.isEmpty()) {
        params += " ";
      }

      // convert tclArg property from "name value" to "name=value"
      params += obj->property("tclArg").toString().replace(" ", "=");
    }

    // Build up a cmd string to generate the IP
    QString cmd = "configure_ip " + this->m_requestedIpName + " -mod_name " +
                  moduleEdit.text() + " -version " +
                  QString::fromStdString(m_meta.version) + " " + params +
                  " -out_file " + outputPath.text();
    cmd += "\nipgenerate";

    GlobalSession->TclInterp()->evalCmd(cmd.toStdString());
    // TODO @skyler-rs Sept2022 remove below test print once once command format
    // is finalized
    std::cout << cmd.toStdString() << std::endl;

    // Update the Ip Instances in the source tree
    MainWindow* win = qobject_cast<MainWindow*>(GlobalSession->MainWindow());
    if (win) {
      // TODO @skyler-rs Sept2022 change this to a slot/signal implementation
      // once this dlg is changed to a dock widget in the next sprint
      win->updateSourceTree();
    }
  });

  // Forward standard QDialogButtonBox signals to the parent dialog
  QObject::connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
  QObject::connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

void IpConfigDlg::CreateParamFields() {
  QStringList tclArgList;
  json parentJson;
  // Loop through IPDefinitions stored in IPCatalog
  for (auto def : getDefinitions()) {
    // if this definition is for the requested IP
    if (m_requestedIpName.toStdString() == def->Name()) {
      // Store VLNV meta data for the requested IP
      m_meta = FOEDAG::getIpInfoFromPath(def->FilePath());
      updateMetaLabel(m_meta);

      // set default module name to the BuildName provided by the generate
      // script otherwise default to the to the VLNV name
      std::string build_name = def->BuildName();
      if (build_name.empty()) {
        build_name = m_meta.name;
      }
      moduleEdit.setText(QString::fromStdString(build_name));

      // Build widget factory json for each parameter
      for (auto param : def->Parameters()) {
        json childJson;
        // Add P to the arg as the configure_ip format is -P{ARG_NAME}
        childJson["arg"] = "P" + param->Name();

        childJson["label"] =
            QString::fromStdString(param->Name()).toStdString();
        std::string defaultValue;

        // Currently all fields are input/linedit
        childJson["widgetType"] = "input";

        switch (param->GetType()) {
          case Value::Type::ParamInt:
            defaultValue = std::to_string(param->GetValue());
            // set validator so this field only accepts integer values
            childJson["validator"] = "int";
            // Note: Do not set "default" as the value will be passed in
            // tclArgList and passing default as well a tclArg will result in
            // the value not registering as a diff when set and the tclArg
            // property won't be set in widgetFactory
            break;
          case Value::Type::ParamString:
            defaultValue = param->GetSValue();
            break;
          case Value::Type::ConstInt:
            // set validator so this field only accepts integer values
            childJson["validator"] = "int";
            defaultValue = std::to_string(param->GetValue());
            break;
          default:
            defaultValue = std::to_string(param->GetValue());
            break;
        }
        parentJson[param->Name()] = childJson;

        // Create a list of tcl defaults that will be passed to createWidget
        tclArgList << QString("-P%1 %2")
                          .arg(QString::fromStdString(param->Name()))
                          .arg(QString::fromStdString(defaultValue));
      }
    }
  }

  // Use passed args if we are updating an IP instance
  if (!m_instanceValueArgs.isEmpty()) {
    tclArgList = m_instanceValueArgs;
  }

  // Create and add the child widget to our parent container

  auto form = createWidgetFormLayout(parentJson, tclArgList);
  paramsBox.setLayout(form);
}

void IpConfigDlg::CreateOutputFields() {
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
  int count = 0;
  for (auto [labelName, widget] : pairs) {
    form->addRow(QString::fromStdString(labelName), widget);
    count++;
  }

  // Update the output dir when module name changes
  QObject::connect(&moduleEdit, &QLineEdit::textChanged, this,
                   &IpConfigDlg::updateOutputPath);

  // add the layout to the output group box
  outputBox.setLayout(form);
}

void IpConfigDlg::updateMetaLabel(VLNV info) {
  // Create a descriptive sentence that lists all the VLNV info
  QString verStr = QString::fromStdString(info.version).replace("_", ".");
  std::string text = "<em>Configuring " + info.name + " (" +
                     verStr.toStdString() + ")" + " from " + info.vendor +
                     "'s " + info.library + " library</em>";
  metaLabel.setTextFormat(Qt::RichText);
  metaLabel.setText(QString::fromStdString(text));
  metaLabel.setWordWrap(true);
}

// Returns the IPDefinitions stored in the current IPGenerator's IPCatalog
std::vector<FOEDAG::IPDefinition*> IpConfigDlg::getDefinitions() {
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

void IpConfigDlg::updateOutputPath() {
  // Strip end separator from baseDir if there is one
  QString baseDir = m_baseDirDefault;
  if (baseDir.endsWith(SEPARATOR)) {
    baseDir.chop(SEPARATOR.length());
  }

  outputPath.setText(baseDir);
}
