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

#include "ProgrammerGui/SummaryProgressBar.h"

#include <QProgressBar>

#include "gtest/gtest.h"

using namespace FOEDAG;

TEST(SummaryProgressBar, progressBar) {
  SummaryProgressBar progressBar;
  EXPECT_NE(progressBar.progressBar(), nullptr);
}

TEST(SummaryProgressBar, AddProgressBar) {
  SummaryProgressBar progressBar;
  QProgressBar *bar1 = new QProgressBar;
  QProgressBar *bar2 = new QProgressBar;
  progressBar.AddProgressBar(bar1);
  progressBar.AddProgressBar(bar2);

  bar1->setValue(50);
  EXPECT_EQ(progressBar.progressBar()->value(), 25);

  bar2->setValue(50);
  EXPECT_EQ(progressBar.progressBar()->value(), 50);

  bar2->setValue(100);
  EXPECT_EQ(progressBar.progressBar()->value(), 75);

  bar1->setValue(100);
  EXPECT_EQ(progressBar.progressBar()->value(), 100);
}

TEST(SummaryProgressBar, AddProgressBarThreeBars) {
  SummaryProgressBar progressBar;
  QProgressBar *bar1 = new QProgressBar;
  QProgressBar *bar2 = new QProgressBar;
  QProgressBar *bar3 = new QProgressBar;
  progressBar.AddProgressBar(bar1);
  progressBar.AddProgressBar(bar2);
  progressBar.AddProgressBar(bar3);

  bar1->setValue(50);
  EXPECT_EQ(progressBar.progressBar()->value(), 17);

  bar2->setValue(50);
  EXPECT_EQ(progressBar.progressBar()->value(), 33);

  bar3->setValue(50);
  EXPECT_EQ(progressBar.progressBar()->value(), 50);
}

TEST(SummaryProgressBar, clear) {
  SummaryProgressBar progressBar;
  QProgressBar *bar1 = new QProgressBar;
  progressBar.AddProgressBar(bar1);

  bar1->setValue(50);
  EXPECT_EQ(progressBar.progressBar()->value(), 50);

  progressBar.clear();
  EXPECT_EQ(progressBar.progressBar()->value(), 0);
}
