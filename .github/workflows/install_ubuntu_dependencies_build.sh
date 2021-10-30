# Install required dependencies for Ubuntu systems
sudo apt-get update -qq
sudo apt install -y \
  g++-9 \
  tclsh \
  default-jre \
  cmake \
  build-essential \
  swig \
  google-perftools \
  libgoogle-perftools-dev \
  uuid-dev \
  lcov \
  valgrind \
  xorg \
  qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
  xvfb
sudo ln -sf /usr/bin/g++-9 /usr/bin/g++
sudo ln -sf /usr/bin/gcc-9 /usr/bin/gcc
sudo ln -sf /usr/bin/gcov-9 /usr/bin/gcov
