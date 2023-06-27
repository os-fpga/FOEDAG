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

#include "Utils/FileUtils.h"

#include <fstream>

#include "gtest/gtest.h"

namespace fs = std::filesystem;
using namespace FOEDAG;

TEST(FileUtils, WriteToFileNewLine) {
  fs::path file{"test.txt"};
  std::string content{"some content"};
  FileUtils::WriteToFile(file, content, true);

  std::ifstream ifs{file.string()};
  EXPECT_EQ(ifs.good(), true);

  std::string readContent((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
  EXPECT_EQ(content + "\n", readContent);
}

TEST(FileUtils, WriteToFileNoNewLine) {
  fs::path file{"test.txt"};
  std::string content{"some content"};
  FileUtils::WriteToFile(file, content, false);

  std::ifstream ifs{file.string()};
  EXPECT_EQ(ifs.good(), true);

  std::string readContent((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
  EXPECT_EQ(content, readContent);
}

TEST(FileUtils, removeAll) {
  fs::path testFolder{"testFolder"};
  FileUtils::MkDirs(testFolder);
  FileUtils::WriteToFile(testFolder / "test1.txt", "content");
  FileUtils::WriteToFile(testFolder / "test2.txt", "content");
  int fileCount{0};
  ASSERT_NO_THROW({
    for (auto const& entry : fs::directory_iterator{testFolder}) fileCount++;
  });
  EXPECT_EQ(fileCount, 2);

  FileUtils::removeAll(testFolder);

  fileCount = 0;
  ASSERT_NO_THROW(
      {  // directory_iterator will throw exception if path does not exists
        for (auto const& entry : fs::directory_iterator{testFolder})
          fileCount++;
      });
  EXPECT_EQ(fileCount, 0);
}
