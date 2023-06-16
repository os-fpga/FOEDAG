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

#include "CompilerDefines.h"

#include <QHeaderView>

#include "Compiler.h"
#include "Main/Tasks.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "Simulation/Simulator.h"
#include "TaskManager.h"
#include "TaskModel.h"
#include "TaskTableView.h"
#include "Utils/FileUtils.h"
#include "Utils/QtUtils.h"

extern FOEDAG::Session *GlobalSession;

namespace FOEDAG {

TaskTableView *prepareCompilerView(Compiler *compiler,
                                   TaskManager **taskManager) {
  TaskManager *tManager = new TaskManager(compiler);
  TaskModel *model = new TaskModel{tManager};
  TaskTableView *view = new TaskTableView{tManager};
  QObject::connect(view, &TaskTableView::TaskDialogRequested,
                   FOEDAG::handleTaskDialogRequested);
  QObject::connect(view, &TaskTableView::ViewFileRequested,
                   FOEDAG::handleViewFileRequested);
  QObject::connect(
      view, &TaskTableView::ViewReportRequested,
      [tManager, compiler](Task *task, const QString &reportId) {
        auto &reportManagerRegistry = tManager->getReportManagerRegistry();
        auto reportManager =
            reportManagerRegistry.getReportManager(tManager->taskId(task));
        if (reportManager)
          FOEDAG::handleViewReportRequested(compiler, task, reportId,
                                            *reportManager);
      });

  QObject::connect(view, &TaskTableView::ViewWaveform, [compiler](Task *task) {
    auto simType = task->cusomData().data.value<Simulator::SimulationType>();
    auto fileName = compiler->GetSimulator()->WaveFile(simType);
    std::filesystem::path filePath =
        compiler->FilePath(Compiler::ToCompilerAction(simType), fileName);
    if (FileUtils::FileExists(filePath)) {
      std::string cmd = "wave_open " + filePath.string();
      GlobalSession->CmdStack()->push_and_exec(new Command(cmd));
    }
  });

  view->setModel(model);

  view->resizeColumnsToContents();
  view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
  view->horizontalHeader()->setStretchLastSection(true);
  view->setMinimumWidth(340);

  compiler->setTaskManager(tManager);
  if (taskManager) *taskManager = tManager;
  return view;
}

uint toTaskId(int action, Compiler *const compiler) {
  switch (static_cast<Compiler::Action>(action)) {
    case Compiler::Action::Analyze:
      if (compiler->AnalyzeOpt() == Compiler::DesignAnalysisOpt::Clean)
        return ANALYSIS_CLEAN;
      return ANALYSIS;
    case Compiler::Action::Synthesis:
      if (compiler->SynthOpt() == Compiler::SynthesisOpt::Clean)
        return SYNTHESIS_CLEAN;
      return SYNTHESIS;
    case Compiler::Action::Global:
      if (compiler->GlobPlacementOpt() == Compiler::GlobalPlacementOpt::Clean)
        return GLOBAL_PLACEMENT_CLEAN;
      return GLOBAL_PLACEMENT;
    case Compiler::Action::Detailed:
      if (compiler->PlaceOpt() == Compiler::PlacementOpt::Clean)
        return PLACEMENT_CLEAN;
      return PLACEMENT;
    case Compiler::Action::Pack:
      if (compiler->PackOpt() == Compiler::PackingOpt::Clean)
        return PACKING_CLEAN;
      return PACKING;
    case Compiler::Action::Routing:
      if (compiler->RouteOpt() == Compiler::RoutingOpt::Clean)
        return ROUTING_CLEAN;
      return ROUTING;
    case Compiler::Action::STA:
      if (compiler->TimingAnalysisOpt() == Compiler::STAOpt::View)
        return PLACE_AND_ROUTE_VIEW;
      else if (compiler->TimingAnalysisOpt() == Compiler::STAOpt::Clean)
        return TIMING_SIGN_OFF_CLEAN;
      return TIMING_SIGN_OFF;
    case Compiler::Action::Bitstream:
      if (compiler->BitsOpt() == Compiler::BitstreamOpt::Clean)
        return BITSTREAM_CLEAN;
      return BITSTREAM;
    case Compiler::Action::Power:
      if (compiler->PowerAnalysisOpt() == Compiler::PowerOpt::Clean)
        return POWER_CLEAN;
      return POWER;
    case Compiler::Action::IPGen:
      return IP_GENERATE;
    case Compiler::Action::NoAction:
    case Compiler::Action::Batch:
      return TaskManager::invalid_id;
    case Compiler::Action::SimulateRTL:
      if (compiler->GetSimulator()->SimulationOption() ==
          Simulator::SimulationOpt::Clean)
        return SIMULATE_RTL_CLEAN;
      return SIMULATE_RTL;
    case Compiler::Action::SimulateGate:
      if (compiler->GetSimulator()->SimulationOption() ==
          Simulator::SimulationOpt::Clean)
        return SIMULATE_GATE_CLEAN;
      return SIMULATE_GATE;
    case Compiler::Action::SimulatePNR:
      if (compiler->GetSimulator()->SimulationOption() ==
          Simulator::SimulationOpt::Clean)
        return SIMULATE_PNR_CLEAN;
      return SIMULATE_PNR;
    case Compiler::Action::SimulateBitstream:
      if (compiler->GetSimulator()->SimulationOption() ==
          Simulator::SimulationOpt::Clean)
        return SIMULATE_BITSTREAM_CLEAN;
      return SIMULATE_BITSTREAM;
    case Compiler::Action::ProgramDevice:
      return TaskManager::invalid_id;
    case Compiler::Action::Configuration:
      return TaskManager::invalid_id;
  }
  return TaskManager::invalid_id;
}

FOEDAG::Design::Language FromFileType(const QString &type, bool postSynth) {
  if (QtUtils::IsEqual(type, "v")) {
    if (postSynth) return Design::Language::VERILOG_NETLIST;
    return Design::Language::VERILOG_2001;
  }
  if (QtUtils::IsEqual(type, "sv")) return Design::Language::SYSTEMVERILOG_2017;
  if (QtUtils::IsEqual(type, "vhd")) return Design::Language::VHDL_2008;
  if (QtUtils::IsEqual(type, "blif")) return Design::Language::BLIF;
  if (QtUtils::IsEqual(type, "eblif")) return Design::Language::EBLIF;
  if (QtUtils::IsEqual(type, "c") || QtUtils::IsEqual(type, "cc"))
    return Design::Language::C;
  if (QtUtils::IsEqual(type, "cpp")) return Design::Language::CPP;
  return postSynth ? Design::Language::VERILOG_NETLIST
                   : Design::Language::VERILOG_2001;
}

int read_sdc(const QString &file) {
  QString f = file;
  f.replace(PROJECT_OSRCDIR, Project::Instance()->projectPath());
  int res = Tcl_Eval(GlobalSession->TclInterp()->getInterp(),
                     qPrintable(QString("read_sdc {%1}").arg(f)));
  return (res == TCL_OK) ? 0 : -1;
}

bool target_device(const QString &target) {
  const int res = Tcl_Eval(GlobalSession->TclInterp()->getInterp(),
                           qPrintable(QString("target_device %1").arg(target)));
  return (res == TCL_OK);
}

int toAction(uint taskId) {
  switch (taskId) {
    case IP_GENERATE:
      return static_cast<int>(Compiler::Action::IPGen);
    case ANALYSIS:
      return static_cast<int>(Compiler::Action::Analyze);
    case SYNTHESIS:
      return static_cast<int>(Compiler::Action::Synthesis);
    case PACKING:
      return static_cast<int>(Compiler::Action::Pack);
    case GLOBAL_PLACEMENT:
      return static_cast<int>(Compiler::Action::Global);
    case PLACEMENT:
      return static_cast<int>(Compiler::Action::Detailed);
    case ROUTING:
      return static_cast<int>(Compiler::Action::Routing);
    case TIMING_SIGN_OFF:
      return static_cast<int>(Compiler::Action::STA);
    case POWER:
      return static_cast<int>(Compiler::Action::Power);
    case BITSTREAM:
      return static_cast<int>(Compiler::Action::Bitstream);
    case SIMULATE_RTL:
      return static_cast<int>(Compiler::Action::SimulateRTL);
    case SIMULATE_GATE:
      return static_cast<int>(Compiler::Action::SimulateGate);
    case SIMULATE_PNR:
      return static_cast<int>(Compiler::Action::SimulatePNR);
    case SIMULATE_BITSTREAM:
      return static_cast<int>(Compiler::Action::SimulateBitstream);
  }
  return static_cast<int>(Compiler::Action::NoAction);
}

}  // namespace FOEDAG
