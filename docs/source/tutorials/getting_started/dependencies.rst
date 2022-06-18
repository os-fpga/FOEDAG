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
======

Minimal requirements:

* Microsoft Visual Studio сommunity edition
* Qt5 for MSVC
* Make sure component 'C++ CMake tools for windows' is installed for Microsoft Visual Studio
* Make sure Qt bin are in the PATH variable. e.g. set PATH=C:\Qt\5.15.2\msvc2019_64\bin;%PATH%
* Make sure Qt5_DIR is set. e.g. C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5
