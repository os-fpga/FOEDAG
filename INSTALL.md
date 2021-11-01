### Clone and Initialize Submodules



```
On Ubuntu 18.04:
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
  
On Ubuntu 20.04:
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

On Ubuntu 21.04:
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
   
On AlmaLinux 8.4 (*needs further testing*):
   sudo dnf install qt5-qtbase-devel
   Note: Both "make" and "make test" will pass after this package is installed, however I don't know the difference between this package and the Ubuntu 20.04/21.04 ones yet, and potential differences may bring impacts in the future. Consider support for RHEL-based distros experimental at the moment.
```

```
git clone https://github.com/os-fpga/FOEDAG.git
cd FOEDAG
git submodule update --init --recursive
```

### Compile source codes

```
  make
or
  make debug
or
  make release_no_tcmalloc (For no tcmalloc)
  
make install (/usr/local/bin and /usr/local/lib/foedag by default which requires sudo privilege,
             use PREFIX= for alternative locations.)
```

### Run quick test

```
make test
```

### Build documentation

```
make doc
```
You can view the documentation under the ``docs/build/html`` using a web browser, e.g., Firefox
