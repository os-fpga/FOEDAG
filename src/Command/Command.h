/*
 Copyright 2021 The Foedag team

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifndef COMMAND_H
#define COMMAND_H

namespace FOEDAG {

class Command {
 private:
 public:
  Command(const std::string& cmd, const std::string& undo);
  const std::string& do_cmd();
  const std::string& undo_cmd();

  ~Command();

 private:
  std::string m_do;
  std::string m_undo;
};

}

#endif
