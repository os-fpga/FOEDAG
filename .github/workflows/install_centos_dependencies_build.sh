# Install required dependencies for CentOS systems
yum update -y
yum group install -y "Development Tools" 
yum remove -y git*
yum install -y https://packages.endpointdev.com/rhel/7/os/x86_64/endpoint-repo.x86_64.rpm
yum install -y git wget
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
yum install -y pkgconfig coreutils
yum install -y perl-IPC-Cmd
yum install -y alsa-lib mesa-dri-drivers openssl openssl-devel sudo
yum install -y python3-devel bzip2-devel libffi-devel
ln -s $PWD/cmake-3.24.4-linux-x86_64/bin/ctest /usr/bin/ctest

# downloads the Qt6 artifact from a specific URL
# saves it as 'buildqt6-centos7-gcc.tar.gz.'
wget https://github.com/RapidSilicon/post_build_artifacts/releases/download/v0.2/qt6.2.4_withopensslWebEngine.tar.gz -O buildqt6-centos7-gcc.tar.gz  

if [ -f buildqt6-centos7-gcc.tar.gz ]
then
  echo "Found QT build artifact, untarring..."
  tar -xvzf buildqt6-centos7-gcc.tar.gz
  mv  Qt6.2.4 /usr/local
else
  echo "Fail to find compiled Qt binaries"
  exit 2
fi

wget https://github.com/os-fpga/post_build_artifacts/releases/download/v0.1/python3.8_static_zlib_8march_2023.tar.gz -O python.tar.gz
tar -xzf python.tar.gz
mv python3.8 /opt
rm /opt/python3.8/bin/python3 && ln -sf /opt/python3.8/bin/python3.8 /opt/python3.8/bin/python3

yum clean all
