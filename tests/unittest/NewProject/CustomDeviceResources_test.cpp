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

#include <QStringList>

#include "NewProject/CustomLayoutBuilder.h"
#include "gtest/gtest.h"
using namespace FOEDAG;

TEST(EFpgaMath, EFpgaMath) {
  EFpgaMath eFpga{{1.3, 100, 100, 0, 0, 3400}};
  EXPECT_EQ(eFpga.widthMultiple(), 2);
  EXPECT_EQ(eFpga.heightMultiple(), 6);
  EXPECT_EQ((int)eFpga.phisicalClb(), 3400);
  EXPECT_EQ((int)eFpga.grids(), 4000);
  EXPECT_EQ(eFpga.width(), 56);
  EXPECT_EQ(eFpga.height(), 78);
  EXPECT_EQ(eFpga.dspCol(), 4);
  EXPECT_EQ(eFpga.bramCol(), 4);
  EXPECT_EQ(eFpga.clbCol(), 44);
  EXPECT_EQ(eFpga.columns(), 52);
  EXPECT_EQ(eFpga.dspCount(), 104);
  EXPECT_EQ(eFpga.bramCount(), 104);
  EXPECT_EQ(eFpga.clbCount(), 3432);

  EXPECT_EQ(eFpga.need() * 100, 75);
  EXPECT_EQ(int(eFpga.actual() * 100), 83);
  EXPECT_EQ(eFpga.isBlockCountValid(), true);
}

TEST(EFpgaMath, bramColumns) {
  EFpgaMath eFpga{{1.0, 100, 0, 0, 0, 3000}};

  std::vector<int> expectBram = {19, 23, 27, 32, 36, 40};
  EXPECT_EQ(eFpga.bramColumns(), expectBram);
}

TEST(EFpgaMath, dspColumns) {
  EFpgaMath eFpga{{1.0, 0, 100, 0, 0, 3000}};

  std::vector<int> expectDsp = {19, 23, 27, 32, 36, 40};
  EXPECT_EQ(eFpga.dspColumns(), expectDsp);
}

TEST(EFpgaMath, bothColumns) {
  EFpgaMath eFpga{{1.0, 100, 100, 0, 0, 3000}};

  std::vector<int> expectDsp = {10, 18, 26, 39, 47, 55};
  EXPECT_EQ(eFpga.dspColumns(), expectDsp);

  std::vector<int> expectBram = {14, 22, 30, 35, 43, 51};
  EXPECT_EQ(eFpga.bramColumns(), expectBram);
}
