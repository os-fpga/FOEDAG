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
#include "IPDialogBox.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QGridLayout>
#include <QMessageBox>
#include <QPainter>
#include <QScrollArea>
#include <QUrl>

#include "IPGenerate/IPCatalogBuilder.h"
#include "Main/WidgetFactory.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/DesignFileWatcher.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Utils/FileUtils.h"
#include "Utils/JsonWriter.h"
#include "Utils/StringUtils.h"
#include "nlohmann_json/json.hpp"
#include "ui_IPDialogBox.h"

extern FOEDAG::Session* GlobalSession;

using json = nlohmann::ordered_json;
inline void initializeResources() { Q_INIT_RESOURCE(ip_resources); }
namespace FOEDAG {

class ImageViewer : public QObject {
 public:
  explicit ImageViewer(const QString& img) : image(img) {}
  void setImage(const QString& img) { image = img; }

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override {
    if (event->type() == QEvent::MouseButtonPress) {
      auto pixmap = QImage{image};
      if (!label && !pixmap.isNull()) {
        label = new QLabel{};
        label->setWindowTitle("Image");
        label->setAttribute(Qt::WA_DeleteOnClose);
        label->setWindowFlags(Qt::WindowCloseButtonHint);
        label->setWindowModality(Qt::ApplicationModal);
        pixmap = pixmap.scaled({800, 1200}, Qt::KeepAspectRatio);
        label->setPixmap(QPixmap::fromImage(pixmap));
        label->show();
        label->setFixedSize(label->size());
        connect(label, &QLabel::destroyed, this, [this]() { label = nullptr; });
      }
      if (label) label->move(QCursor::pos() - label->rect().center());
    }
    return QObject::eventFilter(watched, event);
  }
  QString image;
  QLabel* label{nullptr};
};

std::filesystem::path getUserProjectPath() {
  return ProjectManager::projectIPsPath(
      GlobalSession->GetCompiler()->ProjManager()->projectPath());
}

IPDialogBox::IPDialogBox(const DeviceParameters& deviceInfo, QWidget* parent,
                         const QString& requestedIpName,
                         const QString& moduleName,
                         const QStringList& instanceValueArgs)
    : QDialog(parent),
      ui(new Ui::IPDialogBox),
      m_requestedIpName(requestedIpName),
      m_paramsBox(new QWidget),
      m_instanceValueArgs(instanceValueArgs),
      m_deviceParameters(deviceInfo) {
  initializeResources();

  ui->setupUi(this);

  connect(ui->cancel, &QPushButton::clicked, this, &IPDialogBox::reject);
  connect(ui->generateIp, &QPushButton::clicked, this,
          &IPDialogBox::GenerateIp);
  connect(ui->restoreDefaults, &QPushButton::clicked, this,
          &IPDialogBox::RestoreToDefaults);
  connect(ui->documentation, &QPushButton::clicked, this,
          &IPDialogBox::OpenDocumentaion);
  connect(ui->ipLocation, &QPushButton::clicked, this,
          &IPDialogBox::OpenIpLocation);

  if (moduleName.isEmpty()) {
    if (auto def = getDefinition(m_requestedIpName.toStdString()); def) {
      auto meta = FOEDAG::getIpInfoFromPath(def->FilePath());
      ui->labelName->setText(QString::fromStdString(meta.name));

      std::string build_name = def->BuildName();
      if (build_name.empty()) {
        build_name = meta.name;
      }
      ui->lineEditModuleName->setText(QString::fromStdString(build_name));
    }
  } else {
    ui->lineEditModuleName->setText(moduleName);
  }
  ui->paramsBox->setLayout(new QHBoxLayout);
  ui->paramsBox->layout()->setContentsMargins(0, 0, 0, 0);

  // Fill and add Parameters box
  CreateParamFields(false);

  if (!requestedIpName.isEmpty()) handleEditorChanged({}, nullptr);

  setWindowTitle("Configure IP");
  ui->lineEditModuleName->setValidator(
      new QRegularExpressionValidator{QRegularExpression{"^[a-zA-Z0-9_]*$"}});
  ui->labelImage->setMouseTracking(true);
}

IPDialogBox::~IPDialogBox() {}

QString IPDialogBox::ModuleName() const {
  return ui->lineEditModuleName->text();
}

std::string IPDialogBox::ModuleNameStd() const {
  return ui->lineEditModuleName->text().toStdString();
}

void IPDialogBox::OpenDocumentaion() {
  if (auto def = getDefinition(m_requestedIpName.toStdString()); def) {
    auto filePath = def->FilePath().parent_path();
    filePath /= "docs";
    auto doc = FileUtils::FindFileByExtension(filePath, ".pdf");
    if (doc.empty()) {
      QMessageBox::warning(this, ui->documentation->windowTitle(),
                           "Failed to find documentation");
    } else {
      QDesktopServices::openUrl(
          QUrl::fromLocalFile(QString::fromStdString(doc.string())));
    }
  }
}

void IPDialogBox::OpenIpLocation() {
  if (auto def = getDefinition(m_requestedIpName.toStdString()); def) {
    auto filePath = def->FilePath().parent_path();
    auto url = QUrl::fromLocalFile(QString::fromStdString(filePath.string()));
    if (!QDesktopServices::openUrl(url)) {
      QMessageBox::warning(
          this, ui->ipLocation->windowTitle(),
          "Failed to open " + QString::fromStdString(filePath.string()));
    }
  }
}

void IPDialogBox::RestoreToDefaults() {
  auto answer = QMessageBox::question(
      this, ui->restoreDefaults->text(),
      "Restore to default all parameters. Do you want to continue?");
  if (answer == QMessageBox::Yes) {
    const QSignalBlocker blocker{WidgetFactoryDependencyNotifier::Instance()};
    CreateParamFields(true);
  }
}

void IPDialogBox::GenerateIp() {
  const bool success = Generate(true);
  if (success) accept();
}

void IPDialogBox::handleEditorChanged(const QString& customId,
                                      QWidget* widget) {
  // block signal otherwice it will be cicled
  const QSignalBlocker sBlocker{WidgetFactoryDependencyNotifier::Instance()};

  // save currect values
  bool valid{true};
  auto properties = saveProperties(valid);
  if (!valid) {
    showInvalidParametersWarning(this);
    return;
  }

  // save currect values as json
  bool ok{true};
  Compiler* compiler = GlobalSession->GetCompiler();
  auto generator = compiler->GetIPGenerator();
  std::filesystem::path baseDir{generator->GetTmpPath()};
  std::filesystem::path outFile = baseDir / ModuleNameStd();
  QString outFileStr =
      QString::fromStdString(FileUtils::GetFullPath(outFile).string());
  Generate(false, outFileStr);
  const auto& [newJson, filePath] =
      generateNewJson(m_requestedIpName, m_deviceParameters, ok);
  if (ok) {
    // receive new json and rebuild gui
    genarateNewPanel(newJson, filePath);

    // restore values
    restoreProperties(properties);
    ui->labelSummary->setText(GenerateSummary(newJson));
    auto instances = generator->IPInstances();
    auto foundIp = std::find_if(
        instances.begin(), instances.end(), [this](IPInstance* inst) {
          return inst->IPName() == m_requestedIpName.toStdString();
        });
    if (foundIp != instances.end())
      LoadImage(generator->GetTmpCachePath(*foundIp).parent_path());
  } else {
    showInvalidParametersWarning(this);
  }
}

std::vector<IPDefinition*> IPDialogBox::getDefinitions() {
  // This just checks at each getter step to make sure no nulls are returned
  if (GlobalSession && GlobalSession->GetCompiler() &&
      GlobalSession->GetCompiler()->GetIPGenerator() &&
      GlobalSession->GetCompiler()->GetIPGenerator()->Catalog()) {
    return GlobalSession->GetCompiler()
        ->GetIPGenerator()
        ->Catalog()
        ->Definitions();
  }
  return {};
}

IPDefinition* IPDialogBox::getDefinition(const std::string& name) {
  for (auto def : getDefinitions())
    if (name == def->Name()) return def;
  return nullptr;
}

QString IPDialogBox::GenerateSummary(const std::string& newJson) {
  json object;
  try {
    object = json::parse(newJson);
  } catch (std::exception& e) {
    return {"No Summary"};
  }

  if (object.contains("Summary")) {
    QString table = "<table cellspacing=10>";
    auto summary = object.at("Summary");
    for (auto it = summary.begin(); it != summary.end(); it++) {
      QString row =
          R"(<tr><td align="left">%1:</td><td align="left">%2</td></tr>)";
      std::string value;
      if (it.value().is_string()) {
        value = it.value().get<std::string>();
      } else {
        std::ostringstream stringStream;
        stringStream << it.value();
        value = stringStream.str();
      }
      row = row.arg(QString::fromStdString(it.key()),
                    QString::fromStdString(value));
      table += row;
    }
    table += "</table>";
    return table;
  }
  return {"No Summary"};
}

sequential_map<QVariant, QVariant> IPDialogBox::saveProperties(
    bool& valid) const {
  QLayout* fieldsLayout = m_paramsBox->layout();
  QList<QObject*> settingsObjs =
      FOEDAG::getTargetObjectsFromLayout(fieldsLayout);
  sequential_map<QVariant, QVariant> properties{};

  for (QObject* obj : settingsObjs) {
    properties.push_back({obj->property("customId"), obj->property("value")});
    if (obj->property("invalid").toBool()) valid = false;
  }
  return properties;
}

void IPDialogBox::showInvalidParametersWarning(QWidget* parent) {
  QMessageBox::warning(parent, tr("Invalid Parameter Value"),
                       tr("Atleast one invalid (red) parameter value found. "
                          "Reevaluate parameters before generating the IP."),
                       QMessageBox::Ok);
}

std::pair<bool, QString> IPDialogBox::GetParams(
    const QList<QObject*>& settingsObjs) {
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
  return {invalidVals, params};
}

void IPDialogBox::restoreProperties(
    const sequential_map<QVariant, QVariant>& properties) {
  QList<QObject*> paramObjects =
      FOEDAG::getTargetObjectsFromLayout(m_paramsBox->layout());
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

void IPDialogBox::genarateNewPanel(const std::string& newJson,
                                   const std::string& filePath) {
  Compiler* compiler = GlobalSession->GetCompiler();
  auto generator = compiler->GetIPGenerator();
  IPCatalogBuilder builder(compiler);
  if (!builder.buildLiteXIPFromJson(generator->Catalog(), filePath, newJson)) {
    qWarning() << "Failed to parse new json";
    return;
  }
  CreateParamFields(true);
}

void IPDialogBox::CreateParamFields(bool generateParameres) {
  QStringList tclArgList;
  json parentJson;
  // Loop through IPDefinitions stored in IPCatalog
  if (auto def = getDefinition(m_requestedIpName.toStdString()); def) {
    // Build widget factory json for each parameter
    m_meta = FOEDAG::getIpInfoFromPath(def->FilePath());

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
        } else if (param->GetParamType() == IPParameter::ParamType::FilePath) {
          childJson["widgetType"] = "filepath";
        } else if (!param->GetOptions().empty()) {
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
            std::string rangeStr = " <span style=\"color:grey;\">[" + range[0] +
                                   ", " + range[1] + "]</span>";
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

  // Use passed args if we are updating an IP instance
  if (!m_instanceValueArgs.isEmpty()) {
    tclArgList = m_instanceValueArgs;
  }

  if (generateParameres) {
    if (m_scrollArea) {
      ui->paramsBox->layout()->removeWidget(m_scrollArea);
      m_scrollArea->deleteLater();
    }
    m_paramsBox = new QWidget{this};

    if (parentJson.empty()) {
      // Add a note if no parameters were available
      QVBoxLayout* layout = new QVBoxLayout();
      layout->addWidget(new QLabel("<em>This IP has no parameters</em>"));
      m_paramsBox->setLayout(layout);
    } else {
      // Create and add the child widget to our parent container
      auto form = createWidgetFormLayout(parentJson, tclArgList);
      m_paramsBox->setLayout(form);
    }
    m_scrollArea = new QScrollArea{};
    m_scrollArea->setWidget(m_paramsBox);
    ui->paramsBox->layout()->addWidget(m_scrollArea);
  }

  connect(WidgetFactoryDependencyNotifier::Instance(),
          &WidgetFactoryDependencyNotifier::editorChanged, this,
          &IPDialogBox::handleEditorChanged, Qt::UniqueConnection);
}

std::pair<std::string, std::string> IPDialogBox::generateNewJson(
    const QString& ipName, const DeviceParameters& deviceInfo, bool& ok) {
  Compiler* compiler = GlobalSession->GetCompiler();
  auto generator = compiler->GetIPGenerator();

  std::string newJson{};
  std::filesystem::path executable{};

  for (IPInstance* inst : generator->IPInstances()) {
    if (inst->IPName() != ipName.toStdString()) continue;

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
        JsonStreamWriter jsonF{jsonFile};
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
          jsonF.insert(param.Name(), value);
        }
        jsonF["build_dir"] = inst->OutputFile().parent_path().string();
        jsonF["build_name"] = inst->OutputFile().filename().string();
        jsonF["build"] = false;
        jsonF["json"] = jsonFile.filename().string();
        jsonF["json_template"] = false;
        jsonF["device"] = deviceInfo.deviceName.toStdString();
        jsonF["device_path"] = deviceInfo.deviceFile.string();
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

bool IPDialogBox::Generate(bool addToProject, const QString& outputPath) {
  // Find settings fields in the parameter box layout
  QLayout* fieldsLayout = m_paramsBox->layout();
  QList<QObject*> settingsObjs =
      FOEDAG::getTargetObjectsFromLayout(fieldsLayout);

  auto [invalidVals, params] = GetParams(settingsObjs);

  // Alert the user if one or more of the field validators is invalid
  if (invalidVals) {
    showInvalidParametersWarning(this);
    return false;
  } else {
    // If all enabled fields are valid, configure and generate IP
    std::filesystem::path baseDir(getUserProjectPath());
    std::filesystem::path outFile = baseDir / ModuleNameStd();
    QString outFileStr =
        outputPath.isEmpty()
            ? QString::fromStdString(FileUtils::GetFullPath(outFile).string())
            : outputPath;

    // Build up a cmd string to generate the IP
    QString cmd = "configure_ip " + this->m_requestedIpName + " -mod_name " +
                  ModuleName() + " -version " +
                  QString::fromStdString(m_meta.version) + " " + params +
                  " -out_file " + outFileStr;
    if (addToProject)
      cmd += "\nipgenerate -modules " + ModuleName() + "\n";
    else
      cmd += " -template";

    int returnVal{false};
    auto resultStr =
        GlobalSession->TclInterp()->evalCmd(cmd.toStdString(), &returnVal);
    if (returnVal != TCL_OK) {
      qWarning() << "Error: " << QString::fromStdString(resultStr);
      return false;
    }

    if (addToProject) {
      AddIpToProject(cmd);
    }
  }
  return true;
}

void IPDialogBox::AddIpToProject(const QString& cmd) {
  FOEDAG::Compiler* compiler = nullptr;
  ProjectManager* projManager = nullptr;
  if (GlobalSession && (compiler = GlobalSession->GetCompiler()) &&
      (projManager = compiler->ProjManager())) {
    auto getUniqueString = [](const QString& ipConfigCmd) -> QString {
      // Use the configure command's first half(ipName and module name) as a
      // unique id to determine if this IP configuration has already been stored
      QStringList cmdParts = ipConfigCmd.split("-mod_name");
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

QString IPDialogBox::outPath() const {
  std::filesystem::path baseDir(getUserProjectPath());
  std::filesystem::path vlnvPath =
      baseDir / m_meta.vendor / m_meta.library / m_meta.name / m_meta.version;

  // Add the module wrapper
  std::filesystem::path outPath = vlnvPath / ModuleNameStd();

  // Update the output path text
  return QString::fromStdString(FileUtils::GetFullPath(outPath).string());
}

void IPDialogBox::LoadImage(const std::filesystem::path& location) {
  auto filePath = location;
  filePath /= "docs";
  auto image = FileUtils::FindFileByExtension(filePath, ".png");
  if (!image.empty()) {
    if (!m_imageViewer) {
      m_imageViewer =
          std::make_unique<ImageViewer>(QString::fromStdString(image.string()));
      ui->labelImage->installEventFilter(m_imageViewer.get());
    } else {
      m_imageViewer->setImage(QString::fromStdString(image.string()));
    }
    auto pixmap = QPixmap{QString::fromStdString(image.string())};
    pixmap = pixmap.scaled({200, 300}, Qt::KeepAspectRatio);
    ui->labelImage->setPixmap(pixmap);
  } else {
    // ui->labelImage->setPixmap({});
    ui->labelImage->setText("No image");
  }
}

}  // namespace FOEDAG
