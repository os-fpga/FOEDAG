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

#include "PinAssignment/PortsLoader.h"
#include "gtest/gtest.h"
using namespace FOEDAG;
#include <filesystem>

TEST(PortsLoader, LoadGeneral) {
  PortsModel model;
  PortsLoader loader{&model};
  auto [res, error] = loader.load(":/PinAssignment/ports_test.json");
  EXPECT_EQ(res, true) << error.toStdString();

  auto listModel = model.listModel();
  QStringList expected = {"",          "inp1",     "out1[4]", "out1[3]",
                          "out1[2]",   "out1[1]",  "out1[0]", "inout1[2]",
                          "inout1[1]", "inout1[0]"};
  EXPECT_EQ(listModel->stringList(), expected);
  EXPECT_EQ(model.ports().count(), 1);
}

TEST(PortsLoader, LoadAllData) {
  PortsModel model;
  PortsLoader loader{&model};
  auto [res, error] = loader.load(":/PinAssignment/ports_test.json");
  EXPECT_EQ(res, true) << error.toStdString();

  auto allports = model.ports();
  for (const auto &group : allports) {
    EXPECT_EQ(group.name, QString());
    EXPECT_EQ(group.ports.count(), 3);
    auto port1 = group.ports.at(0);
    EXPECT_EQ(port1.name, "inp1") << port1.name.toStdString();
    EXPECT_EQ(port1.dir, "Input") << port1.dir.toStdString();
    EXPECT_EQ(port1.packagePin, "") << port1.packagePin.toStdString();
    EXPECT_EQ(port1.type, "LOGIC") << port1.type.toStdString();
    EXPECT_EQ(port1.range, "Msb: 0, lsb: 0") << port1.range.toStdString();

    auto port2 = group.ports.at(1);
    EXPECT_EQ(port2.name, "out1") << port2.name.toStdString();
    EXPECT_EQ(port2.dir, "Output") << port2.dir.toStdString();
    EXPECT_EQ(port2.packagePin, "") << port2.packagePin.toStdString();
    EXPECT_EQ(port2.type, "REG") << port2.type.toStdString();
    EXPECT_EQ(port2.range, "Msb: 4, lsb: 0") << port2.range.toStdString();

    auto port3 = group.ports.at(2);
    EXPECT_EQ(port3.name, "inout1") << port3.name.toStdString();
    EXPECT_EQ(port3.dir, "Inout") << port3.dir.toStdString();
    EXPECT_EQ(port3.packagePin, "") << port3.packagePin.toStdString();
    EXPECT_EQ(port3.type, "LOGIC") << port3.type.toStdString();
    EXPECT_EQ(port3.range, "Msb: 2, lsb: 0") << port3.range.toStdString();
  }
}

TEST(PortsLoader, LoadBus) {
  PortsModel model;
  PortsLoader loader{&model};
  auto [res, error] = loader.load(":/PinAssignment/ports_test.json");
  EXPECT_EQ(res, true) << error.toStdString();

  const int groupIndex{0};
  const int portIndex{1};
  auto port = model.ports().at(groupIndex).ports.at(portIndex);
  EXPECT_EQ(port.isBus, true);
  EXPECT_EQ(port.ports.count(), 5);
  for (int i = 0; i < port.ports.count(); i++) {
    auto p = port.ports.at(i);
    EXPECT_EQ(p.name, QString("out1[%1]").arg(QString::number(4 - i)));
    EXPECT_EQ(p.dir, "Output");
    EXPECT_EQ(p.packagePin, "");
    EXPECT_EQ(p.type, "REG");
    EXPECT_EQ(p.range, "Msb: 4, lsb: 0");
  }
}

TEST(PortsLoader, LoadCorruptedJson) {
  PortsModel model;
  PortsLoader loader{&model};
  auto [res, error] = loader.load(":/PinAssignment/corrupted.json");
  EXPECT_EQ(res, false) << error.toStdString();
  EXPECT_NE(error, QString{});
}
