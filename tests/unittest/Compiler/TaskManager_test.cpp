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

#include "Compiler/TaskManager.h"

#include "Compiler/Compiler.h"
#include "Compiler/CompilerDefines.h"
#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(TaskManager, getDownstreamCleanTasks) {
  TaskManager taskManager{nullptr};

  taskManager.task(SIMULATE_RTL)
      ->setCustomData({CustomDataType::Sim,
                       static_cast<int>(Simulator::SimulationType::RTL)});
  taskManager.task(SIMULATE_PNR)
      ->setCustomData({CustomDataType::Sim,
                       static_cast<int>(Simulator::SimulationType::PNR)});
  taskManager.task(SIMULATE_GATE)
      ->setCustomData({CustomDataType::Sim,
                       static_cast<int>(Simulator::SimulationType::Gate)});
  taskManager.task(SIMULATE_BITSTREAM)
      ->setCustomData(
          {CustomDataType::Sim,
           static_cast<int>(Simulator::SimulationType::BitstreamBackDoor)});

  auto simulationRtl = taskManager.task(SIMULATE_RTL_CLEAN);
  auto cleanTasks = taskManager.getDownstreamCleanTasks(simulationRtl);
  EXPECT_EQ(cleanTasks.count(), 1);
  EXPECT_EQ(cleanTasks.at(0), simulationRtl);

  auto simulationGate = taskManager.task(SIMULATE_GATE_CLEAN);
  cleanTasks = taskManager.getDownstreamCleanTasks(simulationGate);
  EXPECT_EQ(cleanTasks.count(), 1);
  EXPECT_EQ(cleanTasks.at(0), simulationGate);

  auto simulationPnr = taskManager.task(SIMULATE_PNR_CLEAN);
  cleanTasks = taskManager.getDownstreamCleanTasks(simulationPnr);
  EXPECT_EQ(cleanTasks.count(), 1);
  EXPECT_EQ(cleanTasks.at(0), simulationPnr);

  auto simulationBitstream = taskManager.task(SIMULATE_BITSTREAM_CLEAN);
  cleanTasks = taskManager.getDownstreamCleanTasks(simulationBitstream);
  EXPECT_EQ(cleanTasks.count(), 1);
  EXPECT_EQ(cleanTasks.at(0), simulationBitstream);

  auto analysis = taskManager.task(ANALYSIS_CLEAN);
  cleanTasks = taskManager.getDownstreamCleanTasks(analysis);
  EXPECT_EQ(cleanTasks.count(), 12);
}
