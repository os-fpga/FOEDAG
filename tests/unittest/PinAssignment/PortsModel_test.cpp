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

#include "PinAssignment/PortsModel.h"

#include "PinAssignment/PortsLoader.h"
#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(PortsModel, GetPort) {
  PortsModel model;
  PortsLoader loader{&model};
  loader.load(":/PinAssignment/ports_test.json");

  std::vector<IOPort> ports;
  for (uint i{0}; i < 4; i++)
    ports.push_back(model.GetPort(QString{"out1[%1]"}.arg(QString::number(i))));

  for (uint i = 0; i < ports.size(); i++) {
    auto p = ports.at(i);
    EXPECT_EQ(p.name, QString("out1[%1]").arg(QString::number(i)));
    EXPECT_EQ(p.dir, "Output");
    EXPECT_EQ(p.packagePin, "");
    EXPECT_EQ(p.type, "REG");
  }
}
