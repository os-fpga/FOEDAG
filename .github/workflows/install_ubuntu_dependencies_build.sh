# Install required dependencies for Ubuntu systems
sudo apt-get update -qq
sudo apt install -y \
  g++-9 \
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
  autotools-dev automake libgtk-3-dev libjudy-dev libbz2-dev liblzma-dev libgconf2-dev libgtk2.0-dev tcl-dev tk-dev gperf gtk2-engines-pixbuf

# For QML: qtdeclarative5-dev

sudo ln -sf /usr/bin/g++-9 /usr/bin/g++
sudo ln -sf /usr/bin/gcc-9 /usr/bin/gcc
sudo ln -sf /usr/bin/gcov-9 /usr/bin/gcov
