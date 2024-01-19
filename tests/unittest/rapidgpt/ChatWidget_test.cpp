/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rapidgpt/ChatWidget.h"

#include "gtest/gtest.h"
using namespace FOEDAG;

TEST(ChatWidget, addMessage) {
  ChatWidget chat;
  Message message{{"test"}};
  chat.addMessage(message);

  EXPECT_EQ(chat.count(), 1);
}

TEST(ChatWidget, clear) {
  ChatWidget chat;
  Message message{{"test"}};
  chat.addMessage(message);

  EXPECT_EQ(chat.count(), 1);
  chat.clear();
  EXPECT_EQ(chat.count(), 0);
}

TEST(ChatWidget, removeAt) {
  ChatWidget chat;
  Message message{{"test"}};
  chat.addMessage(message);
  chat.addMessage(message);

  EXPECT_EQ(chat.count(), 2);
  chat.removeAt(0);
  EXPECT_EQ(chat.count(), 1);
  chat.removeAt(0);
  EXPECT_EQ(chat.count(), 0);
}
