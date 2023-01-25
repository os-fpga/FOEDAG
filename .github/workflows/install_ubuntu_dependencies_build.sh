# Install required dependencies for Ubuntu systems
sudo apt-get update -qq
sudo apt install -y \
  g++-11 gcc-11 \
  tclsh \
  cmake \
  build-essential \
  google-perftools \
  libgoogle-perftools-dev \
  uuid-dev \
  lcov \
  valgrind \
  xorg \
  qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
  xvfb \
  yosys \
  automake

# For QML: qtdeclarative5-dev

sudo ln -sf /usr/bin/g++-11 /usr/bin/g++
sudo ln -sf /usr/bin/gcc-11 /usr/bin/gcc
sudo ln -sf /usr/bin/gcov-11 /usr/bin/gcov
