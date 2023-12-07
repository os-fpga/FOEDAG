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
#include <QFile>

#include "Compiler/CompilerOpenFPGA.h"
#include "Main/Settings.h"
#include "Main/Tasks.h"
#include "MainWindow/Session.h"
#include "gtest/gtest.h"

using namespace FOEDAG;
extern FOEDAG::Session* GlobalSession;

class CompilerSettingsFixture : public testing::Test {
 public:
  CompilerSettingsFixture() {
    // auto context = new FOEDAG::ToolContext("Foedag", "OpenFPGA", "foedag");
    session = new Session{nullptr, nullptr,   nullptr,  nullptr,
                          nullptr, &compiler, &settings};
    compiler.SetSimulator(&simulator);
    compiler.SetSession(session);
    sessionBackup = GlobalSession;
    GlobalSession = session;
  }
  ~CompilerSettingsFixture() override {
    compiler.SetSimulator(nullptr);
    GlobalSession = sessionBackup;
    delete session;
  }

 protected:
  CompilerOpenFPGA compiler{};
  Settings settings;
  Simulator simulator{};
  Session* session{nullptr};
  Session* sessionBackup{nullptr};
};

TEST_F(CompilerSettingsFixture, TclArgsSynthesisOptions) {
  const bool ok = settings.loadJsonFile(":/Settings/settings_test.json");
  EXPECT_EQ(ok, true);

  const std::string testArgs{"-fast -_SynthOpt_ delay -no_flatten"};
  auto args = parseArguments(testArgs);
  TclArgs_setSynthesisOptions(args);

  EXPECT_EQ(compiler.SynthOptimization(), SynthesisOptimization::Delay);
  EXPECT_EQ(compiler.SynthMoreOpt(), "-fast -no_flatten");

  auto getArgs = TclArgs_getSynthesisOptions();
  EXPECT_EQ(getArgs.toString(), "-fast -no_flatten -_SynthOpt_ delay");
}

TEST_F(CompilerSettingsFixture, TclArgsPlacementOptions) {
  const bool ok = settings.loadJsonFile(":/Settings/settings_test.json");
  EXPECT_EQ(ok, true);

  const std::string testArgs{
      "-one -pin_assign_method pin_constraint_disabled -two"};
  auto args = parseArguments(testArgs);
  TclArgs_setPlacementOptions(args);

  EXPECT_EQ(compiler.PinAssignOpts(),
            Compiler::PinAssignOpt::Pin_constraint_disabled);
  EXPECT_EQ(compiler.PlaceMoreOpt(), "-one -two");

  auto getArgs = TclArgs_getPlacementOptions();
  EXPECT_EQ(getArgs.toString(),
            "-one -two -pin_assign_method pin_constraint_disabled");
}

TEST_F(CompilerSettingsFixture, TclArgsPackingOptions) {
  const bool ok = settings.loadJsonFile(":/Settings/settings_test.json");
  EXPECT_EQ(ok, true);

  const std::string testArgs{
      "-one -netlist_lang vhdl -clb_packing timing_driven -two"};
  auto args = parseArguments(testArgs);
  TclArgs_setPackingOptions(args);

  EXPECT_EQ(compiler.GetNetlistType(), Compiler::NetlistType::VHDL);
  EXPECT_EQ(compiler.ClbPackingOption(), ClbPacking::Timing_driven);

  auto getArgs = TclArgs_getPackingOptions();
  EXPECT_EQ(getArgs.toString(),
            "-netlist_lang vhdl -clb_packing timing_driven");
}

TEST_F(CompilerSettingsFixture, TclArgsTimingAnalysisOptions) {
  const bool ok = settings.loadJsonFile(":/Settings/settings_test.json");
  EXPECT_EQ(ok, true);

  const std::string testArgs{"-one -_StaOpt_ opensta -two"};
  auto args = parseArguments(testArgs);
  TclArgs_setTimingAnalysisOptions(args);

  EXPECT_EQ(compiler.TimingAnalysisEngineOpt(),
            Compiler::STAEngineOpt::Opensta);

  auto getArgs = TclArgs_getTimingAnalysisOptions();
  EXPECT_EQ(getArgs.toString(), "-_StaOpt_ opensta");
}

TEST_F(CompilerSettingsFixture, TclArgsSimulateOptionsRTL) {
  const bool ok = settings.loadJsonFile(":/Settings/settings_test.json");
  EXPECT_EQ(ok, true);

  // use default sim type since TclArgs_setSimulateOptions_rtl required
  // userValue, which appears only after apply
  const std::string testArgs{
      "-one -rtl_filepath file -rtl_sim_type icarus -run_rtl_opt some_opt "
      "-sim_rtl_opt some_opt -el_rtl_opt some_opt -com_rtl_opt some_opt -two"};
  auto args = parseArguments(testArgs);
  TclArgs_setSimulateOptions_rtl(args);

  EXPECT_EQ(compiler.GetSimulator()->WaveFile(Simulator::SimulationType::RTL),
            "file");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorCompileOption(
          Simulator::SimulationType::RTL, Simulator::SimulatorType::Icarus),
      "some_opt");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorElaborationOption(
          Simulator::SimulationType::RTL, Simulator::SimulatorType::Icarus),
      "some_opt");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorExtraOption(
          Simulator::SimulationType::RTL, Simulator::SimulatorType::Icarus),
      "some_opt");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorSimulationOption(
          Simulator::SimulationType::RTL, Simulator::SimulatorType::Icarus),
      "some_opt");

  auto getArgs = TclArgs_getSimulateOptions_rtl();
  const std::string expected{
      "-rtl_filepath file -rtl_sim_type icarus -run_rtl_opt some_opt "
      "-sim_rtl_opt some_opt -el_rtl_opt some_opt -com_rtl_opt some_opt"};
  EXPECT_EQ(getArgs.toString(), expected);
}

