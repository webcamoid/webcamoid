#!/bin/sh

if [ "${CXX}" = clang++ ]; then
    UNUSEDARGS="-Qunused-arguments"

COMPILER="ccache ${CXX} ${UNUSEDARGS}"

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    qmake Webcamoid.pro \
        QMAKE_CXX="${COMPILER}"
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    LIBUSBVER=$(ls /usr/local/Cellar/libusb | tail -n 1)
    LIBUVCVER=$(ls /usr/local/Cellar/libuvc | tail -n 1)

    qmake Webcamoid.pro \
        QMAKE_CXX="${COMPILER}" \
        LIBUSBINCLUDES=/usr/local/Cellar/libusb/${LIBUSBVER}/include \
        LIBUVCINCLUDES=/usr/local/Cellar/libuvc/${LIBUVCVER}/include \
        LIBUVCLIBS=-L/usr/local/Cellar/libuvc/${LIBUVCVER}/lib \
        LIBUVCLIBS+=-luvc
fi

make
