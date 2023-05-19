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

using json = nlohmann::ordered_json;
using namespace FOEDAG;

#define TASKS_KEY "Tasks"
#define SYNTH_ARG "_SynthOpt_"
#define TIMING_ANALYSIS_ARG "_StaOpt_"
#define PLACE_ARG "pin_assign_method"
#define PACKING_ARG "netlist_lang"

#define TASKS_DEBUG false

namespace {

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
          CreateReportGenerator<TableReportGenerator>(report, reportLayout);
      reportGenerator->Generate();

      newReport = false;
      break;
    }
  }

  if (newReport) {
    auto reportsWidget = new QWidget;
    auto reportLayout = new QVBoxLayout;
    reportLayout->setContentsMargins(0, 0, 0, 0);

    auto reportGenerator =
        CreateReportGenerator<TableReportGenerator>(report, reportLayout);
    reportGenerator->Generate();

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
}  // namespace

auto TASKS_DBG_PRINT = [](std::string printStr) {
  if (TASKS_DEBUG) {
    std::cout << printStr << std::flush;
  }
};

// Grab a specific arg and value from a list of args and return that specific
// pair as well as the rest of the args w/ that specifc arg removed
auto separateArg = [](const QString& argName,
                      const QString& argString) -> std::pair<QString, QString> {
  QString targetArg = "";
  QString otherArgs = argString;
  QString searchStr = argName;

  if (!searchStr.isEmpty()) {
    // prepend - if one doesn't exist
    if (searchStr[0] != "-") {
      searchStr = "-" + searchStr;
    }
    // Find the arg and remove it from the otherArgs
    auto argIdx = argString.indexOf(searchStr);
    if (argIdx != -1) {
      targetArg =
          argString.mid(argIdx, argString.indexOf("-", argIdx + 1) - argIdx);
      otherArgs = otherArgs.replace(targetArg, "");
    }
  }
  return {targetArg, otherArgs};
};

// Lookup for SynthOpt values
static std::map<FOEDAG::Compiler::SynthesisOpt, const char*> synthOptMap = {
    {FOEDAG::Compiler::SynthesisOpt::Area, "area"},
    {FOEDAG::Compiler::SynthesisOpt::Delay, "delay"},
    {FOEDAG::Compiler::SynthesisOpt::Mixed, "mixed"},
    {FOEDAG::Compiler::SynthesisOpt::Clean, "clean"}};
// Lookup for PlaceOpt values
static std::map<FOEDAG::Compiler::PinAssignOpt, const char*> pinOptMap = {
    {FOEDAG::Compiler::PinAssignOpt::Random, "random"},
    {FOEDAG::Compiler::PinAssignOpt::In_Define_Order, "in_define_order"},
    {FOEDAG::Compiler::PinAssignOpt::Free, "free"}};

// Lookup for PackingOpt values
static std::map<FOEDAG::Compiler::NetlistType, const char*> netlistOptMap = {
    {FOEDAG::Compiler::NetlistType::Blif, "blif"},
    {FOEDAG::Compiler::NetlistType::EBlif, "eblif"},
    {FOEDAG::Compiler::NetlistType::Edif, "edif"},
    {FOEDAG::Compiler::NetlistType::VHDL, "vhdl"},
    {FOEDAG::Compiler::NetlistType::Verilog, "verilog"}};

// Helper to convert a SynthesisOpt enum to string
auto synthOptToStr = [](FOEDAG::Compiler::SynthesisOpt opt) -> QString {
  return synthOptMap[opt];
};

// Helper to convert a string to SynthesisOpt enum
auto synthStrToOpt = [](const QString& str) -> FOEDAG::Compiler::SynthesisOpt {
  auto it = find_if(
      synthOptMap.begin(), synthOptMap.end(),
      [str](const std::pair<FOEDAG::Compiler::SynthesisOpt, const char*> p) {
        return p.second == str;
      });

  auto val = FOEDAG::Compiler::SYNTH_OPT_DEFAULT;
  if (it != synthOptMap.end()) {
    val = (*it).first;
  }

  return val;
};

auto pinOptToStr = [](FOEDAG::Compiler::PinAssignOpt opt) -> QString {
  return pinOptMap.at(opt);
};

