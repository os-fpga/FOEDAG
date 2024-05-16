#include "InteractivePathAnalysis/client/ZlibUtils.h"

#include "gtest/gtest.h"

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





