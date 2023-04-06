#include "Utils/StringUtils.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;

namespace FOEDAG {
namespace {

struct TrimTest : testing::Test {
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

struct MultiLineTest : testing::Test {
  std::string multiline_str;

  MultiLineTest()
      : multiline_str{
            "This program is free software.\nThis program is "
            "distributed.\nHope that this program will be useful."} {}
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
  const auto replaced_str =
      StringUtils::replaceAll(multiline_str, "This program", "It");
  EXPECT_EQ(replaced_str,
            "It is free software.\nIt is distributed.\nHope that this program "
            "will be useful.");
}

TEST_F(MultiLineTest, tokenizeTest) {
  auto tokenized_lines = StringUtils::tokenize(multiline_str, "\n");
  ASSERT_EQ(tokenized_lines.size(), 3);
  EXPECT_EQ(*tokenized_lines.rbegin(),
            "Hope that this program will be useful.");
}

TEST_F(MultiLineTest, tokenizeNoSeparator) {
  std::string testStr{"test0test1test2"};
  auto tokenized_lines = StringUtils::tokenize(testStr, "\n");
  ASSERT_EQ(tokenized_lines.size(), 1);
  EXPECT_EQ(*tokenized_lines.rbegin(), "test0test1test2");
}

TEST_F(MultiLineTest, tokenizeLongSeparator_0) {
  std::string testStr{"test0SEPARATORtest1SEPARATORtest2"};
  auto tokenized_lines = StringUtils::tokenize(testStr, "SEPARATOR");
  ASSERT_EQ(tokenized_lines.size(), 3);
  EXPECT_EQ(*tokenized_lines.rbegin(), "test2") << *tokenized_lines.rbegin();
}

TEST_F(MultiLineTest, tokenizeLongSeparator_1) {
  std::string testStr{"test0SEPtest1SEPARATORtest2"};
  auto tokenized_lines = StringUtils::tokenize(testStr, "SEPARATOR");
  ASSERT_EQ(tokenized_lines.size(), 2);
  EXPECT_EQ(*tokenized_lines.begin(), "test0SEPtest1");
}

TEST_F(MultiLineTest, tokenizeLongSeparator_2) {
  std::string testStr{"test0SEPtest1SEPtest2SEP"};
  auto tokenized_lines = StringUtils::tokenize(testStr, "SEPARATOR");
  ASSERT_EQ(tokenized_lines.size(), 1);
  EXPECT_EQ(*tokenized_lines.begin(), "test0SEPtest1SEPtest2SEP");
}

TEST_F(MultiLineTest, tokenizeTestSkipEmpty_0) {
  auto testStr{"  string   with   empty    spaces  "};
  auto tokenized_lines = StringUtils::tokenize(testStr, " ", true);
  ASSERT_EQ(tokenized_lines.size(), 4);
  EXPECT_EQ(*tokenized_lines.begin(), "string");
  EXPECT_EQ(*(tokenized_lines.begin() + 1), "with");
  EXPECT_EQ(*(tokenized_lines.begin() + 2), "empty");
  EXPECT_EQ(*(tokenized_lines.begin() + 3), "spaces");
}

TEST_F(MultiLineTest, tokenizeTestSkipEmpty_1) {
  auto testStr{"string  with empty spaces"};
  auto tokenized_lines = StringUtils::tokenize(testStr, " ", true);
  ASSERT_EQ(tokenized_lines.size(), 4);
  EXPECT_EQ(*tokenized_lines.begin(), "string");
  EXPECT_EQ(*(tokenized_lines.begin() + 1), "with");
  EXPECT_EQ(*(tokenized_lines.begin() + 2), "empty");
  EXPECT_EQ(*(tokenized_lines.begin() + 3), "spaces");
}

TEST(StringUtilsTest, LeafTest) {
  const auto input_str = std::string("tests.unittest.utils");
  const auto leaf_str = StringUtils::leaf(input_str);
  EXPECT_EQ(leaf_str, "utils");
}

TEST(StringUtilsTest, toStringTest) {
  const auto dbl_str = std::string("3.000");
  auto res{StringUtils::to_string(3.0, 3)};
  ASSERT_EQ(res, dbl_str);
}

TEST(StringUtilsTest, toNumberTest) {
  const auto str = std::string("337");
  auto [num, ok]{StringUtils::to_number<int>(str)};
  ASSERT_EQ(ok, true);
  ASSERT_EQ(num, 337);
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

TEST(StringUtilsTest, endsWithTest) {
  EXPECT_EQ(StringUtils::endsWith("string123", "123"), true);
  EXPECT_EQ(StringUtils::endsWith("string123 ", "123"), false);
  EXPECT_EQ(StringUtils::endsWith("string123", "12"), false);
}

TEST(StringUtilsTest, toLowerTest) {
    EXPECT_EQ(StringUtils::toLower("LOWERCASE"), std::string("lowercase"));
}

TEST(StringUtilsTest, toUpperTest) {
    EXPECT_EQ(StringUtils::toUpper("upperCase"), std::string("UPPERCASE"));
}

TEST(StringUtilsTest, joinEmpty) {
  std::vector<std::string> join{};
  auto res = StringUtils::join(join, " ");
  EXPECT_EQ(res, "");
}

TEST(StringUtilsTest, joinOneElement) {
  std::vector<std::string> join{"element"};
  auto res = StringUtils::join(join, "////");
  EXPECT_EQ(res, "element");
}

TEST(StringUtilsTest, joinFewElements) {
  std::vector<std::string> join{"element1", "element2"};
  auto res = StringUtils::join(join, "---");
  EXPECT_EQ(res, "element1---element2");
}

TEST(StringUtilsTest, containsEmpty) {
  StringVector empty{};
  auto res = StringUtils::contains(empty, "any");
  EXPECT_EQ(res, false);
}

TEST(StringUtilsTest, containsFound) {
  StringVector test{"test1", "test2"};
  auto res = StringUtils::contains(test, "test2");
  EXPECT_EQ(res, true);
}

TEST(StringUtilsTest, containsNotFound) {
  StringVector test{"test1", "test2"};
  auto res = StringUtils::contains(test, "test3");
  EXPECT_EQ(res, false);
}

TEST(StringUtilsTest, setArgumentValueEmpty) {
  StringVector test{};
  StringUtils::setArgumentValue(test, "arg", "value");
  StringVector expected{"arg", "value"};
  EXPECT_EQ(test, expected);
}

TEST(StringUtilsTest, setArgumentValueExists) {
  StringVector test{"arg", "value"};
  StringUtils::setArgumentValue(test, "arg", "value1");
  StringVector expected{"arg", "value1"};
  EXPECT_EQ(test, expected);
}

}  // namespace
}  // namespace FOEDAG