auto pinStrToOpt = [](const QString& str) -> FOEDAG::Compiler::PinAssignOpt {
  auto it = find_if(
      pinOptMap.begin(), pinOptMap.end(),
      [str](const std::pair<FOEDAG::Compiler::PinAssignOpt, const char*> p) {
        return p.second == str;
      });

  auto val = FOEDAG::Compiler::PinAssignOpt::In_Define_Order;
  if (it != pinOptMap.end()) {
    val = (*it).first;
  }

  return val;
};
// This will grab Synthesis related options from Compiler::SynthOpt &
// Compiler::SynthMoreOpt, convert/combine them, and return them as an
// arg list QString
std::string FOEDAG::TclArgs_getSynthesisOptions() {
  // Collect Synthesis Tcl Params
  QString tclOptions =
      QString::fromStdString(GlobalSession->GetCompiler()->SynthMoreOpt());
  // Syntehsis has one top level option that doesn't get passed with
  // SynthMoreOpt so we need to give it a fake arg and pass it
  tclOptions += " -" + QString(SYNTH_ARG) + " " +
                synthOptToStr(GlobalSession->GetCompiler()->SynthOpt());
  return tclOptions.toStdString();
};

// This will take an arg list, separate out the SynthOpt to set on the compiler
// and then set the rest of the options under SynthMoreOpt
void FOEDAG::TclArgs_setSynthesisOptions(const std::string& argsStr) {
  auto [synthArg, moreOpts] =
      separateArg(SYNTH_ARG, QString::fromStdString(argsStr).trimmed());
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (compiler) {
    QStringList tokens = synthArg.split(" ");
    if (tokens.count() > 1) {
      compiler->SynthOpt(synthStrToOpt(tokens[1]));
    }
    compiler->SynthMoreOpt(moreOpts.toStdString());
  }
};

std::string FOEDAG::TclArgs_getPlacementOptions() {
  // Collect placement Tcl Params
  QString tclOptions =
      QString::fromStdString(GlobalSession->GetCompiler()->PlaceMoreOpt());
  tclOptions += " -" + QString(PLACE_ARG) + " " +
                pinOptToStr(GlobalSession->GetCompiler()->PinAssignOpts());
  return tclOptions.toStdString();
}

