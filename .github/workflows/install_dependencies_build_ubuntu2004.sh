#!/usr/bin/env bash

# The package list is designed for Ubuntu 20.04 LTS
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update
apt-get install -y \
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
