#!/bin/bash

if [ "${TRAVIS_OS_NAME}" = linux ] && [ -z "${ANDROID_BUILD}" ]; then
    EXEC="docker exec ${DOCKERSYS}"
fi

BUILDSCRIPT=dockerbuild.sh

if [ "${ANDROID_BUILD}" = 1 ]; then
    echo "Deploy not supported for Android"
elif [ "${TRAVIS_OS_NAME}" = linux ]; then
    ${EXEC} xvfb-run --auto-servernum python3 ports/deploy/deploy.py
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    ${EXEC} python3 ports/deploy/deploy.py
fi
