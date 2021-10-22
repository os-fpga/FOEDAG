
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

#include "Command.h"

using namespace FOEDAG;

Command::Command(const std::string& cmd, const std::string& undo)
    : m_do(cmd), m_undo(undo) {}

const std::string& Command::Command::do_cmd() { return m_do; }

const std::string& Command::Command::undo_cmd() { return m_undo; }

Command::~Command() {}