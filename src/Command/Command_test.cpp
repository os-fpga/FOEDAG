/*
 Copyright 2021 The Foedag Team

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
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
