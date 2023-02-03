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

#include "PinAssignment/PackagePinsLoader.h"

#include <filesystem>

#include "PinAssignment/PackagePinsModel.h"
#include "gtest/gtest.h"
using namespace FOEDAG;

TEST(PackagePinsLoader, LoadGeneral) {
  PackagePinsModel model;
  PackagePinsLoader loader{&model};
  loader.loadHeader(":/PinAssignment/package_pin_info.json");
  auto [res, error] = loader.load(":/PinAssignment/Pin_Table.csv");
  EXPECT_EQ(res, true) << error.toStdString();

  auto listModel = model.listModel();
  QStringList expected = {"", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8"};
  EXPECT_EQ(listModel->stringList(), expected);
  EXPECT_EQ(model.pinData().count(), 3);
}

TEST(PackagePinsLoader, LoadNoModel) {
  PackagePinsLoader loader{nullptr};
  auto [res, error] = loader.load(":/PinAssignment/Pin_Table.csv");
  EXPECT_EQ(res, false);
  EXPECT_NE(error, QString());
}

TEST(PackagePinsLoader, LoadWrongFilePath) {
  PackagePinsModel model;
  PackagePinsLoader loader{&model};
  auto [res, error] = loader.load("wrong/path/file.csv");
  EXPECT_EQ(res, false);
  EXPECT_NE(error, QString());
}

TEST(PackagePinsLoader, LoadHeaderCorruptedJson) {
  PackagePinsModel model;
  PackagePinsLoader loader{&model};
  auto [res, error] = loader.loadHeader(":/PinAssignment/corrupted.json");
  EXPECT_EQ(res, false);
  EXPECT_NE(error, QString());
}

TEST(PackagePinsLoader, LoadAllPins) {
  PackagePinsModel model;
  PackagePinsLoader loader{&model};
  loader.loadHeader(":/PinAssignment/package_pin_info.json");
  auto [res, error] = loader.load(":/PinAssignment/Pin_Table.csv");
  EXPECT_EQ(res, true) << error.toStdString();

  EXPECT_EQ(model.pinData().count(), 3);

  auto bank0 = model.pinData().at(0);
  EXPECT_EQ(bank0.name, "Bank 0");
  EXPECT_EQ(bank0.pinData.count(), 3);
  EXPECT_EQ(bank0.pinData.at(0).data.at(PinName), "A1");
  EXPECT_EQ(bank0.pinData.at(0).data.at(BallName), "A1");
  EXPECT_EQ(bank0.pinData.at(1).data.at(PinName), "A2");
  EXPECT_EQ(bank0.pinData.at(1).data.at(BallName), "A2");
  EXPECT_EQ(bank0.pinData.at(2).data.at(PinName), "A3");
  EXPECT_EQ(bank0.pinData.at(2).data.at(BallName), "A3");

  auto bank1 = model.pinData().at(1);
  EXPECT_EQ(bank1.name, "Bank 1");
  EXPECT_EQ(bank1.pinData.count(), 3);
  EXPECT_EQ(bank1.pinData.at(0).data.at(PinName), "A4");
  EXPECT_EQ(bank1.pinData.at(0).data.at(BallName), "A4");
  EXPECT_EQ(bank1.pinData.at(1).data.at(PinName), "A5");
  EXPECT_EQ(bank1.pinData.at(1).data.at(BallName), "A5");
  EXPECT_EQ(bank1.pinData.at(2).data.at(PinName), "A6");
  EXPECT_EQ(bank1.pinData.at(2).data.at(BallName), "A6");

  auto bank2 = model.pinData().at(2);
  EXPECT_EQ(bank2.name, "Bank 2");
  EXPECT_EQ(bank2.pinData.count(), 2);
  EXPECT_EQ(bank2.pinData.at(0).data.at(PinName), "A7");
  EXPECT_EQ(bank2.pinData.at(0).data.at(BallName), "A7");
  EXPECT_EQ(bank2.pinData.at(1).data.at(PinName), "A8");
  EXPECT_EQ(bank2.pinData.at(1).data.at(BallName), "A8");
}

TEST(PackagePinsLoader, LoadHeaderGeneral) {
  PackagePinsModel model;
  PackagePinsLoader loader{&model};
  auto [res, error] =
      loader.loadHeader(":/PinAssignment/package_pin_info.json");
  EXPECT_EQ(res, true) << error.toStdString();

  auto header = model.header();
  EXPECT_EQ(header.size(), 16);
  // check random column
  auto col0 = header.at(0);
  EXPECT_EQ(col0.name, "Name");
  EXPECT_EQ(col0.id, 0);
  EXPECT_EQ(col0.visible, true);
  EXPECT_NE(col0.description, QString());
}

TEST(PackagePinsLoader, LoadHeaderNoModel) {
  PackagePinsLoader loader{nullptr};
  auto [res, error] = loader.loadHeader(":/PinAssignment/Pin_Table.csv");
  EXPECT_EQ(res, false);
  EXPECT_NE(error, QString());
}

TEST(PackagePinsLoader, LoadHeaderWrongFilePath) {
  PackagePinsModel model;
  PackagePinsLoader loader{&model};
  auto [res, error] = loader.loadHeader("wrong/path/file.csv");
  EXPECT_EQ(res, false);
  EXPECT_NE(error, QString());
}
