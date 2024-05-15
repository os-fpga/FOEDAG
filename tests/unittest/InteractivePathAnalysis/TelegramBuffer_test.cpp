/**
  * @file TelegramBuffer_test.cpp
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

#include "InteractivePathAnalysis/client/TelegramBuffer.h"

#include "gtest/gtest.h"

using namespace FOEDAG;

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

TEST(TelegramBuffer, NotFilledTelegramButWithPrependedRubish)
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray rubish{"#@!"};
    const comm::ByteArray msgBody{"some message"};
    const comm::TelegramHeader msgHeader{comm::TelegramHeader::constructFromData(msgBody)};

    tBuff.append(rubish);
    tBuff.append(msgHeader.buffer());

    auto frames = tBuff.takeTelegramFrames();
    EXPECT_EQ(0, frames.size());

    EXPECT_EQ(msgHeader.buffer(), tBuff.data()); // the rubish prefix fragment will be absent here
}

TEST(TelegramBuffer, OneFinishedOneOpened)
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray msgBody1{"message1"};
    const comm::ByteArray msgBody2{"message2"};

    const comm::TelegramHeader msgHeader1{comm::TelegramHeader::constructFromData(msgBody1)};
    const comm::TelegramHeader msgHeader2{comm::TelegramHeader::constructFromData(msgBody2)};

    comm::ByteArray t1(msgHeader1.buffer());
    t1.append(msgBody1);

    comm::ByteArray t2(msgHeader2.buffer());
    t2.append(msgBody2);
    t2.resize(t2.size()-2); // drop 2 last elements

    tBuff.append(t1);
    tBuff.append(t2);

    auto frames = tBuff.takeTelegramFrames();
    EXPECT_EQ(1, frames.size());

    EXPECT_EQ(msgBody1, frames[0]->data);

    EXPECT_EQ(t2, tBuff.data());
}

TEST(TelegramBuffer, TwoFinished)
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray msgBody1{"message1"};
    const comm::ByteArray msgBody2{"message2"};

    const comm::TelegramHeader msgHeader1{comm::TelegramHeader::constructFromData(msgBody1)};
    const comm::TelegramHeader msgHeader2{comm::TelegramHeader::constructFromData(msgBody2)};

    comm::ByteArray t1(msgHeader1.buffer());
    t1.append(msgBody1);

    comm::ByteArray t2(msgHeader2.buffer());
    t2.append(msgBody2);

    tBuff.append(t1);
    tBuff.append(t2);

    auto frames = tBuff.takeTelegramFrames();
    EXPECT_EQ(2, frames.size());

    EXPECT_EQ(msgBody1, frames[0]->data);
    EXPECT_EQ(msgBody2, frames[1]->data);

    EXPECT_EQ(comm::ByteArray{}, tBuff.data());
}

TEST(TelegramBuffer, Clear)
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray msgBody1{"message1"};
    const comm::ByteArray msgBody2{"message2"};

    const comm::TelegramHeader msgHeader1{comm::TelegramHeader::constructFromData(msgBody1)};
    const comm::TelegramHeader msgHeader2{comm::TelegramHeader::constructFromData(msgBody2)};

    comm::ByteArray t1(msgHeader1.buffer());
    t1.append(msgBody1);

    comm::ByteArray t2(msgHeader2.buffer());
    t2.append(msgBody2);

    tBuff.clear();

    auto frames = tBuff.takeTelegramFrames();
    EXPECT_EQ(0, frames.size());

    EXPECT_EQ(comm::ByteArray{}, tBuff.data());
}
