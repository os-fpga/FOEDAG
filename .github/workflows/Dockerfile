FROM centos:7
COPY .github/workflows/install_centos_dependencies_build.sh .
RUN chmod +x install_centos_dependencies_build.sh 
RUN ls -la && bash install_centos_dependencies_build.sh
RUN yum clean all
ENV CC=/opt/rh/devtoolset-11/root/usr/bin/gcc
ENV CXX=/opt/rh/devtoolset-11/root/usr/bin/g++
ENV QMAKE_CC='/opt/rh/devtoolset-11/root/usr/bin/gcc'
ENV QMAKE_CXX='/opt/rh/devtoolset-11/root/usr/bin/g++'
ENV PATH="/opt/python3.8/bin:/usr/local/Qt6.2.4/bin:/usr/lib/ccache:$PATH"
ENV PREFIX=/tmp/foedag-install
ENV ADDITIONAL_CMAKE_OPTIONS='-DMY_CXX_WARNING_FLAGS="-W -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Werror -UNDEBUG"'
ENV RULE_MESSAGES=off
ENV LC_ALL=en_US.utf-8
ENV LANG=en_US.utf-8
ENV PRODUCTION_DEVICES=""
RUN useradd -m -d /home/runner -u 1001 -s /bin/bash -p $(openssl passwd -1 "$pass") runner
USER runner
ENTRYPOINT ["/bin/bash"]
