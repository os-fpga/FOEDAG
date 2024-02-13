#include "InteractivePathAnalysis/client/TelegramParser.h"

#include "gtest/gtest.h"

TEST(TelegramParser, base)
{
    const std::string tdata{R"({"JOB_ID":"7","CMD":"2","OPTIONS":"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3","DATA":"some_data...","STATUS":"1"})"};

    EXPECT_EQ(std::optional<int>{7}, comm::TelegramParser::tryExtractFieldJobId(tdata));
    EXPECT_EQ(std::optional<int>{2}, comm::TelegramParser::tryExtractFieldCmd(tdata));
    EXPECT_EQ(std::optional<std::string>{"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3"}, comm::TelegramParser::tryExtractFieldOptions(tdata));
    EXPECT_EQ(std::optional<std::string>{"some_data..."}, comm::TelegramParser::tryExtractFieldData(tdata));
    EXPECT_EQ(std::optional<int>{1}, comm::TelegramParser::tryExtractFieldStatus(tdata));
}


