# Install required dependencies for CentOS systems
yum update -y
yum group install -y "Development Tools" 
yum install -y epel-release 
curl -C - -O https://cmake.org/files/v3.15/cmake-3.15.7-Linux-x86_64.tar.gz
tar xzf cmake-3.15.7-Linux-x86_64.tar.gz
ln -s $PWD/cmake-3.15.7-Linux-x86_64/bin/cmake /usr/bin/cmake
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
yum install -y xcb-util-image-devel xcb-util-keysyms-devel xcb-util-renderutil-devel xcb-util-wm-devel
yum install -y gtk3-devel zip unzip
yum install -y libusbx-devel libusb-devel
yum install -y pkgconfig
ln -s $PWD/cmake-3.15.7-Linux-x86_64/bin/ctest /usr/bin/ctest
echo 'QMAKE_CC=/opt/rh/devtoolset-11/root/usr/bin/gcc' >> $GITHUB_ENV
echo 'QMAKE_CXX=/opt/rh/devtoolset-11/root/usr/bin/g++' >> $GITHUB_ENV
echo 'PATH=/usr/local/Qt-5.15.4/bin:/usr/lib/ccache:'"$PATH" >> $GITHUB_ENV

if [ -f buildqt5-centos7-gcc.zip ]
then
  echo "Found QT build artifact, untarring..."
  unzip buildqt5-centos7-gcc.zip
  tar xvzf buildqt5-centos7-gcc.tgz
fi

echo "Downloading QT..."
curl -L https://github.com/RapidSilicon/post_build_artifacts/releases/download/v0.1/Qt_5.15.4.tar.gz --output qt-everywhere-src-5.15.4.tar.gz
tar -xzf qt-everywhere-src-5.15.4.tar.gz
mv 5.15.4 /usr/local/Qt-5.15.4
yum clean all
rm -rf qt-everywhere-src-5.15.4.tar.gz

