#!/bin/sh

if [ -z "${DISABLE_CCACHE}" ]; then
    if [ "${CXX}" = clang++ ]; then
        UNUSEDARGS="-Qunused-arguments"
    fi

    COMPILER="ccache ${CXX} ${UNUSEDARGS}"
else
    COMPILER=${CXX}
fi

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    EXEC="docker exec ${DOCKERSYS}"
fi

if [ "${DOCKERIMG}" = ubuntu:precise ]; then
#    ${EXEC} source /opt/qt56/bin/qt56-env.sh
    QT_BASE_DIR=/opt/qt56
    dockPath=$(${EXEC} bash -c 'echo "$PATH"')
    docker run --env QTDIR="$QT_BASE_DIR" ${DOCKERIMG} bash
    docker run --env PATH="$QT_BASE_DIR/bin:$dockPath" ${DOCKERIMG} bash
elif [ "${DOCKERIMG}" = ubuntu:trusty ] || \
     [ "${DOCKERIMG}" = ubuntu:xenial ]; then
#    ${EXEC} source /opt/qt58/bin/qt58-env.sh
    QT_BASE_DIR=/opt/qt58
    dockPath=$(${EXEC} bash -c 'echo "$PATH"')
    docker run --env QTDIR="$QT_BASE_DIR" ${DOCKERIMG} bash
    docker run --env PATH="$QT_BASE_DIR/bin:$dockPath" ${DOCKERIMG} bash
fi

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    if [ "${DOCKERSYS}" = debian ]; then
        ${EXEC} qmake -qt=5 -spec ${COMPILESPEC} Webcamoid.pro \
            QMAKE_CXX="${COMPILER}"
    else
        ${EXEC} qmake-qt5 -spec ${COMPILESPEC} Webcamoid.pro \
            QMAKE_CXX="${COMPILER}"
    fi
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    ${EXEC} qmake -spec ${COMPILESPEC} Webcamoid.pro \
        QMAKE_CXX="${COMPILER}" \
        LIBUSBINCLUDES=/usr/local/opt/libusb/include \
        LIBUVCINCLUDES=/usr/local/opt/libuvc/include \
        LIBUVCLIBS=-L/usr/local/opt/libuvc/lib \
        LIBUVCLIBS+=-luvc

fi

if [ -z "${NJOBS}" ]; then
    NJOBS=4
fi

${EXEC} make -j${NJOBS}
