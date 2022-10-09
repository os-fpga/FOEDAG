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

#include "NewProject/source_grid.h"

#include <QDebug>

#include "Compiler/CompilerDefines.h"
#include "gtest/gtest.h"
using namespace FOEDAG;

namespace FOEDAG {

bool operator==(const FOEDAG::tagFileData &a, const FOEDAG::tagFileData &b) {
  return (a.m_isFolder == b.m_isFolder) && (a.m_fileName == b.m_fileName) &&
         (a.m_filePath == b.m_filePath) && (a.m_fileType == b.m_fileType) &&
         (a.m_language == b.m_language) && (a.m_workLibrary == b.m_workLibrary);
}

}  // namespace FOEDAG

TEST(sourceGrid, getTableViewDataEmpty) {
  sourceGrid grid;
  grid.setGridType(GT_SOURCE);
  EXPECT_EQ(grid.getTableViewData(), QList<filedata>{});
}

TEST(sourceGrid, getTableViewDataNotEmpty) {
  sourceGrid grid;
  grid.setGridType(GT_SOURCE);
  filedata data1{false,       "v",   "test1.v", Design::VERILOG_2001,
                 "somePath/", "lib"};
  grid.AddTableItem(data1);
  filedata data2{false,       "sv",   "test2.sv", Design::SYSTEMVERILOG_2012,
                 "somePath/", "lib2"};
  grid.AddTableItem(data2);
  QList<filedata> expected = {data1, data2};
  EXPECT_EQ(grid.getTableViewData(), expected);
}

TEST(sourceGrid, UpTableItem) {
  sourceGrid grid;
  grid.setGridType(GT_SOURCE);
  filedata data1{false,       "v",   "test1.v", Design::VERILOG_2001,
                 "somePath/", "lib"};
  grid.AddTableItem(data1);
  filedata data2{false,       "sv",   "test2.sv", Design::SYSTEMVERILOG_2012,
                 "somePath/", "lib2"};
  grid.AddTableItem(data2);

  grid.selectRow(1);
  grid.UpTableItem();

  QList<filedata> expected = {data2, data1};
  auto actual = grid.getTableViewData();
  QString out;
  QDebug d{&out};
  d << "Actual: " << actual << "\n";
  d << "Expected: " << expected;
  EXPECT_EQ(actual, expected) << out.toStdString();
}

TEST(sourceGrid, DownTableItem) {
  sourceGrid grid;
  grid.setGridType(GT_SOURCE);
  filedata data1{false,       "v",   "test1.v", Design::VERILOG_2001,
                 "somePath/", "lib"};
  grid.AddTableItem(data1);
  filedata data2{false,       "sv",   "test2.sv", Design::SYSTEMVERILOG_2012,
                 "somePath/", "lib2"};
  grid.AddTableItem(data2);

  grid.selectRow(0);
  grid.DownTableItem();

  QList<filedata> expected = {data2, data1};
  auto actual = grid.getTableViewData();
  QString out;
  QDebug d{&out};
  d << "Actual: " << actual << "\n";
  d << "Expected: " << expected;
  EXPECT_EQ(actual, expected) << out.toStdString();
}

TEST(sourceGrid, DeleteTableItemFirstElement) {
  sourceGrid grid;
  grid.setGridType(GT_SOURCE);
  filedata data1{false,       "v",   "test1.v", Design::VERILOG_2001,
                 "somePath/", "lib"};
  grid.AddTableItem(data1);
  filedata data2{false,       "sv",   "test2.sv", Design::SYSTEMVERILOG_2012,
                 "somePath/", "lib2"};
  grid.AddTableItem(data2);

  grid.selectRow(0);
  grid.DeleteTableItem();

  QList<filedata> expected = {data2};
  auto actual = grid.getTableViewData();
  EXPECT_EQ(actual, expected);
}

TEST(sourceGrid, DeleteTableItemLastElement) {
  sourceGrid grid;
  grid.setGridType(GT_SOURCE);
  filedata data1{false,       "v",   "test1.v", Design::VERILOG_2001,
                 "somePath/", "lib"};
  grid.AddTableItem(data1);
  filedata data2{false,       "sv",   "test2.sv", Design::SYSTEMVERILOG_2012,
                 "somePath/", "lib2"};
  grid.AddTableItem(data2);

  grid.selectRow(1);
  grid.DeleteTableItem();

  QList<filedata> expected = {data1};
  auto actual = grid.getTableViewData();
  EXPECT_EQ(actual, expected);
}

TEST(sourceGrid, ClearTable) {
  sourceGrid grid;
  grid.setGridType(GT_SOURCE);
  filedata data1{false,       "v",  "test1.v", Design::VERILOG_2001,
                 "somePath/", "lib"};
  grid.AddTableItem(data1);
  filedata data2{false,       "sv",  "test2.sv", Design::SYSTEMVERILOG_2012,
                 "somePath/", "lib2"};
  grid.AddTableItem(data2);

  grid.ClearTable();

  QList<filedata> expected = {};
  auto actual = grid.getTableViewData();
  EXPECT_EQ(actual, expected);
}
