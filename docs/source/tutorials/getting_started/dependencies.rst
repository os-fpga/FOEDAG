.. _tutorial_dependencies:

Dependencies
--------------

Full list of dependencies can be found at install_dependencies_build_.
In particular, OpenFPGA requires specific versions for the following dependencies:

:cmake:
  version >3.15 for graphical interface

:gcc:
  version >9 as the project requires full support on C++17 features

:qt:
  version >5 as the project requires Qt5 features

:python dependencies:
  python packages are also required:
  
  python3 -m pip install -r requirements.txt

.. _install_dependencies_build: https://github.com/osfpga/FOEDAG/blob/master/.github/workflows/install_dependencies_build.sh
