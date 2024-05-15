/**
  * @file ConvertUtils_test.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "InteractivePathAnalysis/client/ConvertUtils.h"

#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(ConvertUtils, toInt)
{
    EXPECT_EQ(std::optional<int>{-2}, tryConvertToInt("-2"));
    EXPECT_EQ(std::optional<int>{0}, tryConvertToInt("0"));
    EXPECT_EQ(std::optional<int>{2}, tryConvertToInt("2"));
    EXPECT_EQ(std::nullopt, tryConvertToInt("2."));
    EXPECT_EQ(std::nullopt, tryConvertToInt("2.0"));
    EXPECT_EQ(std::nullopt, tryConvertToInt("two"));
    EXPECT_EQ(std::nullopt, tryConvertToInt("2k"));
    EXPECT_EQ(std::nullopt, tryConvertToInt("k2"));
}
