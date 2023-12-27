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

#include "rapidgpt/RapidGpt.h"

#include "gtest/gtest.h"
#include "qdebug.h"

using namespace FOEDAG;

TEST(RapidGpt, sendWithoutKey) {
  RapidGptContext context;
  context.data.push_back("test");
  RapidGpt rapigGpt{{}};
  auto result = rapigGpt.send(context);
  EXPECT_EQ(result, false);
  EXPECT_EQ(context.data.count(), 1);
}

TEST(RapidGpt, sendEmptyContext) {
  RapidGptContext context;
  RapidGpt rapigGpt{{}};
  auto result = rapigGpt.send(context);
  EXPECT_EQ(result, false);
}

TEST(RapidGpt, sendWrongKey) {
  RapidGptContext context;
  context.data.push_back("test");
  RapidGpt rapigGpt{"key"};
  auto result = rapigGpt.send(context);
  EXPECT_EQ(result, false);
  EXPECT_EQ(rapigGpt.errorString().isEmpty(), false);
}
