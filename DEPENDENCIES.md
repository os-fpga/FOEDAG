#### Ubuntu 18.04

```
   sudo apt-get install qt5-default g++-9 \
      tclsh \
      cmake \
      build-essential \
      swig \
      google-perftools \
      libgoogle-perftools-dev \
      uuid-dev \
      lcov \
      valgrind \
      xorg \
      xvfb
```
  
#### Ubuntu 20.04

```
   sudo apt-get install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools g++-9 \
      tclsh \
      cmake \
      build-essential \
      swig \
      google-perftools \
      libgoogle-perftools-dev \
      uuid-dev \
      lcov \
      valgrind \
      xorg \
      xvfb
```

#### Ubuntu 21.04

```
   sudo apt-get install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools g++-9 \
      tclsh \
      cmake \
      build-essential \
      swig \
      google-perftools \
      libgoogle-perftools-dev \
      uuid-dev \
      lcov \
      valgrind \
      xorg \
      xvfb
```
   
#### AlmaLinux 8.4

.. warning:: *needs further testing*

```
   sudo dnf install qt5-qtbase-devel
```

.. note:: Both "make" and "make test" will pass after this package is installed, however I don't know the difference between this package and the Ubuntu 20.04/21.04 ones yet, and potential differences may bring impacts in the future. Consider support for RHEL-based distros experimental at the moment.
