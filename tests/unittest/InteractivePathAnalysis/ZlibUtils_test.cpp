/**
  * @file ZlibUtils_test.cpp
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

#include "InteractivePathAnalysis/client/ZlibUtils.h"

#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(ZlibUtils, compressionAndDecompression)
{
    const std::string orig{"This string is going to be compressed now"};

    std::optional<std::string> compressedOpt = tryCompress(orig);
    EXPECT_TRUE(compressedOpt);
    std::optional<std::string> decompressedOpt = tryDecompress(compressedOpt.value());
    EXPECT_TRUE(decompressedOpt);

    EXPECT_TRUE(orig != compressedOpt.value());
    EXPECT_EQ(orig, decompressedOpt.value());
}





