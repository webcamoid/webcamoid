#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2017  Gonzalo Exequiel Pedone
#
# Webcamoid is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamoid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
#
# Web-Site: http://webcamoid.github.io/

if [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
elif [ "${TRAVIS_OS_NAME}" = linux ] && [ -z "${ANDROID_BUILD}" ]; then
    if [ -z "${DAILY_BUILD}" ]; then
        EXEC="docker exec ${DOCKERSYS}"
    else
        EXEC="docker exec -e DAILY_BUILD=1 ${DOCKERSYS}"
    fi
fi

DEPLOYSCRIPT=deployscript.sh

if [ "${ANDROID_BUILD}" = 1 ]; then
    echo "Deploy not supported for Android"
elif [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind ${PWD} root.x86_64/home/user/webcamoid

    cat << EOF > ${DEPLOYSCRIPT}
#!/bin/sh

export LC_ALL=C
cd /home/user/webcamoid
export PATH="\$PWD/.local/bin:\$PATH"
export WINEPREFIX=\$PWD/.wine
xvfb-run --auto-servernum wineboot -u
xvfb-run --auto-servernum python ports/deploy/deploy.py
EOF
    chmod +x ${DEPLOYSCRIPT}
    sudo cp -vf ${DEPLOYSCRIPT} root.x86_64/home/user/

    ${EXEC} bash /home/user/${DEPLOYSCRIPT}
    sudo umount root.x86_64/home/user/webcamoid
    sudo umount root.x86_64
elif [ "${TRAVIS_OS_NAME}" = linux ]; then
    cat << EOF > ${DEPLOYSCRIPT}
#!/bin/sh

export PATH="\$PWD/.local/bin:\$PATH"
xvfb-run --auto-servernum python3 ports/deploy/deploy.py
EOF

    chmod +x ${DEPLOYSCRIPT}

    ${EXEC} bash ${DEPLOYSCRIPT}
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    ${EXEC} python3 ports/deploy/deploy.py
fi
