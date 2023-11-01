# Install required dependencies for CentOS systems
yum update -y
yum group install -y "Development Tools" 
yum install -y epel-release 
curl -C - -O https://cmake.org/files/v3.24/cmake-3.24.4-linux-x86_64.tar.gz
tar xzf cmake-3.24.4-linux-x86_64.tar.gz
ln -s $PWD/cmake-3.24.4-linux-x86_64/bin/cmake /usr/bin/cmake
yum install -y centos-release-scl-rh
yum install -y devtoolset-11
yum install -y devtoolset-11-toolchain
yum install -y devtoolset-11-gcc-c++
scl enable devtoolset-11 bash
yum install -y tcl
yum install -y make
yum install -y which
yum install -y google-perftools
yum install -y gperftools gperftools-devel
yum install -y uuid-devel
yum install -y valgrind
yum install -y python3
yum install -y xorg-x11-server-Xorg xorg-x11-xauth xorg-x11-apps 
yum install -y xorg-x11-server-Xvfb
yum install -y mesa-libGL-devel
yum install -y libxcb libxcb-devel xcb-util xcb-util-devel libxkbcommon-devel libxkbcommon-x11-devel
yum install -y xcb-util-image-devel xcb-util-keysyms-devel xcb-util-renderutil-devel xcb-util-wm-devel compat-libxcb compat-libxcb-devel xcb-util-cursor xcb-util-cursor-devel
yum install -y gtk3-devel zip unzip
yum install -y libusbx-devel libusb-devel
yum install -y pkgconfig
ln -s $PWD/cmake-3.24.4-linux-x86_64/bin/ctest /usr/bin/ctest
echo 'QMAKE_CC=/opt/rh/devtoolset-11/root/usr/bin/gcc' >> $GITHUB_ENV
echo 'QMAKE_CXX=/opt/rh/devtoolset-11/root/usr/bin/g++' >> $GITHUB_ENV
echo 'PATH=/usr/local/Qt-6.5.1/bin:/usr/lib/ccache:'"$PATH" >> $GITHUB_ENV

if [ -f buildqt6-centos7-gcc.tar.gz ]
then
  echo "Found QT build artifact, untarring..."
  tar -xvzf buildqt6-centos7-gcc.tar.gz
  mv  Qt-6.5.1 /usr/local
else
  echo "Fail to find compiled Qt binaries"
  exit 2
fi

yum clean all
