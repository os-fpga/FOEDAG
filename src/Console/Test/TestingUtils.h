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

extern "C" {
#include <tcl.h>
}
#include <vector>

namespace FOEDAG::testing {

#define DISABLE_COPY_AND_ASSIGN(className)          \
 private:                                           \
  className() = default;                            \
  className(const className &) = delete;            \
  className(className &&) = delete;                 \
  className &operator=(const className &) = delete; \
  className &operator=(className &&) = delete;

class test {
 public:
  test(const char *name);
  virtual int runTest(void *clientData, Tcl_Interp *interp, int argc,
                      const char *argv[]) = 0;
  const char *name() const;
  static void runAllTests(Tcl_Interp *interp, void *clientData = nullptr);

 public:
  void *clientData;

 protected:
  const char *m_name;
  DISABLE_COPY_AND_ASSIGN(test)
};

class TestRunner {
 public:
  static TestRunner &instance();
  std::vector<test *> registeredTests;

  DISABLE_COPY_AND_ASSIGN(TestRunner)
};

void initTesting();

#define TEST_NAME(name) TEST_##name
#define NAME_TO_STRING(n) #n

#define TCL_TEST(name)                                                         \
  class TEST_NAME(name) : public FOEDAG::testing::test {                       \
   public:                                                                     \
    TEST_NAME(name)() : test(NAME_TO_STRING(name)) {}                          \
    int runTest(void *clientData, Tcl_Interp *interp, int argc,                \
                const char *argv[]) override;                                  \
    static TEST_NAME(name) * instance;                                         \
  };                                                                           \
  TEST_NAME(name) * TEST_NAME(name)::instance = new TEST_NAME(name)();         \
  int TEST_NAME(name)::runTest(void *clientData, Tcl_Interp *interp, int argc, \
                               const char *argv[])

namespace internal {

void INIT_TEST(const char *name, test *testPtr, Tcl_Interp *interpreter,
               void *clientDataPtr);

}  // namespace internal

}  // namespace FOEDAG::testing
