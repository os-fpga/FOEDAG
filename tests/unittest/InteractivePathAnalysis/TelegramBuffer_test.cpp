#include "InteractivePathAnalysis/client/TelegramBuffer.h"

#include "gtest/gtest.h"


TEST(ByteArray, Base)
{
    comm::ByteArray array1{"111"};
    comm::ByteArray array2{"222"};
    comm::ByteArray array{array1};
    array.append(array2);

    EXPECT_EQ('1', array.at(0));
    EXPECT_EQ('1', array.at(1));
    EXPECT_EQ('1', array.at(2));
    EXPECT_EQ('2', array.at(3));
    EXPECT_EQ('2', array.at(4));
    EXPECT_EQ('2', array.at(5));

    EXPECT_EQ("111222", array.to_string());

    EXPECT_EQ(6, array.size());

    array.append('3');

    EXPECT_EQ(7, array.size());
    EXPECT_EQ("1112223", array.to_string());

    EXPECT_EQ('3', array.at(6));

    array.clear();

    EXPECT_EQ(0, array.size());
    EXPECT_EQ("", array.to_string());
}

TEST(TelegramBuffer, OneOpened)
{
    comm::TelegramBuffer buff{1024};
    buff.append(comm::ByteArray{"111"});
    buff.append(comm::ByteArray{"222"});

    auto frames = buff.takeTelegramFrames();
    EXPECT_EQ(0, frames.size());

    EXPECT_EQ("111222", buff.data().to_string());
}

TEST(TelegramBuffer, OneFinishedOneOpened)
{
    comm::TelegramBuffer buff{1024};
    buff.append(comm::ByteArray{"111\x17"});
    buff.append(comm::ByteArray{"222"});

    auto frames = buff.takeTelegramFrames();
    EXPECT_EQ(1, frames.size());

    EXPECT_EQ("111", frames[0]->data.to_string());

    EXPECT_EQ("222", buff.data().to_string());
}

TEST(TelegramBuffer, TwoFinished)
{
    comm::TelegramBuffer buff{1024};
    buff.append(comm::ByteArray{"111\x17"});
    buff.append(comm::ByteArray{"222\x17"});

    auto frames = buff.takeTelegramFrames();
    EXPECT_EQ(2, frames.size());

    EXPECT_EQ("111", frames[0]->data.to_string());
    EXPECT_EQ("222", frames[1]->data.to_string());

    EXPECT_EQ("", buff.data().to_string());
}

TEST(TelegramBuffer, TwoCleared)
{
    comm::TelegramBuffer buff{1024};
    buff.append(comm::ByteArray{"111\x17"});
    buff.append(comm::ByteArray{"222\x17"});

    buff.clear();

    auto frames = buff.takeTelegramFrames();
    EXPECT_EQ(0, frames.size());

    EXPECT_EQ("", buff.data().to_string());
}
