#Copyright 2021 The Foedag team

#GPL License

#Copyright (c) 2021 The Open-Source FPGA Foundation

#This program is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.

#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Use bash as the default shell
SHELL := /bin/bash

XVFB = xvfb-run --auto-servernum --server-args="-screen 0, 1280x1024x24"

ifdef $(LC_ALL)
	undefine LC_ALL
endif

CPU_CORES ?= $(shell nproc)
ifeq ($(CPU_CORES),)
	CPU_CORES := $(shell sysctl -n hw.physicalcpu)
endif
ifeq ($(CPU_CORES),)
	CPU_CORES := 2  # Good minimum assumption
endif

PREFIX ?= /usr/local
ADDITIONAL_CMAKE_OPTIONS ?=

# If 'on', then the progress messages are printed. If 'off', makes it easier
# to detect actual warnings and errors  in the build output.
RULE_MESSAGES ?= on

release: run-cmake-release
	cmake --build build -j $(CPU_CORES)

release_no_tcmalloc: run-cmake-release_no_tcmalloc
	cmake --build build -j $(CPU_CORES)

debug: run-cmake-debug
	cmake --build dbuild -j $(CPU_CORES)

run-cmake-release:
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DCMAKE_RULE_MESSAGES=$(RULE_MESSAGES) $(ADDITIONAL_CMAKE_OPTIONS) -S . -B build

run-cmake-release_no_tcmalloc:
	cmake -DNO_TCMALLOC=On -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DCMAKE_RULE_MESSAGES=$(RULE_MESSAGES) $(ADDITIONAL_CMAKE_OPTIONS) -S . -B build

run-cmake-debug:
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DCMAKE_RULE_MESSAGES=$(RULE_MESSAGES) -DNO_TCMALLOC=On $(ADDITIONAL_CMAKE_OPTIONS) -S . -B dbuild

run-cmake-coverage:
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DCMAKE_RULE_MESSAGES=$(RULE_MESSAGES) -DMY_CXX_WARNING_FLAGS="--coverage" $(ADDITIONAL_CMAKE_OPTIONS) -S . -B coverage-build

test/unittest: run-cmake-release
	cmake --build build --target UnitTests -j $(CPU_CORES)
	pushd build && ctest --output-on-failure && popd

test/unittest-d: run-cmake-debug
	cmake --build dbuild --target UnitTests -j $(CPU_CORES)
	pushd dbuild && ctest --output-on-failure && popd

test/unittest-coverage: run-cmake-coverage
	cmake --build coverage-build --target UnitTests -j $(CPU_CORES)
	pushd coverage-build && ctest --output-on-failure && popd

coverage-build/foedag.coverage: test/unittest-coverage
	lcov --no-external --exclude "*_test.cpp" --capture --directory coverage-build/CMakeFiles/foedag.dir --base-directory src --output-file coverage-build/foedag.coverage

coverage-build/html: foedag-build/foedag.coverage
	genhtml --output-directory coverage-build/html $^
	realpath coverage-build/html/index.html

test/regression: run-cmake-release

test/valgrind: run-cmake-debug
	valgrind --tool=memcheck --log-file=valgrind.log dbuild/bin/aurora --batch --script tests/TestBatch/hello.tcl --compiler test; 
	grep "ERROR SUMMARY: 0" valgrind.log
	$(XVFB) valgrind --tool=memcheck --log-file=valgrind_gui.log dbuild/bin/aurora --replay tests/TestGui/gui_start_stop.tcl;
	grep "ERROR SUMMARY: 0" valgrind_gui.log 
	$(XVFB) valgrind --tool=memcheck --log-file=valgrind_gui.log dbuild/bin/newproject --replay tests/TestGui/gui_new_project.tcl
	grep "ERROR SUMMARY: 0" valgrind_gui.log
	$(XVFB) valgrind --tool=memcheck --log-file=valgrind_gui.log dbuild/bin/projnavigator --replay tests/TestGui/gui_project_navigator.tcl
	grep "ERROR SUMMARY: 0" valgrind_gui.log
	$(XVFB) valgrind --tool=memcheck --log-file=valgrind_gui.log dbuild/bin/texteditor --replay tests/TestGui/gui_text_editor.tcl
	grep "ERROR SUMMARY: 0" valgrind_gui.log
	$(XVFB) valgrind --tool=memcheck --log-file=valgrind_gui.log dbuild/bin/newfile --replay tests/TestGui/gui_new_file.tcl
	grep "ERROR SUMMARY: 0" valgrind_gui.log
	$(XVFB) valgrind --tool=memcheck --log-file=valgrind_gui.log dbuild/bin/console_test --replay tests/TestGui/gui_console.tcl
	grep "ERROR SUMMARY: 0" valgrind_gui.log
	$(XVFB) valgrind --tool=memcheck --log-file=valgrind_gui.log dbuild/bin/aurora --replay tests/TestGui/gui_foedag.tcl
	grep "ERROR SUMMARY: 0" valgrind_gui.log
	$(XVFB) valgrind --tool=memcheck --log-file=valgrind_gui.log dbuild/bin/aurora --replay tests/TestGui/gui_task_dlg.tcl
	grep "ERROR SUMMARY: 0" valgrind_gui.log

