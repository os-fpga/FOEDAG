#include "InteractivePathAnalysis/client/TelegramParser.h"

#include "gtest/gtest.h"

TEST(TelegramParser, base)
{
    const std::string tdata{R"({"JOB_ID":"7","CMD":"2","OPTIONS":"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3","DATA":"some_data...","STATUS":"1"})"};

    EXPECT_EQ(std::optional<int>{7}, comm::TelegramParser::tryExtractFieldJobId(tdata));
    EXPECT_EQ(std::optional<int>{2}, comm::TelegramParser::tryExtractFieldCmd(tdata));
    std::optional<std::string> optionsOpt;
    EXPECT_TRUE(comm::TelegramParser::tryExtractFieldOptions(tdata, optionsOpt));
    EXPECT_EQ(std::optional<std::string>{"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3"}, optionsOpt.value());
    std::optional<std::string> dataOpt;
    EXPECT_TRUE(comm::TelegramParser::tryExtractFieldData(tdata, dataOpt));
    EXPECT_EQ(std::optional<std::string>{"some_data..."}, dataOpt.value());
    EXPECT_EQ(std::optional<int>{1}, comm::TelegramParser::tryExtractFieldStatus(tdata));
}


