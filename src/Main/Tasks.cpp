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

#include "Tasks.h"

#include <QDebug>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "Compiler/Reports/IDataReport.h"
#include "Compiler/Reports/ITaskReport.h"
#include "Compiler/Reports/ITaskReportManager.h"
#include "Compiler/Task.h"
#include "Compiler/TaskManager.h"
#include "Foedag.h"
#include "Main/JsonReportGenerator.h"
#include "ReportGenerator.h"
#include "TextEditor/text_editor_form.h"
#include "Utils/QtUtils.h"
#include "Utils/StringUtils.h"
#include "WidgetFactory.h"

#ifdef USE_IPA
#include "InteractivePathAnalysis/NCriticalPathModuleInfo.h"
#include "InteractivePathAnalysis/NCriticalPathWidget.h"
#endif  // USE_IPA

using json = nlohmann::ordered_json;
using namespace FOEDAG;

#define TASKS_KEY "Tasks"
#define SYNTH_ARG "_SynthOpt_"
#define TIMING_ANALYSIS_ARG "_StaOpt_"
#define PLACE_ARG "pin_assign_method"
#define PACKING_ARG "netlist_lang"

#define TASKS_DEBUG false

struct SettingHelper {
  QString settingKey;
  QString path;
};

namespace {

bool isReportTypeFile(const ITaskReport& report) {
  for (const auto& report : report.getDataReports()) {
    if (report->type() == DataReportType::File) {
      return true;
    }
  }
  return false;
}

std::unique_ptr<ReportGenerator> CreateGenerator(bool fileReport,
                                                 const ITaskReport& report,
                                                 QBoxLayout* layoput) {
  if (fileReport) {
    return std::move(
        CreateReportGenerator<OpenFileReportGenerator>(report, layoput));
  } else {
    return std::move(
        CreateReportGenerator<TableReportGenerator>(report, layoput));
  }
  return nullptr;
}

void openReportView(Compiler* compiler, const Task* task,
                    const ITaskReport& report) {
  auto reportName = report.getName();
  bool newReport{true};
  auto tabWidget = TextEditorForm::Instance()->GetTabWidget();
  for (int i = 0; i < tabWidget->count(); i++) {
    if (tabWidget->tabText(i) == reportName) {
      tabWidget->setCurrentIndex(i);
      auto reportLayout{
          dynamic_cast<QVBoxLayout*>(tabWidget->currentWidget()->layout())};

      // clear previous report
      QLayoutItem* item;
      while ((item = reportLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
      }

      auto reportGenerator =
          CreateGenerator(isReportTypeFile(report), report, reportLayout);
      if (reportGenerator) reportGenerator->Generate();

      newReport = false;
      break;
    }
  }

  if (newReport) {
    for (const auto& report : report.getDataReports()) {
      if (report->type() == DataReportType::File) {
        int result = TextEditorForm::Instance()->OpenFile(report->getName());
        if (result == 0) {
          return;
        }
      }
    }
    auto reportsWidget = new QWidget;
    auto reportLayout = new QVBoxLayout;
    reportLayout->setContentsMargins(0, 0, 0, 0);

    auto reportGenerator =
        CreateGenerator(isReportTypeFile(report), report, reportLayout);
    if (reportGenerator) reportGenerator->Generate();

    reportsWidget->setLayout(reportLayout);

    tabWidget->addTab(reportsWidget, reportName);
    tabWidget->setCurrentWidget(reportsWidget);

    QObject::connect(
        task, &Task::statusChanged, reportsWidget,
        [compiler, reportsWidget, reportName, task]() {
          auto tabWidget = TextEditorForm::Instance()->GetTabWidget();
          // Remove the report if underlying task status has changed
          if (auto index = tabWidget->indexOf(reportsWidget); index != -1) {
            tabWidget->removeTab(index);
            compiler->Message(reportName.toStdString() + " report closed.");
            reportsWidget->deleteLater();
          }
        });
  }
}

#ifdef USE_IPA
void openInteractivePathAnalysisView(Compiler* compiler) {
  bool newView{true};
  auto tabWidget = TextEditorForm::Instance()->GetTabWidget();
  for (int i = 0; i < tabWidget->count(); i++) {
    if (tabWidget->tabText(i) == NCRITICALPATH_UI_NAME) {
      tabWidget->setCurrentIndex(i);

      newView = false;
      break;
    }
  }

  if (newView) {
    std::filesystem::path staRootLocationPath(
        compiler->FilePath(Compiler::Action::STA));
    SimpleLogger::instance().setFilePath(
        QString::fromStdString(staRootLocationPath.string()) + "/" +
        NCRITICALPATH_INNER_NAME + ".log");
    std::filesystem::path ipaSettingsFilePath =
        staRootLocationPath / (std::string(NCRITICALPATH_INNER_NAME) + ".json");
    NCriticalPathWidget* viewWidget =
        new NCriticalPathWidget(compiler, ipaSettingsFilePath);

    viewWidget->setProperty("deleteOnCloseTab", true);
    tabWidget->addTab(viewWidget, NCRITICALPATH_UI_NAME);
    tabWidget->setCurrentWidget(viewWidget);
  }
}
#endif  // USE_IPA

}  // namespace

auto TASKS_DBG_PRINT = [](std::string printStr) {
  if (TASKS_DEBUG) {
    std::cout << printStr << std::flush;
  }
};

// Lookup for SynthOpt values
static QMap<FOEDAG::SynthesisOptimization, std::string> synthOptMap = {
    {FOEDAG::SynthesisOptimization::Area, "area"},
    {FOEDAG::SynthesisOptimization::Delay, "delay"},
    {FOEDAG::SynthesisOptimization::Mixed, "mixed"}};
// Lookup for PlaceOpt values
static QMap<FOEDAG::Compiler::PinAssignOpt, std::string> pinOptMap = {
    {FOEDAG::Compiler::PinAssignOpt::Random, "random"},
    {FOEDAG::Compiler::PinAssignOpt::In_Define_Order, "in_define_order"},
    {FOEDAG::Compiler::PinAssignOpt::Pin_constraint_disabled,
     "pin_constraint_disabled"}};
// Lookup for PackingOpt values
static QMap<FOEDAG::Compiler::NetlistType, std::string> netlistOptMap = {
    {FOEDAG::Compiler::NetlistType::Blif, "blif"},
    {FOEDAG::Compiler::NetlistType::EBlif, "eblif"},
    {FOEDAG::Compiler::NetlistType::Edif, "edif"},
    {FOEDAG::Compiler::NetlistType::VHDL, "vhdl"},
    {FOEDAG::Compiler::NetlistType::Verilog, "verilog"}};

static QMap<FOEDAG::ClbPacking, std::string> ClbPackingMap = {
    {FOEDAG::ClbPacking::Auto, "auto"},
    {FOEDAG::ClbPacking::Dense, "dense"},
    {FOEDAG::ClbPacking::Timing_driven, "timing_driven"}};

// This will grab Synthesis related options from Compiler::SynthOpt &
// Compiler::SynthMoreOpt, convert/combine them, and return them as an
// arg list QString
ArgumentsMap FOEDAG::TclArgs_getSynthesisOptions() {
  // Collect Synthesis Tcl Params
  // Syntehsis has one top level option that doesn't get passed with
  // SynthMoreOpt so we need to give it a fake arg and pass it
  ArgumentsMap arguments =
      parseArguments(GlobalSession->GetCompiler()->SynthMoreOpt());
  arguments.addArgument(
      SYNTH_ARG,
      synthOptMap.value(GlobalSession->GetCompiler()->SynthOptimization(),
                        std::string{}));
  return arguments;
};

// This will take an arg list, separate out the SynthOpt to set on the compiler
// and then set the rest of the options under SynthMoreOpt
void FOEDAG::TclArgs_setSynthesisOptions(const ArgumentsMap& args) {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (compiler) {
    ArgumentsMap arguments = args;
    auto [hasKey, synthOpt] = arguments.takeValue(SYNTH_ARG);
    if (hasKey) {
      auto val = FOEDAG::Compiler::SYNTH_OPT_DEFAULT;
      val = synthOptMap.key(synthOpt, val);
      compiler->SynthOptimization(val);
    }
    compiler->SynthMoreOpt(arguments.toString());
  }
};

ArgumentsMap FOEDAG::TclArgs_getPlacementOptions() {
  ArgumentsMap tclOptions =
      parseArguments(GlobalSession->GetCompiler()->PlaceMoreOpt());
  tclOptions.addArgument(
      PLACE_ARG, pinOptMap.value(GlobalSession->GetCompiler()->PinAssignOpts(),
                                 std::string{}));
  return tclOptions;
}

void FOEDAG::TclArgs_setPlacementOptions(const ArgumentsMap& argsStr) {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (compiler) {
    ArgumentsMap options = argsStr;
    auto [exists, pinArg] = options.takeValue(PLACE_ARG);
    if (exists) {
      auto PinAssignOpt = Compiler::PinAssignOpt::In_Define_Order;
      PinAssignOpt = pinOptMap.key(pinArg, PinAssignOpt);
      compiler->PinAssignOpts(PinAssignOpt);
    }
    compiler->PlaceMoreOpt(options.toString());
  }
}

// Hardcoded example callbacks to demonstrate how to use TclArgs with the task
// settings dialog
// NOTE: Do not do UI/integration (unit is ok) testing with this example as its
// initial hardcoding can make some settings aspects like loading saved values
// seem broken
static QString TclExampleArgs =
    "-double_spin_ex 3.3 -int_spin_ex 3 -radio_ex b3 -check_ex -dropdown_ex "
    "option3 -input_ex "
    "spaces_TclArgSpace_require_TclArgSpace_extra_TclArgSpace_formatting";

ArgumentsMap FOEDAG::TclArgs_getExampleArgs() {
  return parseArguments(TclExampleArgs.toStdString());
};
void FOEDAG::TclArgs_setExampleArgs(const ArgumentsMap& argsStr) {
  TclExampleArgs = QString::fromStdString(argsStr.toString());
};

QDialog* FOEDAG::createTaskDialog(const QString& taskName,
                                  const QString& path) {
  QString title = "Edit " + taskName + " Settings";
  QString prefix = "tasksDlg_" + taskName + "_";

  return FOEDAG::createSettingsDialog("/Tasks/" + taskName, path, title, prefix,
                                      {});
};

void FOEDAG::handleTaskDialogRequested(const QString& category,
                                       const QString& path) {
  QDialog* dlg = createTaskDialog(category, path);
  if (dlg) {
    dlg->exec();
  }

  if (GlobalSession->GetCompiler() &&
      GlobalSession->GetCompiler()->ProjManager()) {
    auto projPath = GlobalSession->GetCompiler()->ProjManager()->projectPath();
    if (projPath.empty()) return;
    auto synthPath =
        QU::ToQString(ProjectManager::projectSynthSettingsPath(projPath));
    auto implPath =
        QU::ToQString(ProjectManager::projectImplSettingsPath(projPath));
    const QVector<SettingHelper> dependencies{{SYNTH_SETTING_KEY, synthPath},
                                              {PACKING_SETTING_KEY, implPath}};
    if (std::any_of(dependencies.begin(), dependencies.end(),
                    [category](const SettingHelper& helper) {
                      return helper.settingKey == category;
                    })) {
      for (const auto& setting : dependencies) {
        if (setting.settingKey != category)
          GlobalSession->GetSettings()->syncWith(setting.settingKey,
                                                 setting.path);
      }
    }
  }
}

void FOEDAG::handleViewFileRequested(const QString& filePath) {
  QString path = filePath;
  path.replace(PROJECT_OSRCDIR, Project::Instance()->projectPath());
  TextEditorForm::Instance()->OpenFile(path);
}

void FOEDAG::handleViewReportRequested(Compiler* compiler, const Task* task,
                                       const QString& reportId,
                                       ITaskReportManager& reportManager) {
  auto report = reportManager.createReport(reportId);
  if (!report) return;

  openReportView(compiler, task, *report);
}

#ifdef USE_IPA
void FOEDAG::handleViewInteractivePathAnalysisRequested(Compiler* compiler) {
  openInteractivePathAnalysisView(compiler);
}
#endif  // USE_IPA

void TclArgs_setSimulateOptions(const std::string& simTypeStr,
                                Simulator::SimulationType simType,
                                const ArgumentsMap& argsStr) {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (!compiler) return;

  auto simulator{compiler->GetSimulator()};
  if (!simulator) return;

  auto restore = [](const std::string& str) {
    return restoreAll(QString::fromStdString(str)).toStdString();
  };

  const auto& [fileExists, file] = argsStr.value(simTypeStr + "_filepath");
  if (fileExists) {
    simulator->WaveFile(simType, restore(file));
  }

  const auto& [simTypeExists, simTypeValue] =
      argsStr.value(simTypeStr + "_sim_type");
  if (simTypeExists) {
    bool ok{false};
    auto simTool = Simulator::ToSimulatorType(simTypeValue, ok);
    if (ok) {
      simulator->UserSimulationType(simType, simTool);
    } else {
      qWarning() << "Not supported simulator: " << simTypeValue.c_str();
    }
  }

  Settings* settings = compiler->GetSession()->GetSettings();
  const std::map<std::string, json> settingsMap{
      {"rtl", settings->getJson()["Tasks"]["Simulate RTL"]["rtl_sim_type"]},
      {"gate", settings->getJson()["Tasks"]["Simulate Gate"]["gate_sim_type"]},
      {"pnr", settings->getJson()["Tasks"]["Simulate PNR"]["pnr_sim_type"]},
      {"bitstream", settings->getJson()["Tasks"]["Simulate Bitstream"]
                                       ["bitstream_sim_type"]}};

  using SetFunction =
      std::function<void(Simulator*, const std::string&,
                         Simulator::SimulatorType, const std::string&)>;

  auto applyOptions = [&settingsMap, simulator](const std::string& args,
                                                SetFunction setter,
                                                const std::string& level) {
    auto json = settingsMap.at(level);
    const std::string unset{"<unset>"};
    std::string simulatorStr = unset;
    if (json.contains("userValue")) {
      simulatorStr = json["userValue"];
    } else if (json.contains("default")) {
      simulatorStr = json["default"].get<std::string>();
    }
    if (simulatorStr != unset) {
      simulatorStr =
          Settings::getLookupValue(json, QString::fromStdString(simulatorStr))
              .toStdString();
    }

    if (setter) {
      bool ok{false};
      auto simulatorType = Simulator::ToSimulatorType(simulatorStr, ok);
      if (ok) setter(simulator, level, simulatorType, args);
    }
  };

  const auto& [runExists, runStr] = argsStr.value("run_" + simTypeStr + "_opt");
  if (runExists) {
    applyOptions(restore(runStr), &Simulator::SetSimulatorExtraOption,
                 simTypeStr);
  }
  const auto& [simExists, simStr] = argsStr.value("sim_" + simTypeStr + "_opt");
  if (simExists) {
    applyOptions(restore(simStr), &Simulator::SetSimulatorSimulationOption,
                 simTypeStr);
  }
  const auto& [elExists, elStr] = argsStr.value("el_" + simTypeStr + "_opt");
  if (elExists) {
    applyOptions(restore(elStr), &Simulator::SetSimulatorElaborationOption,
                 simTypeStr);
  }
  const auto& [comExists, comStr] = argsStr.value("com_" + simTypeStr + "_opt");
  if (comExists) {
    applyOptions(restore(comStr), &Simulator::SetSimulatorCompileOption,
                 simTypeStr);
  }
}

ArgumentsMap TclArgs_getSimulateOptions(const std::string& simTypeStr,
                                        Simulator::SimulationType simType) {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (!compiler) return {};

  auto simulator{compiler->GetSimulator()};
  auto convertSpecialChars = [](const std::string& str) -> std::string {
    return convertAll(QString::fromStdString(str)).toStdString();
  };

  ArgumentsMap argsList;

  argsList.addArgument(simTypeStr + "_filepath",
                       convertSpecialChars(simulator->WaveFile(simType)));

  bool ok{false};
  auto simTypeTmp{simulator->UserSimulationType(simType, ok)};
  std::string simulatorType{};
  if (ok) {
    simulatorType = Simulator::ToString(simTypeTmp);
    argsList.addArgument(simTypeStr + "_sim_type", simulatorType);
  }

  auto pushBackSimulationOptions = [&](const std::string& simType,
                                       const std::string& levelStr,
                                       Simulator::SimulationType levelValue) {
    bool ok{false};
    auto simulatorType = Simulator::ToSimulatorType(simType, ok);
    if (ok) {
      auto tmp = simulator->GetSimulatorExtraOption(levelValue, simulatorType);
      tmp = convertSpecialChars(tmp);
      if (!tmp.empty()) {
        argsList.addArgument("run_" + levelStr + "_opt", tmp);
      }

      tmp = simulator->GetSimulatorSimulationOption(levelValue, simulatorType);
      tmp = convertSpecialChars(tmp);
      if (!tmp.empty()) {
        argsList.addArgument("sim_" + levelStr + "_opt", tmp);
      }

      tmp = simulator->GetSimulatorElaborationOption(levelValue, simulatorType);
      tmp = convertSpecialChars(tmp);
      if (!tmp.empty()) {
        argsList.addArgument("el_" + levelStr + "_opt", tmp);
      }

      tmp = simulator->GetSimulatorCompileOption(levelValue, simulatorType);
      tmp = convertSpecialChars(tmp);
      if (!tmp.empty()) {
        argsList.addArgument("com_" + levelStr + "_opt", tmp);
      }
    }
  };

  pushBackSimulationOptions(simulatorType, simTypeStr, simType);

  return argsList;
}

// This will get Compiler::TimingAnalysisOpt and return an arg string for
// widgetFactory values
ArgumentsMap FOEDAG::TclArgs_getTimingAnalysisOptions() {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (!compiler) return {};

  // Timing Analysis currently only has 1 option for timing engine, if it's not
  // OpenSta then assume None/tatum.
  // Note: "tatum" is only used by widgetFactory. The compiler interface assumes
  // tatum any time Opensta isn't set
  std::string val = "tatum";
  if (compiler->TimingAnalysisEngineOpt() == Compiler::STAEngineOpt::Opensta) {
    val = "opensta";
  }
  ArgumentsMap args{};
  args.addArgument(TIMING_ANALYSIS_ARG, val);
  return args;
};

// This will take an arg list and set the TimingAnalysisOpt off it
void FOEDAG::TclArgs_setTimingAnalysisOptions(const ArgumentsMap& argsStr) {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (!compiler) return;

  const auto& [exists, engineArg] = argsStr.value(TIMING_ANALYSIS_ARG);

  // Determine and set Timing Engine
  auto engineVal = Compiler::STAEngineOpt::Tatum;  // default to VPR/tatum
  if (exists) {
    if (engineArg == "opensta") {
      engineVal = Compiler::STAEngineOpt::Opensta;
    }
  }
  compiler->TimingAnalysisEngineOpt(engineVal);
};

void FOEDAG::TclArgs_setSimulateOptions_rtl(const ArgumentsMap& argsStr) {
  TclArgs_setSimulateOptions("rtl", Simulator::SimulationType::RTL, argsStr);
}

ArgumentsMap FOEDAG::TclArgs_getSimulateOptions_rtl() {
  return TclArgs_getSimulateOptions("rtl", Simulator::SimulationType::RTL);
}

void FOEDAG::TclArgs_setSimulateOptions_gate(const ArgumentsMap& argsStr) {
  TclArgs_setSimulateOptions("gate", Simulator::SimulationType::Gate, argsStr);
}

ArgumentsMap FOEDAG::TclArgs_getSimulateOptions_gate() {
  return TclArgs_getSimulateOptions("gate", Simulator::SimulationType::Gate);
}

void FOEDAG::TclArgs_setSimulateOptions_pnr(const ArgumentsMap& argsStr) {
  TclArgs_setSimulateOptions("pnr", Simulator::SimulationType::PNR, argsStr);
}

ArgumentsMap FOEDAG::TclArgs_getSimulateOptions_pnr() {
  return TclArgs_getSimulateOptions("pnr", Simulator::SimulationType::PNR);
}

void FOEDAG::TclArgs_setSimulateOptions_bitstream(const ArgumentsMap& argsStr) {
  TclArgs_setSimulateOptions(
      "bitstream", Simulator::SimulationType::BitstreamBackDoor, argsStr);
}

ArgumentsMap FOEDAG::TclArgs_getSimulateOptions_bitstream() {
  return TclArgs_getSimulateOptions(
      "bitstream", Simulator::SimulationType::BitstreamBackDoor);
}

void FOEDAG::TclArgs_setPackingOptions(const ArgumentsMap& argsStr) {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (!compiler) return;

  const auto& [exists, netlistArg] = argsStr.value(PACKING_ARG);
  if (exists) {
    auto netlistVal = Compiler::NetlistType::Verilog;
    netlistVal = netlistOptMap.key(netlistArg, netlistVal);
    compiler->SetNetlistType(netlistVal);
  }

  ClbPacking clbPacking{ClbPacking::Auto};
  const auto& [clbPackExists, clbPackingStr] = argsStr.value("clb_packing");
  if (clbPackExists) {
    clbPacking = ClbPackingMap.key(clbPackingStr, clbPacking);
  }
  compiler->ClbPackingOption(clbPacking);
}

ArgumentsMap FOEDAG::TclArgs_getPackingOptions() {
  ArgumentsMap tclOptions;
  auto compiler = GlobalSession->GetCompiler();
  if (compiler) {
    tclOptions.addArgument(PACKING_ARG,
                           netlistOptMap.value(compiler->GetNetlistType()));

    if (ClbPackingMap.contains(compiler->ClbPackingOption())) {
      tclOptions.addArgument("clb_packing",
                             ClbPackingMap.value(compiler->ClbPackingOption()));
    }
  }
  return tclOptions;
}

void FOEDAG::handleJsonReportGeneration(Task* t, TaskManager* tManager,
                                        const QString& projectPath) {
  auto id = tManager->taskId(t);
  auto reportManager =
      tManager->getReportManagerRegistry().getReportManager(id);
  if (reportManager) {
    QString taskName{};
    if (id == SYNTHESIS)
      taskName = "synth";
    else if (id == ROUTING)
      taskName = "route";
    else if (id == TIMING_SIGN_OFF)
      taskName = "sta";
    else if (id == PLACEMENT)
      taskName = "place";
    else if (id == PACKING)
      taskName = "packing";
    const QSignalBlocker blocker{*reportManager};
    if (auto utilId =
            reportManager->getReportIdByType(ReportIdType::Utilization);
        !utilId.isEmpty()) {
      auto report = reportManager->createReport(utilId);
      auto jsonGenerator = CreateReportGenerator<JsonReportGenerator>(
          *report, taskName + "_utilization", projectPath);
      if (jsonGenerator) jsonGenerator->Generate();
    }
    if (auto statId = reportManager->getReportIdByType(ReportIdType::Statistic);
        !statId.isEmpty()) {
      auto report = reportManager->createReport(statId);
      auto jsonGenerator = CreateReportGenerator<JsonReportGenerator>(
          *report, taskName + "_design_stat", projectPath);
      if (jsonGenerator) jsonGenerator->Generate();
    }
  }
}
