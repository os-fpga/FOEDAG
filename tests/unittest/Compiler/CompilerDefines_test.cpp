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

#include "Compiler/CompilerDefines.h"

#include "gtest/gtest.h"
using namespace FOEDAG;
using namespace Design;

TEST(CompilerDefines, FromFileType) {
  EXPECT_EQ(FromFileType("v"), Language::VERILOG_2001);
  EXPECT_EQ(FromFileType("V"), Language::VERILOG_2001);
  EXPECT_EQ(FromFileType("sv"), Language::SYSTEMVERILOG_2017);
  EXPECT_EQ(FromFileType("SV"), Language::SYSTEMVERILOG_2017);
  EXPECT_EQ(FromFileType("sV"), Language::SYSTEMVERILOG_2017);
  EXPECT_EQ(FromFileType("vhd"), Language::VHDL_2008);
  EXPECT_EQ(FromFileType("VHD"), Language::VHDL_2008);
  EXPECT_EQ(FromFileType("vhD"), Language::VHDL_2008);
  EXPECT_EQ(FromFileType("blif"), Language::BLIF);
  EXPECT_EQ(FromFileType("BLIF"), Language::BLIF);
  EXPECT_EQ(FromFileType("bLIf"), Language::BLIF);
  EXPECT_EQ(FromFileType("eblif"), Language::EBLIF);
  EXPECT_EQ(FromFileType("EBLIF"), Language::EBLIF);
  EXPECT_EQ(FromFileType("EbLIf"), Language::EBLIF);

  EXPECT_EQ(FromFileType("C"), Language::C);
  EXPECT_EQ(FromFileType("c"), Language::C);
  EXPECT_EQ(FromFileType("cC"), Language::C);
  EXPECT_EQ(FromFileType("cPP"), Language::CPP);
  EXPECT_EQ(FromFileType("cpp"), Language::CPP);
  EXPECT_EQ(FromFileType("CPP"), Language::CPP);

  // default
  EXPECT_EQ(FromFileType("anything"), Language::OTHER);
}

TEST(CompilerDefines, FromFileTypePostSynth) {
  EXPECT_EQ(FromFileType("v", true), Language::VERILOG_NETLIST);
  EXPECT_EQ(FromFileType("V", true), Language::VERILOG_NETLIST);
  EXPECT_EQ(FromFileType("anything", true), Language::VERILOG_NETLIST);
}
