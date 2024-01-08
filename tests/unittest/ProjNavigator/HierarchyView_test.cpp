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

#include "ProjNavigator/HierarchyView.h"

#include <QTreeWidget>
#include <QtTest/QSignalSpy>

#include "gtest/gtest.h"

using namespace FOEDAG;
namespace fs = std::filesystem;

static const fs::path FilePathProj1{":/ProjNavigator/hier_info.json"};
static const fs::path FilePathProj2{":/ProjNavigator/hier_info_1.json"};
static const fs::path FilePathCorrupted{
    ":/ProjNavigator/hier_info_corrupted.json"};

TEST(HierarchyView, constructor) {
  HierarchyView view{FilePathProj1};
  ASSERT_NE(view.widget(), nullptr);
  ASSERT_EQ(view.widget()->topLevelItemCount(), 1);
  ASSERT_EQ(view.widget()->topLevelItem(0)->childCount(), 0);
}

TEST(HierarchyView, setPortsFileCorruptedFile) {
  HierarchyView view{{}};
  view.setPortsFile(FilePathCorrupted);
  ASSERT_NE(view.widget(), nullptr);
  ASSERT_EQ(view.widget()->topLevelItemCount(), 0);
}

TEST(HierarchyView, setPortsFile) {
  HierarchyView view{FilePathProj1};
  ASSERT_NE(view.widget(), nullptr);
  ASSERT_EQ(view.widget()->topLevelItemCount(), 1);
  ASSERT_EQ(view.widget()->topLevelItem(0)->childCount(), 0);

  view.setPortsFile(FilePathProj2);
  ASSERT_EQ(view.widget()->topLevelItemCount(), 1);
  ASSERT_EQ(view.widget()->topLevelItem(0)->childCount(), 1);
  ASSERT_EQ(view.widget()->topLevelItem(0)->child(0)->childCount(), 4);

  auto child = view.widget()->topLevelItem(0)->child(0);
  EXPECT_EQ(child->child(0)->childCount(), 0);
  EXPECT_EQ(child->child(1)->childCount(), 4);
  EXPECT_EQ(child->child(2)->childCount(), 0);
  EXPECT_EQ(child->child(3)->childCount(), 16);

  child = view.widget()->topLevelItem(0)->child(0)->child(1);
  for (int i = 0; i < 4; i++) EXPECT_EQ(child->child(i)->childCount(), 4);

  child = view.widget()->topLevelItem(0)->child(0)->child(3);
  for (int i = 0; i < 16; i++) EXPECT_EQ(child->child(i)->childCount(), 28);
}

TEST(HierarchyView, clean) {
  HierarchyView view{FilePathProj1};
  ASSERT_NE(view.widget(), nullptr);
  ASSERT_EQ(view.widget()->topLevelItemCount(), 1);
  ASSERT_EQ(view.widget()->topLevelItem(0)->childCount(), 0);

  view.clean();
  ASSERT_EQ(view.widget()->topLevelItemCount(), 0);
}

TEST(HierarchyView, topModuleFile) {
  HierarchyView view{{}};
  fs::path expectedPath{":/ProjNavigator/dut.v"};
  expectedPath = expectedPath.lexically_normal();
  QSignalSpy signalSpy{&view, &HierarchyView::topModuleFile};
  view.setPortsFile(FilePathProj1);

  EXPECT_EQ(signalSpy.count(), 1);
  auto arguments = signalSpy.takeFirst();
  auto file = arguments.at(0).toString();
  EXPECT_EQ(file, QString::fromStdString(expectedPath.string()))
      << file.toStdString();
}
