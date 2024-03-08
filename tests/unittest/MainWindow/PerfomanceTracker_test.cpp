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

#include "MainWindow/PerfomanceTracker.h"

#include <QTableWidget>

#include "Compiler/CompilerDefines.h"
#include "Compiler/Task.h"
#include "Compiler/TaskManager.h"
#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(PerfomanceTracker, Init) {
  PerfomanceTracker pTracker{};
  EXPECT_NE(pTracker.widget(), nullptr);
  EXPECT_EQ(pTracker.isRtl(), true);
}

TEST(PerfomanceTracker, isNotRtl) {
  PerfomanceTracker pTracker{};

  TaskManager mManager{nullptr};
  pTracker.setTaskManager(&mManager);
  pTracker.setIsRtl(false);

  EXPECT_EQ(pTracker.widget()->rowCount(), 10);
}

TEST(PerfomanceTracker, isRtl) {
  PerfomanceTracker pTracker{};

  TaskManager mManager{nullptr};
  pTracker.setTaskManager(&mManager);
  pTracker.update();

  EXPECT_EQ(pTracker.widget()->rowCount(), 11);
}

TEST(PerfomanceTracker, updateTable) {
  PerfomanceTracker pTracker{};

  TaskManager mManager{nullptr};
  pTracker.setTaskManager(&mManager);

  mManager.task(ANALYSIS)->setUtilization({1000, 1024});
  pTracker.update();

  EXPECT_EQ(pTracker.widget()->item(0, 1)->text(), "1");
  EXPECT_EQ(pTracker.widget()->item(0, 2)->text(), "1");
}
