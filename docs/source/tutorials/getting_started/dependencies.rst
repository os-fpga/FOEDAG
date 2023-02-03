.. _tutorial_dependencies:

Dependencies
------------

In general, OpenFPGA requires specific versions for the following dependencies:

:cmake:
  version >3.15 for graphical interface

:gcc:
  version >9 as the project requires full support on C++17 features

:qt:
  version >5 as the project requires Qt5 features

:python dependencies:
  python packages are also required:
  
  python3 -m pip install -r requirements.txt


Ubuntu
======

Full list of dependencies can be found at install_ubuntu_dependencies_build_.

.. include:: ../../../../.github/workflows/install_ubuntu_dependencies_build.sh
  :code: shell

.. _install_ubuntu_dependencies_build: ../../../../../.github/workflows/install_ubuntu_dependencies_build.sh

.. note:: Different Ubuntu version may require different package names. See details in the sub subsections

.. mdinclude:: ../../../../DEPENDENCIES.md

Mac OS
======

Full list of dependencies can be found at install_macos_dependencies_build_.

.. include:: ../../../../.github/workflows/install_macos_dependencies_build.sh
  :code: shell

.. _install_ubuntu_dependencies_build: ../../../../../.github/workflows/install_macos_dependencies_build.sh

WIN
===

MSVC
~~~~

Minimal requirements:

* Microsoft Visual Studio сommunity edition
* Qt5 for MSVC
* Make sure component 'C++ CMake tools for windows' is installed for Microsoft Visual Studio
* Make sure Qt bin are in the PATH variable. e.g. set PATH=C:\Qt\5.15.2\msvc2019_64\bin;%PATH%
* Make sure Qt5_DIR is set. e.g. C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5

MSYS2 MinGW64
~~~~~~~~~~~~~

MSYS2 env with the MinGW64 g++ compiler can be used to build FOEDAG.

* Get the latest installer from : https://www.msys2.org/
* Follow the steps on the main site : https://www.msys2.org/ (also listed below, step 1-6), 
  and **step 7** lists the additional packages needed to build FOEDAG:
  
  1. Invoke the downloaded installer
  
  2. Allow installer to run the **MSYS2 MSYS** Shell
  
  3. Run :code:`pacman -Syu` for initial base updates
  
  4. At the end, it will close the terminal after confirmation
  
  5. Run **MSYS2 MSYS** Shell
  
  6. Run :code:`pacman -Su` for remaining base updates
  
  7. Run :code:`pacman -S --needed base-devel mingw-w64-x86_64-toolchain git mingw-w64-x86_64-cmake mingw-w64-x86_64-qt5-base-debug mingw-w64-x86_64-qt5 mingw-w64-x86_64-qt5-declarative-debug mingw-w64-x86_64-tcl mingw-w64-x86_64-zlib`
     for installing required packages.
     
     Select default (all) packages to install here
  
  8. Close the **MSYS2 MSYS** Shell

* Now, use **MSYS2 MinGW x64** Shell (from Start Menu) to open the right shell and start building.
* | If the system has MSVC compiler setup with Qt5 installed, it is likely that :code:`Qt5_DIR` env variable is set.  
  | If so, the MSYS2 build will pick up the MSVC Qt5 install, and linking will fail due to the difference in name mangling.  
  | In this case, ensure that :code:`Qt5_DIR` is set to the MinGW64 Qt5 packages before building.  
  
  .. code-block::

      export Qt5_DIR=/mingw64/lib/cmake/Qt5

