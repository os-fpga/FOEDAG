/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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

#include <string>
#include <string_view>
#include <vector>

#include "Tcl/TclInterpreter.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;

namespace FOEDAG {
namespace {
TEST(HelloTcl, HelloWorld) {
  TclInterpreter interpreter;
  std::string result =
      interpreter.evalCmd("puts \"Hello Foedag, you have Tcl\"");
  EXPECT_EQ(result, "");
}

TEST(HelloTcl, TestTclError) {
  TclInterpreter interpreter;
  std::string result =
      interpreter.evalCmd("putsss \"Hello Foedag, you have Tcl\"");
  std::string expected =
      R"(invalid command name "putsss"
    while executing
"putsss "Hello Foedag, you have Tcl"")";
  EXPECT_EQ(result, expected);
}

}  // namespace
}  // namespace FOEDAG
