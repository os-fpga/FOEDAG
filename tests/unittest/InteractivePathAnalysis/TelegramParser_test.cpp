#include "InteractivePathAnalysis/client/TelegramParser.h"

#include "gtest/gtest.h"

TEST(TelegramParser, base)
{
    const std::string tData{R"({"JOB_ID":"7","CMD":"2","OPTIONS":"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3","DATA":"some_data...","STATUS":"1"})"};

    EXPECT_EQ(std::optional<int>{7}, comm::TelegramParser::tryExtractFieldJobId(tData));
    EXPECT_EQ(std::optional<int>{2}, comm::TelegramParser::tryExtractFieldCmd(tData));
    std::optional<std::string> optionsOpt;
    EXPECT_TRUE(comm::TelegramParser::tryExtractFieldOptions(tData, optionsOpt));
    EXPECT_EQ(std::optional<std::string>{"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3"}, optionsOpt.value());
    std::optional<std::string> dataOpt;
    EXPECT_TRUE(comm::TelegramParser::tryExtractFieldData(tData, dataOpt));
    EXPECT_EQ(std::optional<std::string>{"some_data..."}, dataOpt.value());
    EXPECT_EQ(std::optional<int>{1}, comm::TelegramParser::tryExtractFieldStatus(tData));
}

TEST(TelegramParser, invalidKeys)
{
    const std::string tBadData{R"({"_JOB_ID":"7","_CMD":"2","_OPTIONS":"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3","_DATA":"some_data...","_STATUS":"1"})"};
    
    EXPECT_EQ(std::nullopt, comm::TelegramParser::tryExtractFieldJobId(tBadData));
    EXPECT_EQ(std::nullopt, comm::TelegramParser::tryExtractFieldCmd(tBadData));
    std::optional<std::string> optionsOpt;
    EXPECT_FALSE(comm::TelegramParser::tryExtractFieldOptions(tBadData, optionsOpt));
    EXPECT_EQ(std::nullopt, optionsOpt);
    std::optional<std::string> dataOpt;
    EXPECT_FALSE(comm::TelegramParser::tryExtractFieldData(tBadData, dataOpt));
    EXPECT_EQ(std::nullopt, dataOpt);
    EXPECT_EQ(std::nullopt, comm::TelegramParser::tryExtractFieldStatus(tBadData));
}

TEST(TelegramParser, invalidTypes)
{
    const std::string tBadData{R"({"JOB_ID":"x","CMD":"y","STATUS":"z"})"};
    
    EXPECT_EQ(std::nullopt, comm::TelegramParser::tryExtractFieldJobId(tBadData));
    EXPECT_EQ(std::nullopt, comm::TelegramParser::tryExtractFieldCmd(tBadData));
    EXPECT_EQ(std::nullopt, comm::TelegramParser::tryExtractFieldStatus(tBadData));
}