void FOEDAG::TclArgs_setPlacementOptions(const std::string& argsStr) {
  auto [pinArg, moreOpts] =
      separateArg(PLACE_ARG, QString::fromStdString(argsStr));
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (compiler) {
    QStringList tokens = pinArg.split(" ");
    if (tokens.count() > 1) {
      compiler->PinAssignOpts(pinStrToOpt(tokens[1]));
    }
    compiler->PlaceMoreOpt(moreOpts.toStdString());
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

std::string FOEDAG::TclArgs_getExampleArgs() {
  return TclExampleArgs.toStdString();
};
void FOEDAG::TclArgs_setExampleArgs(const std::string& argsStr) {
  TclExampleArgs = QString::fromStdString(argsStr);
};

QDialog* FOEDAG::createTaskDialog(const QString& taskName) {
  QString title = "Edit " + taskName + " Settings";
  QString prefix = "tasksDlg_" + taskName + "_";

  return FOEDAG::createSettingsDialog("/Tasks/" + taskName, title, prefix);
};

void FOEDAG::handleTaskDialogRequested(const QString& category) {
  QVector<QString> dependencies{SYNTH_SETTING_KEY, PACKING_SETTING_KEY};
  const bool sync{dependencies.contains(category)};
  dependencies.removeAll(category);
  QDialog* dlg = createTaskDialog(category);
  if (dlg) {
    dlg->exec();
  }

  if (sync) {
    for (const auto& setting : dependencies)
      GlobalSession->GetSettings()->syncWith(setting);
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

void TclArgs_setSimulateOptions(const std::string& simTypeStr,
                                Simulator::SimulationType simType,
                                const std::string& argsStr) {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (!compiler) return;

  auto simulator{compiler->GetSimulator()};

  std::vector<std::string> argsList;
  StringUtils::tokenize(argsStr, " ", argsList, true);

  for (int i = 0; i < argsList.size();) {
    std::string arg{argsList.at(i)};
    std::string value{};
    if ((i + 1) < argsList.size()) {
      std::string tmp{argsList.at(i + 1)};
      if (StringUtils::startsWith(tmp, "-")) {  // new argument
        i++;
      } else {  // argument value
        value = tmp;
        i += 2;
      }
    } else
      i++;

    value = restoreAll(QString::fromStdString(value)).toStdString();

    if (arg.compare("-" + simTypeStr + "_filepath") == 0)
      simulator->WaveFile(simType, value);

    std::pair<bool, Simulator::SimulationType> simTypeTmp{
        false, Simulator::SimulationType::RTL};
    if (arg.compare("-" + simTypeStr + "_sim_type") == 0) {
      simTypeTmp = {true, simType};
    }

    if (simTypeTmp.first) {
      bool ok{false};
      auto simTool = Simulator::ToSimulatorType(value, ok);
      if (ok) {
        simulator->UserSimulationType(simTypeTmp.second, simTool);
      } else {
        qWarning() << "Not supported simulator: " << value.c_str();
      }
    }

    Settings* settings = compiler->GetSession()->GetSettings();
    const std::map<std::string, json> settingsMap{
        {"rtl", settings->getJson()["Tasks"]["Simulate RTL"]["rtl_sim_type"]},
        {"gate",
         settings->getJson()["Tasks"]["Simulate Gate"]["gate_sim_type"]},
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

    if (arg.compare("-run_" + simTypeStr + "_opt") == 0) {
      applyOptions(value, &Simulator::SetSimulatorExtraOption, simTypeStr);
    }
    if (arg.compare("-sim_" + simTypeStr + "_opt") == 0) {
      applyOptions(value, &Simulator::SetSimulatorSimulationOption, simTypeStr);
    }
    if (arg.compare("-el_" + simTypeStr + "_opt") == 0) {
      applyOptions(value, &Simulator::SetSimulatorElaborationOption,
                   simTypeStr);
    }
    if (arg.compare("-com_" + simTypeStr + "_opt") == 0) {
      applyOptions(value, &Simulator::SetSimulatorCompileOption, simTypeStr);
    }
  }
}

std::string TclArgs_getSimulateOptions(const std::string& simTypeStr,
                                       Simulator::SimulationType simType) {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (!compiler) return std::string{};

  auto simulator{compiler->GetSimulator()};
  auto convertSpecialChars = [](const std::string& str) -> std::string {
    return convertAll(QString::fromStdString(str)).toStdString();
  };

  std::vector<std::string> argsList;
  argsList.push_back("-" + simTypeStr + "_filepath");
  argsList.push_back(convertSpecialChars(simulator->WaveFile(simType)));

  bool ok{false};
  auto simTypeTmp{simulator->UserSimulationType(simType, ok)};
  std::string simulatorType{};
  if (ok) {
    argsList.push_back("-" + simTypeStr + "_sim_type");
    simulatorType = Simulator::ToString(simTypeTmp);
    argsList.push_back(simulatorType);
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
        argsList.push_back("-run_" + levelStr + "_opt");
        argsList.push_back(tmp);
      }

      tmp = simulator->GetSimulatorSimulationOption(levelValue, simulatorType);
      tmp = convertSpecialChars(tmp);
      if (!tmp.empty()) {
        argsList.push_back("-sim_" + levelStr + "_opt");
        argsList.push_back(tmp);
      }

      tmp = simulator->GetSimulatorElaborationOption(levelValue, simulatorType);
      tmp = convertSpecialChars(tmp);
      if (!tmp.empty()) {
        argsList.push_back("-el_" + levelStr + "_opt");
        argsList.push_back(tmp);
      }

      tmp = simulator->GetSimulatorCompileOption(levelValue, simulatorType);
      tmp = convertSpecialChars(tmp);
      if (!tmp.empty()) {
        argsList.push_back("-com_" + levelStr + "_opt");
        argsList.push_back(tmp);
      }
    }
  };

  pushBackSimulationOptions(simulatorType, simTypeStr, simType);

  return StringUtils::join(argsList, " ");
}

// This will get Compiler::TimingAnalysisOpt and return an arg string for
// widgetFactory values
std::string FOEDAG::TclArgs_getTimingAnalysisOptions() {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (!compiler) return std::string{};

  // Timing Analysis currently only has 1 option for timing engine, if it's not
  // OpenSta then assume None/tatum.
  // Note: "tatum" is only used by widgetFactory. The compiler interface assumes
  // tatum any time Opensta isn't set
  std::string val = "tatum";
  if (compiler->TimingAnalysisEngineOpt() == Compiler::STAEngineOpt::Opensta) {
    val = "opensta";
  }
  std::string argStr = std::string("-") + TIMING_ANALYSIS_ARG + " " + val;
  return argStr;
};

// This will take an arg list and set the TimingAnalysisOpt off it
void FOEDAG::TclArgs_setTimingAnalysisOptions(const std::string& argsStr) {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (!compiler) return;

  // TimingAnalysis currently only has 1 option so moreOpts won't be used
  [[maybe_unused]] auto [engineArg, moreOpts] =
      separateArg(TIMING_ANALYSIS_ARG, QString::fromStdString(argsStr));

  // Determine and set Timing Engine
  auto engineVal = Compiler::STAEngineOpt::Tatum;  // default to VPR/tatum
  QStringList tokens = engineArg.split(" ");
  if (tokens.size() > 1) {
    if (tokens[1] == "opensta") {
      engineVal = Compiler::STAEngineOpt::Opensta;
    }
  }
  compiler->TimingAnalysisEngineOpt(engineVal);
};

void FOEDAG::TclArgs_setSimulateOptions_rtl(const std::string& argsStr) {
  TclArgs_setSimulateOptions("rtl", Simulator::SimulationType::RTL, argsStr);
}

std::string FOEDAG::TclArgs_getSimulateOptions_rtl() {
  return TclArgs_getSimulateOptions("rtl", Simulator::SimulationType::RTL);
}

void FOEDAG::TclArgs_setSimulateOptions_gate(const std::string& argsStr) {
  TclArgs_setSimulateOptions("gate", Simulator::SimulationType::Gate, argsStr);
}

std::string FOEDAG::TclArgs_getSimulateOptions_gate() {
  return TclArgs_getSimulateOptions("gate", Simulator::SimulationType::Gate);
}

void FOEDAG::TclArgs_setSimulateOptions_pnr(const std::string& argsStr) {
  TclArgs_setSimulateOptions("pnr", Simulator::SimulationType::PNR, argsStr);
}

std::string FOEDAG::TclArgs_getSimulateOptions_pnr() {
  return TclArgs_getSimulateOptions("pnr", Simulator::SimulationType::PNR);
}

void FOEDAG::TclArgs_setSimulateOptions_bitstream(const std::string& argsStr) {
  TclArgs_setSimulateOptions(
      "bitstream", Simulator::SimulationType::BitstreamBackDoor, argsStr);
}

std::string FOEDAG::TclArgs_getSimulateOptions_bitstream() {
  return TclArgs_getSimulateOptions(
      "bitstream", Simulator::SimulationType::BitstreamBackDoor);
}

void FOEDAG::TclArgs_setPackingOptions(const std::string& argsStr) {
  FOEDAG::Compiler* compiler = GlobalSession->GetCompiler();
  if (!compiler) return;

  [[maybe_unused]] auto [netlistArg, moreOpts] =
      separateArg(PACKING_ARG, QString::fromStdString(argsStr));

  auto netlistVal = Compiler::NetlistType::Verilog;
  const QStringList tokens = netlistArg.split(" ");
  if (tokens.size() > 1) {
    auto iter = std::find_if(
        netlistOptMap.begin(), netlistOptMap.end(),
        [tokens](const std::pair<Compiler::NetlistType, const char*> val) {
          return std::string{val.second} == tokens[1].toStdString();
        });
    if (iter != netlistOptMap.end()) {
      netlistVal = iter->first;
    }
  }
  compiler->SetNetlistType(netlistVal);

  ClbPacking clbPacking{ClbPacking::Auto};
  const QStringList moreOptsList = QtUtils::StringSplit(moreOpts, ' ');
  for (int i = 0; i < (moreOpts.size() - 1); i++)
    if (moreOptsList.at(i) == "-clb_packing") {
      if (moreOptsList.at(i + 1) == "auto") {
        clbPacking = ClbPacking::Auto;
      } else if (moreOptsList.at(i + 1) == "dense") {
        clbPacking = ClbPacking::Dense;
      } else if (moreOptsList.at(i + 1) == "timing_driven") {
        clbPacking = ClbPacking::Timing_driven;
      }
      break;
    }
  compiler->ClbPackingOption(clbPacking);
}

std::string FOEDAG::TclArgs_getPackingOptions() {
  QString tclOptions = QString{"-%1 %2"}.arg(
      PACKING_ARG, QString::fromStdString(netlistOptMap.at(
                       GlobalSession->GetCompiler()->GetNetlistType())));

  if (GlobalSession->GetCompiler()->ClbPackingOption() == ClbPacking::Auto) {
    tclOptions += " -clb_packing auto";
  } else if (GlobalSession->GetCompiler()->ClbPackingOption() ==
             ClbPacking::Dense) {
    tclOptions += " -clb_packing dense";
  } else if (GlobalSession->GetCompiler()->ClbPackingOption() ==
             ClbPacking::Timing_driven) {
    tclOptions += " -clb_packing timing_driven";
  }

  return tclOptions.toStdString();
}

void FOEDAG::handleJsonReportGeneration(Task* t, TaskManager* tManager,
                                        const QString& projectPath) {
  auto id = tManager->taskId(t);
  auto reportManager =
      tManager->getReportManagerRegistry().getReportManager(id);
  if (reportManager && reportManager->getAvailableReportIds().size() >= 2) {
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
    auto report = reportManager->createReport(
        reportManager->getAvailableReportIds().first());
    auto jsonGenerator = CreateReportGenerator<JsonReportGenerator>(
        *report, taskName + "_utilization", projectPath);
    if (jsonGenerator) jsonGenerator->Generate();

    report = reportManager->createReport(
        reportManager->getAvailableReportIds().last());
    jsonGenerator = CreateReportGenerator<JsonReportGenerator>(
        *report, taskName + "_design_stat", projectPath);
    if (jsonGenerator) jsonGenerator->Generate();
  }
}
