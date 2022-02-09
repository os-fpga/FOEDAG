/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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
#pragma once
#include "ConsoleTestUtils.h"

namespace FOEDAG {

namespace testing {

class test {
 public:
  test();
  test(const test &copy) = delete;
  virtual int testing(void *clientData, Tcl_Interp *interp, int argc,
                      const char *argv[]) = 0;
  QString name() const;
  static void initTests();

 protected:
  QString m_name;
  static std::vector<test *> registeredTests;
};

#define TEST_NAME(name) TEST_##name
#define NAME_TO_STRING(n) #n

#define TCL_TEST(name)                                                         \
  class TEST_NAME(name) : public FOEDAG::testing::test {                       \
   public:                                                                     \
    TEST_NAME(name)() : test() { m_name = NAME_TO_STRING(name); }              \
    int testing(void *clientData, Tcl_Interp *interp, int argc,                \
                const char *argv[]) override;                                  \
    static TEST_NAME(name) * instance;                                         \
  };                                                                           \
  TEST_NAME(name) * TEST_NAME(name)::instance = new TEST_NAME(name)();         \
  int TEST_NAME(name)::testing(void *clientData, Tcl_Interp *interp, int argc, \
                               const char *argv[])

#define INIT_TEST(name, testPtr)                                   \
  auto lambda = [](void *clientData, Tcl_Interp *interp, int argc, \
                   const char *argv[]) -> int {                    \
    FOEDAG::testing::test *t =                                     \
        static_cast<FOEDAG::testing::test *>(clientData);          \
    return t->testing(clientData, interp, argc, argv);             \
  };                                                               \
  session->TclInterp()->registerCmd(name, lambda, testPtr, nullptr);

}  // namespace testing

}  // namespace FOEDAG
