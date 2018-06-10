#!/bin/bash

if [ "${TRAVIS_OS_NAME}" = linux ] && [ -z "${ANDROID_BUILD}" ]; then
    EXEC="docker exec ${DOCKERSYS}"
fi

if [ "${ANDROID_BUILD}" = 1 ]; then
    echo "Deploy not supported for Android"
elif [ "${TRAVIS_OS_NAME}" = linux ]; then

    DEPLOYSCRIPT=dockerbuild.sh

    cat << EOF > ${DEPLOYSCRIPT}
#!/bin/bash

export PATH="\$PWD/.local/bin:\$PATH"
xvfb-run --auto-servernum python3 ports/deploy/deploy.py
EOF

    chmod +x ${DEPLOYSCRIPT}

    ${EXEC} bash ${DEPLOYSCRIPT}
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    ${EXEC} python3 ports/deploy/deploy.py
fi
