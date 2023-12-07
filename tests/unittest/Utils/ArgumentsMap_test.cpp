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

#include "Utils/ArgumentsMap.h"

#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(ArgumentsMap, toStringOneArg) {
  ArgumentsMap args;
  args.addArgument("key", "value");
  EXPECT_EQ(args.toString(), "-key value");
}

TEST(ArgumentsMap, toStringTwoArg) {
  ArgumentsMap args;
  args.addArgument("key1", "value");
  args.addArgument("key2", "value");
  EXPECT_EQ(args.toString(), "-key1 value -key2 value");
}

TEST(ArgumentsMap, toStringUniqueArg) {
  ArgumentsMap args;
  args.addArgument("key1", "value1");
  args.addArgument("key1", "value2");
  EXPECT_EQ(args.toString(), "-key1 value2");
}

TEST(ArgumentsMap, toStringNoValue) {
  ArgumentsMap args;
  args.addArgument("key1", "value1");
  args.addArgument("key2");
  EXPECT_EQ(args.toString(), "-key1 value1 -key2");
}

TEST(ArgumentsMap, parseArgumentsEmpty) {
  std::string test{};
  auto args = parseArguments(test);
  EXPECT_EQ(args.keys().size(), 0);
}

TEST(ArgumentsMap, parseArgumentsOneArg) {
  std::string test{"-key value"};
  auto args = parseArguments(test);
  EXPECT_EQ(args.keys().size(), 1);
  EXPECT_EQ(args.hasKey("key"), true);
  const ArgValue expetced{true, std::string{"value"}};
  EXPECT_EQ(args.value("key"), expetced);
}

TEST(ArgumentsMap, parseArgumentsNoValues) {
  std::string test{"-key1 -key2"};
  auto args = parseArguments(test);
  EXPECT_EQ(args.keys().size(), 2);
  EXPECT_EQ(args.hasKey("key1"), true);
  EXPECT_EQ(args.hasKey("key2"), true);
  EXPECT_EQ(args.value("key1"), std::string{});
  EXPECT_EQ(args.value("key2"), std::string{});
}

TEST(ArgumentsMap, parseArgumentsOneValueTwoKeys) {
  std::string test{"-key1 val -key2"};
  auto args = parseArguments(test);
  EXPECT_EQ(args.keys().size(), 2);
  EXPECT_EQ(args.hasKey("key1"), true);
  EXPECT_EQ(args.hasKey("key2"), true);
  EXPECT_EQ(args.value("key1"), std::string{"val"});
  EXPECT_EQ(args.value("key2"), std::string{});
}

TEST(ArgumentsMap, takeValue) {
  std::string test{"-key1 val -key2"};
  auto args = parseArguments(test);
  auto val = args.takeValue("key1");
  EXPECT_EQ(val.exist, true);
  EXPECT_EQ(val, "val");
  EXPECT_EQ(args.hasKey("key1"), false);
  EXPECT_EQ(args.hasKey("key2"), true);
}

TEST(ArgumentsMap, takeValueNotExists) {
  std::string test{"-key1 val -key2"};
  auto args = parseArguments(test);
  auto val = args.takeValue("key3");
  ArgValue expected{false, std::string{}};
  EXPECT_EQ(val, expected);
  EXPECT_EQ(args.hasKey("key1"), true);
  EXPECT_EQ(args.hasKey("key2"), true);
}
