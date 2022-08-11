#include <string>

#include "Utils/StringUtils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;

namespace FOEDAG {
namespace {

struct TrimTest: testing::Test {
    std::string initial_str;

    TrimTest() : initial_str{" Standard Template Library = STL  "} {}
};

TEST_F(TrimTest, ltrimTest) {
  const auto ltrimmed_str = StringUtils::ltrim(initial_str);
  EXPECT_EQ(ltrimmed_str, "Standard Template Library = STL  ");
}

TEST_F(TrimTest, rtrimTest) {
    const auto rltrimmed_str = StringUtils::rtrim(initial_str);
    EXPECT_EQ(rltrimmed_str, " Standard Template Library = STL");
}

TEST_F(TrimTest, trimTest) {
    const auto trimmed_str = StringUtils::trim(initial_str);
    EXPECT_EQ(trimmed_str, "Standard Template Library = STL");
}

TEST_F(TrimTest, characterLTrimTest) {
    auto ltrimmed_str = StringUtils::ltrim(initial_str, '=');
    EXPECT_EQ(ltrimmed_str, " STL  ");
}

TEST_F(TrimTest, characterRTrimTest) {
    const auto rtrimmed_str = StringUtils::rtrim(initial_str, '=');
    EXPECT_EQ(rtrimmed_str, " Standard Template Library ");
}

TEST_F(TrimTest, rtrimEqualTest) {
    const auto rtrimmed_equal_str = StringUtils::rtrimEqual(initial_str);
    EXPECT_EQ(rtrimmed_equal_str, " Standard Template Library ");
}

struct MultiLineTest: testing::Test {
    std::string multiline_str;

    MultiLineTest() : multiline_str{"This program is free software.\nThis program is distributed.\nHope that this program will be useful."} {}
};

TEST_F(MultiLineTest, splitLinesTest) {
    const auto lines = StringUtils::splitLines(multiline_str);
    ASSERT_EQ(lines.size(), 3);
    EXPECT_EQ(*lines.rbegin(), "Hope that this program will be useful.");
}

TEST_F(MultiLineTest, lineInStringTest) {
    const auto last_line = StringUtils::getLineInString(multiline_str, 3);
    EXPECT_EQ(last_line, "Hope that this program will be useful.");
}

TEST_F(MultiLineTest, replaceStringTest) {
    const auto replaced_str = StringUtils::replaceAll(multiline_str, "This program", "It");
    EXPECT_EQ(replaced_str, "It is free software.\nIt is distributed.\nHope that this program will be useful.");
}

TEST_F(MultiLineTest, tokenizeTest) {
    auto tokenized_lines = std::vector<std::string>{};
    StringUtils::tokenize(multiline_str, "\n", tokenized_lines);
    ASSERT_EQ(tokenized_lines.size(), 3);
    EXPECT_EQ(*tokenized_lines.rbegin(), "Hope that this program will be useful.");
}

TEST(StringUtilsTest, LeafTest) {
    const auto input_str = std::string("tests.unittest.utils");
    const auto leaf_str = StringUtils::leaf(input_str);
    EXPECT_EQ(leaf_str, "utils");
}

TEST(StringUtilsTest, toStringTest) {
    const auto dbl_str = std::string("3.000");
    ASSERT_EQ(StringUtils::to_string(3.0), dbl_str);
}

TEST(StringUtilsTest, removeCommentsTest) {
    const auto comment = std::string("// Index initialization");
    const auto code = std::string("\n auto i = 0;");
    EXPECT_EQ(StringUtils::removeComments(comment + code), code);
}

TEST(StringUtilsTest, unquotedTest) {
    const auto unittest_str = std::string("unquotedTest");
    const auto quoted_unittest = "\"" + unittest_str + "\"";
    EXPECT_EQ(StringUtils::unquoted(quoted_unittest), unittest_str);
}

}  // namespace
}  // namespace FOEDAG
