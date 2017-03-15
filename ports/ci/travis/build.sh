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

BUILDSCRIPT=dockerbuild.sh

if [ "${DOCKERIMG}" = ubuntu:precise ]; then
    cat << EOF > ${BUILDSCRIPT}
#!/bin/bash

source /opt/qt56/bin/qt56-env.sh

EOF

    chmod +x ${BUILDSCRIPT}
elif [ "${DOCKERIMG}" = ubuntu:trusty ] || \
     [ "${DOCKERIMG}" = ubuntu:xenial ]; then
    cat << EOF > ${BUILDSCRIPT}
#!/bin/bash

source /opt/qt58/bin/qt58-env.sh

EOF

    chmod +x ${BUILDSCRIPT}
fi


if [ "${DOCKERIMG}" = ubuntu:precise ]; then
    ${EXEC} ln -sf /usr/bin/g++-4.9 /usr/bin/g++
    ${EXEC} ln -sf /usr/bin/clang++-3.6 /usr/bin/clang++
fi

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    if [ "${DOCKERSYS}" = debian ]; then
        if [ "${DOCKERIMG}" = ubuntu:precise ] || \
           [ "${DOCKERIMG}" = ubuntu:trusty ] || \
           [ "${DOCKERIMG}" = ubuntu:xenial ]; then
           cat << EOF >> ${BUILDSCRIPT}
qmake -spec ${COMPILESPEC} Webcamoid.pro \
    QMAKE_CXX="${COMPILER}"
EOF
            ${EXEC} bash ${BUILDSCRIPT}
        else
            ${EXEC} qmake -qt=5 -spec ${COMPILESPEC} Webcamoid.pro \
                QMAKE_CXX="${COMPILER}"
        fi
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
