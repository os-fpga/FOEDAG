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

#include "gtest/gtest.h"
#include "PinAssignment/BufferedComboBox.h"
#include <QStringListModel>

using namespace FOEDAG;

TEST(BufferedComboBox, Initialization) {
  BufferedComboBox combo;
  QStringListModel model;
  model.setStringList({"test1", "test2", "test3"});
  combo.setModel(&model);
  EXPECT_EQ(combo.previousText(), QString());
}

TEST(BufferedComboBox, PreviousText) {
  BufferedComboBox combo;
  QStringListModel model;
  model.setStringList({"test1", "test2", "test3"});
  combo.setModel(&model);
  combo.setCurrentIndex(1);
  EXPECT_EQ(combo.previousText(), "test1");
  combo.setCurrentIndex(2);
  EXPECT_EQ(combo.previousText(), "test2");
}
