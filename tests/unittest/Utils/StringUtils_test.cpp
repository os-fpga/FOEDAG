#include <string>

#include "Utils/StringUtils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;

namespace FOEDAG {
namespace {

struct TrimTest: testing::Test {
    std::string initial_str;

    TrimTest() : initial_str{" Google Test = GTest  "} {}
};

TEST_F(TrimTest, SideTrimTest) {
  auto input_str = initial_str;

  const auto ltrimmed_str = StringUtils::ltrim(input_str);
  EXPECT_EQ(ltrimmed_str, "Google Test = GTest  ");

  const auto rltrimmed_str = StringUtils::rtrim(input_str);
  EXPECT_EQ(rltrimmed_str, "Google Test = GTest");

  const auto trimmed_str = StringUtils::trim(initial_str);
  EXPECT_EQ(trimmed_str, rltrimmed_str);
}

TEST_F(TrimTest, CharacterTrimTest) {
auto ltrimmed_str = StringUtils::ltrim(initial_str, 'g');
EXPECT_EQ(ltrimmed_str, "le Test = GTest  ");

const auto rtrimmed_str = StringUtils::rtrim(ltrimmed_str, '=');
EXPECT_EQ(rtrimmed_str, "le Test ");

const auto rtrimmed_equal_str = StringUtils::rtrimEqual(initial_str);
EXPECT_EQ(rtrimmed_str, rtrimmed_equal_str);
}

TEST(StringUtilsTest, LeafTest) {
    const auto input_str = std::string("RapidSilicon.Raptor.Foedag");
    const auto leaf_str = StringUtils::leaf(input_str);
    EXPECT_EQ(leaf_str, "Foedag");
}

TEST(StringUtilsTest, MultiLineTest) {
    const auto input_str = std::string("This program is free software.\nThis program is distributed.\nHope that this program will be useful.");
    const auto lines = StringUtils::splitLines(input_str);
    ASSERT_EQ(lines.size(), 3);

    const auto last_line = StringUtils::getLineInString(input_str, lines.size());
    EXPECT_EQ(*lines.rbegin(), last_line);

    const auto replaced_str = StringUtils::replaceAll(last_line, "this program", "it");
    EXPECT_EQ(replaced_str, "Hope that it will be useful.");

    auto tokenized_lines = std::vector<std::string>{};
    StringUtils::tokenize(input_str, "\n", tokenized_lines);
    EXPECT_EQ(lines.size(), tokenized_lines.size());
}

TEST(StringUtilsTest, MiscTest) {
    const auto dbl_str = std::string("3.000");
    ASSERT_EQ(StringUtils::to_string(3.0), dbl_str);

    const auto comment = std::string("// Index initialization");
    const auto code = std::string("\n auto i = 0;");
    EXPECT_EQ(StringUtils::removeComments(comment + code), code);

    const auto quoted_double = "\"" + dbl_str + "\"";
    EXPECT_EQ(StringUtils::unquoted(quoted_double), dbl_str);
}

}  // namespace
}  // namespace FOEDAG
