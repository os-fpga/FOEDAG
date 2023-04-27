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
yum install -y xcb-util-image-devel xcb-util-keysyms-devel xcb-util-renderutil-devel xcb-util-wm-devel
yum install -y gtk3-devel zip unzip
ln -s $PWD/cmake-3.24.4-linux-x86_64/bin/ctest /usr/bin/ctest
echo 'QMAKE_CC=/opt/rh/devtoolset-11/root/usr/bin/gcc' >> $GITHUB_ENV
echo 'QMAKE_CXX=/opt/rh/devtoolset-11/root/usr/bin/g++' >> $GITHUB_ENV
echo 'PATH=/usr/local/Qt-6.5.0/bin:/usr/lib/ccache:'"$PATH" >> $GITHUB_ENV

if [ -f buildqt6-centos7-gcc.zip ]
then
  echo "Found QT build artifact, untarring..."
  unzip buildqt6-centos7-gcc.zip
  tar xvzf buildqt6-centos7-gcc.tgz
fi

echo "Downloading QT..."
curl -L https://download.qt.io/official_releases/qt/6.5/6.5.0/single/qt-everywhere-src-6.5.0.tar.xz --output qt-everywhere-src-6.5.0.tar.xz
tar -xf qt-everywhere-src-6.5.0.tar.xz

if [ -d "buildqt6" ] 
then
  echo "Installing QT..."
  cd buildqt6  
  make install
  cd ..
else
  echo "Building QT..."
  # work around to make it compile on GCC 11. For reference, see source (https://forum.qt.io/topic/139626/unable-to-build-static-version-of-qt-5-15-2/13)
  #sed -i '44i\#include <limits>' qt-everywhere-src-6.5.0/qtbase/src/corelib/text/qbytearraymatcher.h
  #sed -i '52i\#include <limits>' qt-everywhere-src-6.5.0/qtdeclarative/src/qmldebug/qqmlprofilerevent_p.h
  #cat qt-everywhere-src-6.5.0/qtbase/src/corelib/text/qbytearraymatcher.h
  mkdir buildqt6
  cd buildqt6
  source /opt/rh/devtoolset-11/enable
  ../qt-everywhere-src-6.5.0/configure -opensource -confirm-license -xcb -xcb-xlib -bundled-xcb-xinput -nomake examples
  make -j 2
  echo "Installing QT..."
  make install
  cd ..
fi
