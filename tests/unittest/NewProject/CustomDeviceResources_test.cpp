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
#include <iostream>

#include "NewProject/CustomLayoutBuilder.h"
#include "gtest/gtest.h"
using namespace FOEDAG;

struct Expected {
  bool valid{};
  int luts{};
  int ffs{};
  int dsp{};
  int bram{};
  int carryLength{};
};

CustomLayoutData generate(int w, int h, const QString &bram,
                          const QString &dsp) {
  return CustomLayoutData{QString{}, QString{}, w, h, bram, dsp};
}

std::string toStr(const CustomLayoutData &dat) {
  std::stringstream os;
  os << "CustomLayoutData{width :" << dat.width << ", height : " << dat.height
     << ", dsp: \"" << dat.dsp.toStdString() << "\", bram: \""
     << dat.bram.toStdString() << "\"}";
  return os.str();
}

void test(const CustomDeviceResources &actual, const Expected &expected,
          const CustomLayoutData &output) {
  EXPECT_EQ(actual.isValid(), expected.valid) << toStr(output);
  if (actual.isValid()) {
    EXPECT_EQ(actual.lutsCount(), expected.luts) << toStr(output);
    EXPECT_EQ(actual.ffsCount(), expected.ffs) << toStr(output);
    EXPECT_EQ(actual.dspCount(), expected.dsp) << toStr(output);
    EXPECT_EQ(actual.bramCount(), expected.bram) << toStr(output);
    EXPECT_EQ(actual.carryLengthCount(), expected.carryLength) << toStr(output);
  }
}

std::vector<CustomLayoutData> generateTestData() {
  std::vector<CustomLayoutData> allData{};
  for (int width = 0; width < 12; width++) {
    for (int bramDspCol = 0; bramDspCol < 3; bramDspCol++) {
      QStringList list{bramDspCol, "1"};
      auto dspBram = list.join(",");
      allData.push_back(generate(width, width, dspBram, dspBram));
    }
  }
  return allData;
}

std::vector<Expected> generateExpectedData() {
  std::vector<Expected> allData{};
  allData.push_back({false, 0, 0, 0, 0, 0});       // 0, 0, "", ""
  allData.push_back({false, 0, 0, 0, 0, 0});       // 0, 0, "1", "1"
  allData.push_back({false, 0, 0, 0, 0, 0});       // 0, 0, "1,1", "1,1"
  allData.push_back({false, 0, 0, 0, 0, 0});       // 1, 1, "", ""
  allData.push_back({false, 0, 0, 0, 0, 0});       // 1, 1, "1", "1"
  allData.push_back({false, 0, 0, 0, 0, 0});       // 1, 1, "1,1", "1,1"
  allData.push_back({false, 0, 0, 0, 0, 0});       // 2, 2, "", ""
  allData.push_back({false, 0, 0, 0, 0, 0});       // 2, 2, "1", "1"
  allData.push_back({false, 0, 0, 0, 0, 0});       // 2, 2, "1,1", "1,1"
  allData.push_back({true, 8, 16, 0, 0, 8});       // 3, 3, "", ""
  allData.push_back({false, 0, 0, 0, 0, 8});       // 3, 3, "1", "1"
  allData.push_back({false, 0, 0, 0, 0, 8});       // 3, 3, "1,1", "1,1"
  allData.push_back({true, 32, 64, 0, 0, 16});     // 4, 4, "", ""
  allData.push_back({false, 0, 0, 0, 0, 16});      // 4, 4, "1", "1"
  allData.push_back({false, 32, 64, 0, 0, 16});    // 4, 4, "1,1", "1,1"
  allData.push_back({true, 72, 144, 0, 0, 24});    // 5, 5, "", ""
  allData.push_back({true, 24, 48, 1, 1, 24});     // 5, 5, "1", "1"
  allData.push_back({false, 0, 0, 0, 0, 24});      // 5, 5, "1,1", "1,1"
  allData.push_back({true, 128, 256, 0, 0, 32});   // 6, 6, "", ""
  allData.push_back({false, 64, 128, 1, 1, 32});   // 6, 6, "1", "1"
  allData.push_back({false, 0, 0, 2, 2, 32});      // 6, 6, "1,1", "1,1"
  allData.push_back({true, 200, 400, 0, 0, 40});   // 7, 7, "", ""
  allData.push_back({false, 120, 240, 1, 1, 40});  // 7, 7, "1", "1"
  allData.push_back({false, 40, 80, 2, 2, 40});    // 7, 7, "1,1", "1,1"
  allData.push_back({true, 288, 576, 0, 0, 48});   // 8, 8, "", ""
  allData.push_back({true, 192, 384, 2, 2, 48});   // 8, 8, "1", "1"
  allData.push_back({true, 96, 192, 4, 4, 48});    // 8, 8, "1,1", "1,1"
  allData.push_back({true, 392, 784, 0, 0, 56});   // 9, 9, "", ""
  allData.push_back({false, 280, 560, 2, 2, 56});  // 9, 9, "1", "1"
  allData.push_back({false, 168, 336, 4, 4, 56});  // 9, 9, "1,1", "1,1"
  allData.push_back({true, 512, 1024, 0, 0, 64});  // 10, 10, "", ""
  allData.push_back({false, 0, 0, 0, 0, 0});       // 10, 10, "1", "1"
  allData.push_back({false, 0, 0, 0, 0, 0});       // 10, 10, "1,1", "1,1"
  allData.push_back({true, 648, 1296, 0, 0, 72});  // 11, 11, "", ""
  allData.push_back({true, 504, 1008, 3, 3, 72});  // 11, 11, "1", "1"
  allData.push_back({true, 360, 720, 6, 6, 72});   // 11, 11, "1,1", "1,1"
  return allData;
}

TEST(CustomDeviceResources, CustomDeviceResources) {
  auto actual = generateTestData();
  auto expected = generateExpectedData();
  for (int i = 0; i < actual.size(); i++) {
    CustomDeviceResources resources{actual.at(i)};
    test(resources, expected.at(i), actual.at(i));
  }
}
