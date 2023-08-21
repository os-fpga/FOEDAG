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

#include "Main/ProjectFile/ProjectFileComponent.h"

#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(ErrorCode, constructorDefault) {
  ErrorCode erc{};
  EXPECT_EQ(erc.hasError(), false);
  EXPECT_EQ(erc.message(), QString{});
}

TEST(ErrorCode, constructor) {
  QString errorStr{"Some error"};
  ErrorCode erc{true, errorStr};
  EXPECT_EQ(erc.hasError(), true);
  EXPECT_EQ(erc.message(), errorStr);
}

TEST(ErrorCode, operatorBool) {
  QString errorStr{"Some error"};
  ErrorCode erc{true, errorStr};
  EXPECT_EQ((bool)erc, true);
}

TEST(Version, Equal) {
  Version v1{0, 0, 0};
  Version v2{0, 0, 0};
  bool notEqual = (v1 != v2);
  EXPECT_EQ(notEqual, false);
}

TEST(Version, NotEqualMaj) {
  Version v1{0, 0, 0};
  Version v2{1, 0, 0};
  bool notEqual = (v1 != v2);
  EXPECT_EQ(notEqual, true);
}

TEST(Version, NotEqualMin) {
  Version v1{0, 0, 0};
  Version v2{0, 1, 0};
  bool notEqual = (v1 != v2);
  EXPECT_EQ(notEqual, true);
}

TEST(Version, NotEqualPatch) {
  Version v1{0, 0, 0};
  Version v2{0, 0, 1};
  bool notEqual = (v1 != v2);
  EXPECT_EQ(notEqual, true);
}

TEST(Version, OperatorLess) {
  Version v1{0, 0, 0};
  Version v2{0, 0, 0};
  bool less = (v1 < v2);
  EXPECT_EQ(less, false);
}

TEST(Version, OperatorLessCase0) {
  Version v1{1, 0, 0};
  Version v2{0, 0, 0};
  bool less = (v1 < v2);
  EXPECT_EQ(less, false);
}

TEST(Version, OperatorLessCase1) {
  Version v1{0, 0, 0};
  Version v2{1, 0, 0};
  bool less = (v1 < v2);
  EXPECT_EQ(less, true);
}

TEST(Version, OperatorLessCase2) {
  Version v1{0, 1, 0};
  Version v2{0, 0, 0};
  bool less = (v1 < v2);
  EXPECT_EQ(less, false);
}

TEST(Version, OperatorLessCase3) {
  Version v1{0, 0, 0};
  Version v2{0, 1, 0};
  bool less = (v1 < v2);
  EXPECT_EQ(less, true);
}

TEST(Version, OperatorLessCase4) {
  Version v1{0, 0, 1};
  Version v2{0, 0, 0};
  bool less = (v1 < v2);
  EXPECT_EQ(less, false);
}

TEST(Version, OperatorLessCase5) {
  Version v1{0, 0, 0};
  Version v2{0, 0, 1};
  bool less = (v1 < v2);
  EXPECT_EQ(less, true);
}

TEST(Version, OperatorLessCase6) {
  Version v1{0, 1, 0};
  Version v2{0, 0, 1};
  bool less = (v1 < v2);
  EXPECT_EQ(less, false);
}

TEST(Version, OperatorLessCase7) {
  Version v1{1, 0, 0};
  Version v2{0, 1, 0};
  bool less = (v1 < v2);
  EXPECT_EQ(less, false);
}

TEST(Version, OperatorLessCase8) {
  Version v1{1, 0, 0};
  Version v2{0, 0, 1};
  bool less = (v1 < v2);
  EXPECT_EQ(less, false);
}

TEST(Version, toString) {
  Version v{1, 2, 3};
  QString str = toString(v);
  EXPECT_EQ(str, "1.2.3");
}

TEST(Version, toVersionFailedString) {
  QString str{"some string"};
  bool ok{};
  Version v = toVersion(str, &ok);
  EXPECT_EQ(ok, false);
  EXPECT_EQ(v, Version{});
}

TEST(Version, toVersion2Nums) {
  QString str{"2.9"};
  bool ok{};
  Version v = toVersion(str, &ok);
  EXPECT_EQ(ok, false);
  EXPECT_EQ(v, Version{});
}

TEST(Version, toVersionNotNumber0) {
  QString str{"notnumber.9.0"};
  bool ok{};
  Version v = toVersion(str, &ok);
  EXPECT_EQ(ok, false);
  EXPECT_EQ(v, Version{});
}

TEST(Version, toVersionNotNumber1) {
  QString str{"2.notnumber.0"};
  bool ok{};
  Version v = toVersion(str, &ok);
  EXPECT_EQ(ok, false);
  EXPECT_EQ(v, Version{});
}

TEST(Version, toVersionNotNumber2) {
  QString str{"2.9.notnumber"};
  bool ok{};
  Version v = toVersion(str, &ok);
  EXPECT_EQ(ok, false);
  EXPECT_EQ(v, Version{});
}

TEST(Version, toVersion) {
  QString str{"2.9.0"};
  bool ok{};
  Version v = toVersion(str, &ok);
  Version expected{2, 9, 0};
  EXPECT_EQ(ok, true);
  EXPECT_EQ(v, expected);
}

TEST(Version, toVersionNoVerification) {
  QString str{"2.9.0"};
  Version v = toVersion(str);
  Version expected{2, 9, 0};
  EXPECT_EQ(v, expected);
}

TEST(Version, toVersionNoVerificationFailed) {
  QString str{"some str"};
  Version v = toVersion(str);
  Version expected{};
  EXPECT_EQ(v, expected);
}
