#!/bin/bash
set -e  # halt on first error
BUILD_PATH="$HOME/awesomo_build"


install_dependencies()
{
    # install dependencies
    sudo apt-get install -y \
        subversion \
        cmake \
        libopencv-dev \
        libeigen3-dev \
        libv4l-dev
}

install_apriltags()
{
    # create build directory for awesomo
    mkdir -p $BUILD_PATH

    cd $BUILD_PATH
    svn --trust-server-cert --non-interactive co https://svn.csail.mit.edu/apriltags
    cd apriltags
    export BUILD_PREFIX=$BUILD_PATH/apriltags/build
    echo $BUILD_PREFIX
    mkdir -p $BUILD_PREFIX
    make
    cp -r ./build/include/AprilTags /usr/include/
    cp -r ./build/lib/libapriltags.a /usr/lib/

    # remove apriltags repo
    # cd $BUILD_PATH
    # rm -rf apriltags
}


# RUN
install_dependencies
install_apriltags
