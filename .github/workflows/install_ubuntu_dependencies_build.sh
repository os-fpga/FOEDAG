# Install required dependencies for Ubuntu systems
pip3 install gcovr==6.0
sudo apt-get update -qq
sudo apt install -y \
  g++-11 gcc-11 \
  tclsh \
  cmake \
  build-essential \
  google-perftools \
  uuid-dev \
  valgrind \
  xorg \
  qt6-base-dev qt6-webengine-dev qt6-webengine* libegl1-mesa-dev libx11-xcb-dev libxkbcommon-dev \
  xvfb \
  yosys \
  automake \
  libusb-1.0-0-dev \
  pkg-config

wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list http://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
sudo apt update
sudo apt install vulkan-sdk

sudo apt install -y libunwind-dev
sudo apt install -y --no-install-recommends libgoogle-perftools-dev 

# For QML: qtdeclarative5-dev

sudo ln -sf /usr/bin/g++-11 /usr/bin/g++
sudo ln -sf /usr/bin/gcc-11 /usr/bin/gcc
sudo ln -sf /usr/bin/gcov-11 /usr/bin/gcov
