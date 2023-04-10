/*
Copyright 2023 The Foedag team

GPL License

Copyright (c) 2023 The Open-Source FPGA Foundation

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

#include "CFGCompiler.h"

#include "gmock/gmock.h"

namespace {

TEST(CFGCommon, test_arg_default) {
  CFG_POST_MSG("This is CFGCompiler unit test");
  EXPECT_EQ(1, 1);  // dummy tests to make sure it is built and run
}

}  // end anonymous namespace
