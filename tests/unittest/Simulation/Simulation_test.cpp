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

#include "Simulation/Simulator.h"
#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(Simulator, ToSimulatorType) {
  // SimulatorType { Verilator, Icarus, GHDL, VCS, Questa, Xcelium };
  bool ok{false};
  auto res = Simulator::ToSimulatorType("verilator", ok);
  EXPECT_EQ(res, Simulator::SimulatorType::Verilator);
  EXPECT_EQ(ok, true);
  ok = false;

  res = Simulator::ToSimulatorType("icarus", ok);
  EXPECT_EQ(res, Simulator::SimulatorType::Icarus);
  EXPECT_EQ(ok, true);
  ok = false;

  res = Simulator::ToSimulatorType("ghdl", ok);
  EXPECT_EQ(res, Simulator::SimulatorType::GHDL);
  EXPECT_EQ(ok, true);
  ok = false;

  res = Simulator::ToSimulatorType("vcs", ok);
  EXPECT_EQ(res, Simulator::SimulatorType::VCS);
  EXPECT_EQ(ok, true);
  ok = false;

  res = Simulator::ToSimulatorType("questa", ok);
  EXPECT_EQ(res, Simulator::SimulatorType::Questa);
  EXPECT_EQ(ok, true);
  ok = false;

  res = Simulator::ToSimulatorType("xcelium", ok);
  EXPECT_EQ(res, Simulator::SimulatorType::Xcelium);
  EXPECT_EQ(ok, true);
  ok = false;

  auto defaultValue{Simulator::SimulatorType::Xcelium};
  res = Simulator::ToSimulatorType("unknown", ok, defaultValue);
  EXPECT_EQ(res, defaultValue);
  EXPECT_EQ(ok, false);
}

TEST(Simulator, ToStringSimulatorType) {
  EXPECT_EQ(Simulator::ToString(Simulator::SimulatorType::Verilator),
            "verilator");
  EXPECT_EQ(Simulator::ToString(Simulator::SimulatorType::GHDL), "ghdl");
  EXPECT_EQ(Simulator::ToString(Simulator::SimulatorType::Icarus), "icarus");
  EXPECT_EQ(Simulator::ToString(Simulator::SimulatorType::Questa), "questa");
  EXPECT_EQ(Simulator::ToString(Simulator::SimulatorType::VCS), "vcs");
  EXPECT_EQ(Simulator::ToString(Simulator::SimulatorType::Xcelium), "xcelium");
}

TEST(Simulator, ToSimulationType) {
  // SimulationType { RTL, Gate, PNR, Bitstream };
  bool ok{false};
  auto res = Simulator::ToSimulationType("rtl", ok);
  EXPECT_EQ(ok, true);
  EXPECT_EQ(res, Simulator::SimulationType::RTL);

  ok = false;
  res = Simulator::ToSimulationType("gate", ok);
  EXPECT_EQ(ok, true);
  EXPECT_EQ(res, Simulator::SimulationType::Gate);

  ok = false;
  res = Simulator::ToSimulationType("pnr", ok);
  EXPECT_EQ(ok, true);
  EXPECT_EQ(res, Simulator::SimulationType::PNR);

  ok = false;
  res = Simulator::ToSimulationType("bitstream_bd", ok);
  EXPECT_EQ(ok, true);
  EXPECT_EQ(res, Simulator::SimulationType::BitstreamBackDoor);

  ok = false;
  res = Simulator::ToSimulationType("bitstream_fd", ok);
  EXPECT_EQ(ok, true);
  EXPECT_EQ(res, Simulator::SimulationType::BitstreamFrontDoor);

  ok = false;
  res = Simulator::ToSimulationType("unknown", ok);
  EXPECT_EQ(ok, false);
  EXPECT_EQ(res, Simulator::SimulationType::RTL);
}

TEST(Simulator, UserSimulationType) {
  Simulator sim;
  sim.UserSimulationType(Simulator::SimulationType::RTL,
                         Simulator::SimulatorType::Xcelium);

  bool ok{false};
  auto simulator = sim.UserSimulationType(Simulator::SimulationType::RTL, ok);

  EXPECT_EQ(simulator, Simulator::SimulatorType::Xcelium);
  EXPECT_EQ(ok, true);

  ok = false;
  simulator =
      sim.UserSimulationType(Simulator::SimulationType::BitstreamBackDoor, ok);

  EXPECT_EQ(simulator, Simulator::SimulatorType::Verilator);
  EXPECT_EQ(ok, false);
}
