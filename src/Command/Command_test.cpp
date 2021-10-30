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

#include "Command/CommandStack.h"
#include "Tcl/TclInterpreter.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;

namespace FOEDAG {
namespace {
TEST(Command, TestPush) {
  TclInterpreter interpreter;
  Command cmd("puts \"Hello Foedag, you have Tcl\"", "puts \"Bye!\"");
  CommandStack cmds(&interpreter);
  bool ok = cmds.push_and_exec(&cmd);
  ok &= cmds.pop_and_undo();

  EXPECT_EQ(ok, true);
}

}  // namespace
}  // namespace FOEDAG
