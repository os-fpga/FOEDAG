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

#include "Utils/QtUtils.h"

#include <QEventLoop>

#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(QtUtils, SplitStringEmpty) {
  QString empty{};
  EXPECT_EQ(QtUtils::StringSplit(empty, ' '), QStringList{});
}

TEST(QtUtils, SplitStringSkipEmpty) {
  QString test{" test     test    "};
  auto expect = QStringList{{"test", "test"}};
  EXPECT_EQ(QtUtils::StringSplit(test, ' '), expect);
}

TEST(QtUtils, IsEqual) {
  const QString test{"test"};
  EXPECT_EQ(QtUtils::IsEqual(test, "test"), true);
  EXPECT_EQ(QtUtils::IsEqual(test, "TEST"), true);
  EXPECT_EQ(QtUtils::IsEqual(test, "TesT"), true);
  EXPECT_EQ(QtUtils::IsEqual(test, "tESt"), true);
  EXPECT_EQ(QtUtils::IsEqual(test, "tes"), false);
  EXPECT_EQ(QtUtils::IsEqual(test, "t"), false);
  EXPECT_EQ(QtUtils::IsEqual(test, "any"), false);
}

TEST(QtUtils, AppendToEventQueue) {
  std::vector<int> testData;
  QEventLoop eventLoop;  // event loop to handle events from AppendToEventQueue
  QtUtils::AppendToEventQueue([&]() { testData.push_back(1); });
  QtUtils::AppendToEventQueue([&]() {
    testData.push_back(2);
    eventLoop.exit(0);
  });
  testData.push_back(3);
  eventLoop.exec();
  std::vector<int> expected{3, 1, 2};
  EXPECT_EQ(testData, expected);
}

TEST(QtUtils, CreatePath) {
  QString path{"some_path"};
  QString file{"some_file"};
  auto fullPath = QtUtils::CreatePath(path, file);

  EXPECT_EQ(fullPath, QString{"%1%2%3"}.arg(path, QDir::separator(), file));
}

TEST(QtUtils, CreatePathOneArg) {
  QString path{"some_path"};
  auto fullPath = QtUtils::CreatePath(path);

  EXPECT_EQ(fullPath, QString{"%1"}.arg(path));
}

TEST(QtUtils, CreatePathFourArgs) {
  QString folder1{"folder1"};
  QString folder2{"folder2"};
  QString folder3{"folder3"};
  QString folder4{"folder4"};
  auto fullPath = QtUtils::CreatePath(folder1, folder2, folder3, folder4);

  EXPECT_EQ(fullPath,
            QString{"%1%2%3%4%5%6%7"}.arg(folder1, QDir::separator(), folder2,
                                          QDir::separator(), folder3,
                                          QDir::separator(), folder4));
}

TEST(QtUtils, CreatePathThreeArgs) {
  QString folder1{"folder1"};
  QString folder2{"folder2"};
  QString folder3{"folder3"};
  auto fullPath = QtUtils::CreatePath(folder1, folder2, folder3);

  EXPECT_EQ(fullPath,
            QString{"%1%2%3%4%5"}.arg(folder1, QDir::separator(), folder2,
                                      QDir::separator(), folder3));
}
