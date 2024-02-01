#include "InteractivePathAnalysis/ConvertUtils.h"

#include "gtest/gtest.h"

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


