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

#include "Compiler/Compiler.h"
#include "Configuration/CFGCompiler/CFGCompiler.h"
#include "gtest/gtest.h"

namespace FOEDAG {

Compiler compiler;
CFGCompiler cfgcompiler(&compiler);

TEST(CFGCompiler, test_RegisterCallbackFunction) {
  // New registration - OK
  EXPECT_EQ(cfgcompiler.RegisterCallbackFunction("abc", (cfg_callback_function)(0x123)), true);
  // Allow duplicated registration
  EXPECT_EQ(cfgcompiler.RegisterCallbackFunction("abc", (cfg_callback_function)(0x123)), true);
  // Does not allow duplicated registration but different callback function (conflict)
  EXPECT_EQ(cfgcompiler.RegisterCallbackFunction("abc", (cfg_callback_function)(0x456)), false);
  // New registration - OK
  EXPECT_EQ(cfgcompiler.RegisterCallbackFunction("xyz", (cfg_callback_function)(0x456)), true);
}

void test_callback_good_function(CFGCommon_ARG* cmdarg) {
  CFG_POST_MSG("This is good function");
}

void test_callback_bad_function(CFGCommon_ARG* cmdarg) {
  CFG_INTERNAL_ERROR("This is internal error by purpose");
}

TEST(CFGCompiler, test_CallbackFunction) {
  CFG_unset_callback_message_function();
  // New registration - OK
  EXPECT_EQ(cfgcompiler.RegisterCallbackFunction("good_testing", test_callback_good_function), true);
  EXPECT_EQ(cfgcompiler.RegisterCallbackFunction("bad_testing", test_callback_bad_function), true);
  // Call good function
  cfgcompiler.m_cmdarg.command = "good_testing";
  EXPECT_EQ(cfgcompiler.Configure(), true);
  // Call bad function
  cfgcompiler.m_cmdarg.command = "bad_testing";
  EXPECT_EQ(cfgcompiler.Configure(), false);
}

}
