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

#include "TestingUtils.h"

namespace FOEDAG::testing {

test::test(const char *name) : m_name(name) {
  TestRunner::instance().registeredTests.push_back(this);
}

const char *test::name() const { return m_name; }

void test::runAllTests(Tcl_Interp *interp, void *clientData) {
  for (auto test : TestRunner::instance().registeredTests) {
    internal::INIT_TEST(test->name(), test, interp, clientData);
  }
}

TestRunner &TestRunner::instance() {
  static TestRunner runner;
  return runner;
}

void initTesting() { TestRunner::instance(); }

namespace internal {

void INIT_TEST(const char *name, test *testPtr, Tcl_Interp *interpreter,
               void *clientDataPtr) {
  auto lambda = [](void *clientData, Tcl_Interp *interp, int argc,
                   const char *argv[]) -> int {
    test *t = static_cast<test *>(clientData);
    if (t)
      return t->runTest(t->clientData, interp, argc, argv);
    else
      return TCL_ERROR;
  };
  testPtr->clientData = clientDataPtr;
  Tcl_CreateCommand(interpreter, name, lambda,
                    reinterpret_cast<void *>(testPtr), nullptr);
}

}  // namespace internal

}  // namespace FOEDAG::testing
