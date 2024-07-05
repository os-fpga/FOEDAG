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

#include "IPDialogBox.h"
#include "Main/WidgetFactory.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Utils/FileUtils.h"

using namespace FOEDAG;
extern FOEDAG::Session* GlobalSession;

#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

std::filesystem::path getUserProjectPath() {
  return ProjectManager::projectIPsPath(
      GlobalSession->GetCompiler()->ProjManager()->projectPath());
}

IpConfigWidget::IpConfigWidget(QWidget* parent /*nullptr*/,
                               const QString& requestedIpName /* "" */,
                               const QString& moduleName /* "" */)
    : m_paramsBox{new QGroupBox{"Parameters", this}},
      m_baseDirDefault{getUserProjectPath()},
      m_requestedIpName(requestedIpName) {
  this->setWindowTitle("IP Description/Details");
  this->setObjectName("IpConfigWidget");

  // Main Layout
  QVBoxLayout* topLayout = new QVBoxLayout();
  topLayout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(topLayout);

  // Create container widget and QScrollArea so this widget can shrink
  QWidget* containerWidget = new QWidget();
  containerWidget->setObjectName("ipConfigContainerWidget");
  auto containerLayout = new QVBoxLayout();
  // layout must be set before adding to the scroll area
  // https://doc.qt.io/qt-6/qscrollarea.html#setWidget
  containerWidget->setLayout(containerLayout);
  QScrollArea* scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  scrollArea->setObjectName("ipConfigScrollArea");
  scrollArea->setWidget(containerWidget);
  topLayout->addWidget(scrollArea);

  // Add VLNV meta text description
  containerLayout->addWidget(&m_metaLabel);

  // Fill and add Parameters box
  CreateParamFields();

  // Update the module name if one was passed (this occurs during a
  // re-configure)
  if (!moduleName.isEmpty()) this->m_moduleName = moduleName;

  // run with --json --json-template parameters to get default GUI
  if (!requestedIpName.isEmpty()) generateDetailedInformation();
}

void IpConfigWidget::CreateParamFields() {
  for (auto def : getDefinitions()) {
    // if this definition is for the requested IP
    if (m_requestedIpName.toStdString() == def->Name()) {
      // Store VLNV meta data for the requested IP
      m_meta = FOEDAG::getIpInfoFromPath(def->FilePath());
      Compiler* compiler = GlobalSession->GetCompiler();
      auto generator = compiler->GetIPGenerator();
      for (IPInstance* inst : generator->IPInstances()) {
        if (inst->IPName() != m_requestedIpName.toStdString()) continue;
        m_details = FOEDAG::readIpDetails(
            generator->GetTmpCachePath(inst).parent_path() / "details.json");
      }
      // set default module name to the BuildName provided by the generate
      // script otherwise default to the to the VLNV name
      std::string build_name = def->BuildName();
      if (build_name.empty()) {
        build_name = m_meta.name;
      }
      m_moduleName = QString::fromStdString(build_name);

      // Update meta label now that vlnv and module info is updated
      updateMetaLabel(m_details);
    }
  }
}

void IpConfigWidget::updateMetaLabel(const IPDetails& details) {
  QString text = R"(
<table cellspacing=10>
  <tr>
    <td align="left">Name:</th>
    <td align="left">%1</th>
  </tr>
  <tr>
    <td align="left">Version:</th>
    <td align="left">%2</th>
  </tr>
  <tr>
    <td align="left">Interface:</th>
    <td align="left">%3</th>
  </tr>
  <tr>
    <td align="left">Description:</th>
    <td align="left">%4</th>
  </tr>
</table>
)";

  text = text.arg(QString::fromStdString(details.name),
                  QString::fromStdString(details.version),
                  QString::fromStdString(details.interface_str),
                  QString::fromStdString(details.description));
  m_metaLabel.setTextFormat(Qt::RichText);
  m_metaLabel.setText(text);
  m_metaLabel.setWordWrap(true);
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

void IpConfigWidget::generateDetailedInformation() {
  // save currect values as json
  bool ok{true};
  Compiler* compiler = GlobalSession->GetCompiler();
  auto generator = compiler->GetIPGenerator();
  std::filesystem::path baseDir{generator->GetTmpPath()};
  std::filesystem::path outFile = baseDir / m_moduleName.toStdString();
  QString outFileStr =
      QString::fromStdString(FileUtils::GetFullPath(outFile).string());
  Generate(outFileStr);
  IPDialogBox::generateNewJson(m_requestedIpName, {}, ok);
  if (ok) {
    CreateParamFields();
  } else {
    IPDialogBox::showInvalidParametersWarning(this);
  }
}

void IpConfigWidget::Generate(const QString& outputPath) {
  // Find settings fields in the parameter box layout
  QLayout* fieldsLayout = m_paramsBox->layout();
  QList<QObject*> settingsObjs =
      FOEDAG::getTargetObjectsFromLayout(fieldsLayout);

  auto [invalidVals, params] = IPDialogBox::GetParams(settingsObjs);

  // Alert the user if one or more of the field validators is invalid
  if (invalidVals) {
    IPDialogBox::showInvalidParametersWarning(this);
  } else {
    // If all enabled fields are valid, configure and generate IP
    std::filesystem::path baseDir(m_baseDirDefault);
    std::filesystem::path outFile = baseDir / m_moduleName.toStdString();
    QString outFileStr =
        outputPath.isEmpty()
            ? QString::fromStdString(FileUtils::GetFullPath(outFile).string())
            : outputPath;

    // Build up a cmd string to generate the IP
    QString cmd = "configure_ip " + this->m_requestedIpName + " -mod_name " +
                  m_moduleName + " -version " +
                  QString::fromStdString(m_meta.version) + " " + params +
                  " -out_file " + outFileStr;
    cmd += " -template";

    int returnVal{false};
    auto resultStr =
        GlobalSession->TclInterp()->evalCmd(cmd.toStdString(), &returnVal);
    if (returnVal != TCL_OK) {
      qWarning() << "Error: " << QString::fromStdString(resultStr);
    }
  }
}