TEST_F(CompilerSettingsFixture, TclArgsSimulateOptionsGate) {
  const bool ok = settings.loadJsonFile(":/Settings/settings_test.json");
  EXPECT_EQ(ok, true);

  // use default sim type since TclArgs_setSimulateOptions_gate required
  // userValue, which appears only after apply
  const std::string testArgs{
      "-one -gate_filepath file -gate_sim_type icarus -run_gate_opt some_opt "
      "-sim_gate_opt some_opt -el_gate_opt some_opt -com_gate_opt some_opt "
      "-two"};
  auto args = parseArguments(testArgs);
  TclArgs_setSimulateOptions_gate(args);

  EXPECT_EQ(compiler.GetSimulator()->WaveFile(Simulator::SimulationType::Gate),
            "file");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorCompileOption(
          Simulator::SimulationType::Gate, Simulator::SimulatorType::Icarus),
      "some_opt");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorElaborationOption(
          Simulator::SimulationType::Gate, Simulator::SimulatorType::Icarus),
      "some_opt");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorExtraOption(
          Simulator::SimulationType::Gate, Simulator::SimulatorType::Icarus),
      "some_opt");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorSimulationOption(
          Simulator::SimulationType::Gate, Simulator::SimulatorType::Icarus),
      "some_opt");

  auto getArgs = TclArgs_getSimulateOptions_gate();
  const std::string expected{
      "-gate_filepath file -gate_sim_type icarus -run_gate_opt some_opt "
      "-sim_gate_opt some_opt -el_gate_opt some_opt -com_gate_opt some_opt"};
  EXPECT_EQ(getArgs.toString(), expected);
}

TEST_F(CompilerSettingsFixture, TclArgsSimulateOptionsPnr) {
  const bool ok = settings.loadJsonFile(":/Settings/settings_test.json");
  EXPECT_EQ(ok, true);

  // use default sim type since TclArgs_setSimulateOptions_pnr required
  // userValue, which appears only after apply
  const std::string testArgs{
      "-one -pnr_filepath file -pnr_sim_type icarus -run_pnr_opt some_opt "
      "-sim_pnr_opt some_opt -el_pnr_opt some_opt -com_pnr_opt some_opt -two"};
  auto args = parseArguments(testArgs);
  TclArgs_setSimulateOptions_pnr(args);

  EXPECT_EQ(compiler.GetSimulator()->WaveFile(Simulator::SimulationType::PNR),
            "file");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorCompileOption(
          Simulator::SimulationType::PNR, Simulator::SimulatorType::Icarus),
      "some_opt");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorElaborationOption(
          Simulator::SimulationType::PNR, Simulator::SimulatorType::Icarus),
      "some_opt");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorExtraOption(
          Simulator::SimulationType::PNR, Simulator::SimulatorType::Icarus),
      "some_opt");
  EXPECT_EQ(
      compiler.GetSimulator()->GetSimulatorSimulationOption(
          Simulator::SimulationType::PNR, Simulator::SimulatorType::Icarus),
      "some_opt");

  auto getArgs = TclArgs_getSimulateOptions_pnr();
  const std::string expected{
      "-pnr_filepath file -pnr_sim_type icarus -run_pnr_opt some_opt "
      "-sim_pnr_opt some_opt -el_pnr_opt some_opt -com_pnr_opt some_opt"};
  EXPECT_EQ(getArgs.toString(), expected);
}

TEST_F(CompilerSettingsFixture, TclArgsSimulateOptionsBitstream) {
  const bool ok = settings.loadJsonFile(":/Settings/settings_test.json");
  EXPECT_EQ(ok, true);

  // use default sim type since TclArgs_setSimulateOptions_bitstream required
  // userValue, which appears only after apply
  const std::string testArgs{
      "-one -bitstream_filepath file -bitstream_sim_type icarus "
      "-run_bitstream_opt some_opt -sim_bitstream_opt some_opt "
      "-el_bitstream_opt some_opt -com_bitstream_opt some_opt -two"};
  auto args = parseArguments(testArgs);
  TclArgs_setSimulateOptions_bitstream(args);

  EXPECT_EQ(compiler.GetSimulator()->WaveFile(
                Simulator::SimulationType::BitstreamBackDoor),
            "file");
  EXPECT_EQ(compiler.GetSimulator()->GetSimulatorCompileOption(
                Simulator::SimulationType::BitstreamBackDoor,
                Simulator::SimulatorType::Icarus),
            "some_opt");
  EXPECT_EQ(compiler.GetSimulator()->GetSimulatorElaborationOption(
                Simulator::SimulationType::BitstreamBackDoor,
                Simulator::SimulatorType::Icarus),
            "some_opt");
  EXPECT_EQ(compiler.GetSimulator()->GetSimulatorExtraOption(
                Simulator::SimulationType::BitstreamBackDoor,
                Simulator::SimulatorType::Icarus),
            "some_opt");
  EXPECT_EQ(compiler.GetSimulator()->GetSimulatorSimulationOption(
                Simulator::SimulationType::BitstreamBackDoor,
                Simulator::SimulatorType::Icarus),
            "some_opt");

  auto getArgs = TclArgs_getSimulateOptions_bitstream();
  const std::string expected{
      "-bitstream_filepath file -bitstream_sim_type icarus -run_bitstream_opt "
      "some_opt -sim_bitstream_opt some_opt -el_bitstream_opt some_opt "
      "-com_bitstream_opt some_opt"};
  EXPECT_EQ(getArgs.toString(), expected);
}