test: test/unittest test/regression

test-parallel: release test/unittest
	cmake -E make_directory build/tests
	cmake -E remove_directory build/test
	cmake -E make_directory build/test
#	cmake -S build/test -B build/test/build
#	pushd build && cmake --build test/build -j $(CPU_CORES) && popd

test/openfpga: run-cmake-release
	./build/bin/aurora --batch --compiler openfpga --script tests/Testcases/trivial/test.tcl
	./build/bin/aurora --batch --compiler openfpga --verific --script tests/Testcases/trivial/test.tcl
	./build/bin/aurora --batch --compiler openfpga --script tests/Testcases/aes_decrypt_fpga/aes_decrypt.tcl

test/openfpga_gui: run-cmake-release
	./dbuild/bin/aurora --compiler openfpga --script tests/Testcases/aes_decrypt_fpga/aes_decrypt.tcl

regression: release
	cmake -E make_directory build/tests
	cmake -E remove_directory build/test
	cmake -E make_directory build/test
#	cmake -S build/test -B build/test/build
#	pushd build && cmake --build test/build -j $(CPU_CORES) && popd

clean:
	$(RM) -r build dbuild coverage-build dist tests/TestInstall/build

install: release
	cmake --install build

test_install:
	cmake -DCMAKE_BUILD_TYPE=Release -DINSTALL_DIR=$(PREFIX) -S tests/TestInstall -B tests/TestInstall/build
	cmake --build tests/TestInstall/build -j $(CPU_CORES)

test/gui: run-cmake-debug
	$(XVFB) ./dbuild/bin/aurora --script tests/TestGui/compiler_flow.tcl --compiler test
	$(XVFB) ./dbuild/bin/console_test --replay tests/TestGui/gui_console.tcl
	$(XVFB) ./dbuild/bin/console_test --replay tests/TestGui/gui_console_negative_test.tcl && exit 1 || (echo "PASSED: Caught negative test")
	$(XVFB) ./dbuild/bin/aurora --replay tests/TestGui/gui_start_stop.tcl
	$(XVFB) ./dbuild/bin/newproject --replay tests/TestGui/gui_new_project.tcl
	$(XVFB) ./dbuild/bin/projnavigator --replay tests/TestGui/gui_project_navigator.tcl
	$(XVFB) ./dbuild/bin/texteditor --replay tests/TestGui/gui_text_editor.tcl
	$(XVFB) ./dbuild/bin/newfile --replay tests/TestGui/gui_new_file.tcl
	$(XVFB) ./dbuild/bin/aurora --replay tests/TestGui/gui_foedag.tcl
	$(XVFB) ./dbuild/bin/designruns --replay tests/TestGui/design_runs.tcl
	$(XVFB) ./dbuild/bin/aurora --replay tests/TestGui/gui_task_dlg.tcl

test/gui_mac: run-cmake-debug
	$(XVFB) ./dbuild/bin/aurora --replay tests/TestGui/gui_start_stop.tcl
# Tests hanging on mac
#	$(XVFB) ./dbuild/bin/newproject --replay tests/TestGui/gui_new_project.tcl
#	$(XVFB) ./dbuild/bin/projnavigator --replay tests/TestGui/gui_project_navigator.tcl
#	$(XVFB) ./dbuild/bin/texteditor --replay tests/TestGui/gui_text_editor.tcl
#	$(XVFB) ./dbuild/bin/newfile --replay tests/TestGui/gui_new_file.tcl

test/batch: run-cmake-release
	./build/bin/aurora --batch --script tests/Testcases/aes_decrypt_fpga/aes_decrypt.tcl --compiler test
	./build/bin/aurora --batch --script tests/TestGui/compiler_flow.tcl --compiler test
	./build/bin/aurora --batch --script tests/TestBatch/test_compiler_mt.tcl --compiler test
	./build/bin/aurora --batch --script tests/TestBatch/test_compiler_batch.tcl --compiler test

lib-only: run-cmake-release
	cmake --build build --target foedag -j $(CPU_CORES)

format:
	.github/bin/run-clang-format.sh

help:
	build/bin/aurora --help > docs/source/help/help.txt

doc:
	cd docs && make html
	cd -

uninstall:
	$(RM) -r $(PREFIX)/bin/aurora
	$(RM) -r $(PREFIX)/lib/foedag
	$(RM) -r $(PREFIX)/include/foedag
	$(RM) -r $(PREFIX)/share/foedag

