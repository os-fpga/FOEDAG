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

#include "Utils/sequential_map.h"

#include "gtest/gtest.h"
using namespace FOEDAG;

TEST(sequential_map, operatorInsert) {
  sequential_map<std::string, int> m;
  m["test"] = 5;
  EXPECT_EQ(m.value("test"), 5);
  EXPECT_EQ(m.empty(), false);
}

TEST(sequential_map, empty) {
  sequential_map<std::string, int> m;
  EXPECT_EQ(m.empty(), true);
}

TEST(sequential_map, operatorModyfy) {
  sequential_map<std::string, int> m;
  m["test"] = 5;
  m["test"] = 6;
  EXPECT_EQ(m.value("test"), 6);
}

TEST(sequential_map, values) {
  sequential_map<std::string, int> m;
  m["test0"] = 5;
  m["test1"] = 6;
  auto values = m.values();
  EXPECT_EQ(values.size(), 2);
  EXPECT_EQ(values.at(0).first, "test0");
  EXPECT_EQ(values.at(0).second, 5);
  EXPECT_EQ(values.at(1).first, "test1");
  EXPECT_EQ(values.at(1).second, 6);
}

TEST(sequential_map, value) {
  sequential_map<std::string, int> m;
  m["test0"] = 5;
  m["test1"] = 6;
  auto actual0 = m.value("test0");
  auto actual1 = m.value("test1");
  EXPECT_EQ(actual0, 5);
  EXPECT_EQ(actual1, 6);
}

TEST(sequential_map, valueDefault) {
  sequential_map<std::string, int> m;
  m["test0"] = 5;
  auto actual = m.value("not_exists", 10);
  EXPECT_EQ(actual, 10);
  EXPECT_EQ(m.count(), 1);
}

TEST(sequential_map, pushBack) {
  sequential_map<std::string, int> m;
  m["test0"] = 5;
  m.push_back(std::make_pair("test1", 10));
  auto values = m.values();
  EXPECT_EQ(values.size(), 2);
  EXPECT_EQ(values.at(0).first, "test0");
  EXPECT_EQ(values.at(0).second, 5);
  EXPECT_EQ(values.at(1).first, "test1");
  EXPECT_EQ(values.at(1).second, 10);
}

TEST(sequential_map, pushBackSameValue) {
  sequential_map<std::string, int> m;
  m.push_back(std::make_pair("test1", 10));
  m.push_back(std::make_pair("test1", 5));
  auto values = m.values();
  EXPECT_EQ(values.size(), 1);
  EXPECT_EQ(values.at(0).first, "test1");
  EXPECT_EQ(values.at(0).second, 5);
}

TEST(multi_sequential_map, operatorInsert) {
  sequential_multi_map<std::string, int> m;
  m["test"] = 5;
  EXPECT_EQ(m.value("test"), 5);
  EXPECT_EQ(m.empty(), false);
}

TEST(multi_sequential_map, empty) {
  sequential_multi_map<std::string, int> m;
  EXPECT_EQ(m.empty(), true);
}

TEST(multi_sequential_map, operatorModyfy) {
  sequential_multi_map<std::string, int> m;
  m["test"] = 5;
  m["test"] = 6;
  EXPECT_EQ(m.value("test"), 6);
}

TEST(multi_sequential_map, values) {
  sequential_multi_map<std::string, int> m;
  m["test0"] = 5;
  m["test1"] = 6;
  auto values = m.values();
  EXPECT_EQ(values.size(), 2);
  EXPECT_EQ(values.at(0).first, "test0");
  EXPECT_EQ(values.at(0).second, 5);
  EXPECT_EQ(values.at(1).first, "test1");
  EXPECT_EQ(values.at(1).second, 6);
}

TEST(multi_sequential_map, value) {
  sequential_multi_map<std::string, int> m;
  m["test0"] = 5;
  m["test1"] = 6;
  auto actual0 = m.value("test0");
  auto actual1 = m.value("test1");
  EXPECT_EQ(actual0, 5);
  EXPECT_EQ(actual1, 6);
}

TEST(multi_sequential_map, valueDefault) {
  sequential_multi_map<std::string, int> m;
  m["test0"] = 5;
  auto actual = m.value("not_exists", 10);
  EXPECT_EQ(actual, 10);
  EXPECT_EQ(m.count(), 1);
}

TEST(multi_sequential_map, pushBack) {
  sequential_multi_map<std::string, int> m;
  m.push_back(std::make_pair("test1", 10));
  m.push_back(std::make_pair("test1", 5));
  auto values = m.values();
  EXPECT_EQ(values.size(), 2);
  EXPECT_EQ(values.at(0).first, "test1");
  EXPECT_EQ(values.at(0).second, 10);
  EXPECT_EQ(values.at(1).first, "test1");
  EXPECT_EQ(values.at(1).second, 5);
}
