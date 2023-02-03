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

#include "PinAssignment/PinsBaseModel.h"

#include "gtest/gtest.h"
using namespace FOEDAG;

TEST(PinsBaseModel, PinMap) {
  PinsBaseModel model;
  model.update("port1", "pin1");
  model.update("port2", "pin2");
  QMap<QString, QString> expectedMap{{"port1", "pin1"}, {"port2", "pin2"}};
  QMap<QString, QString> actualMap = model.pinMap();
  EXPECT_EQ(actualMap, expectedMap);
}

TEST(PinsBaseModel, Exists) {
  PinsBaseModel model;
  model.update("port1", "pin1");
  model.update("port2", "pin2");
  EXPECT_EQ(model.exists("port2", "pin2"), true);
  EXPECT_EQ(model.exists("port1", "pin1"), true);

  EXPECT_EQ(model.exists("port2", "pin1"), false);
  EXPECT_EQ(model.exists("port1", "pin2"), false);
}

TEST(PinsBaseModel, UpdatePinEmpty) {
  PinsBaseModel model;
  model.update("port1", "pin1");
  model.update("port2", "pin2");
  model.update("port1", QString{});
  EXPECT_EQ(model.exists("port2", "pin2"), true);
  EXPECT_EQ(model.exists("port1", "pin1"), false);
  EXPECT_EQ(model.exists("port1", QString{}), false);
}

TEST(PinsBaseModel, UpdatePortEmpty) {
  PinsBaseModel model;
  model.update("port1", "pin1");
  model.update("port2", "pin2");
  model.update(QString{}, "pin1");
  EXPECT_EQ(model.exists("port2", "pin2"), true);
  EXPECT_EQ(model.exists("port1", "pin1"), false);
  EXPECT_EQ(model.exists("port1", QString{}), false);
}
