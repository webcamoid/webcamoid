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

#qtIinstallerVerbose=-v

if [ ! -z "${USE_WGET}" ]; then
    export DOWNLOAD_CMD="wget -nv -c"
else
    export DOWNLOAD_CMD="curl --retry 10 -sS -kLOC -"
fi

if [ "${TRAVIS_OS_NAME}" = linux ] &&
   [ "${ANDROID_BUILD}" != 1 ] &&
   [ -z "${ARCH_ROOT_MINGW}" ] ; then
    # Install missing dependenies
    sudo apt-get -qq -y update
    sudo apt-get -qq -y upgrade
    sudo apt-get -qq -y install \
        libxkbcommon-x11-0

    mkdir -p .local/bin
    qtIFW=QtInstallerFramework-linux-x64.run

    # Install Qt Installer Framework
    ${DOWNLOAD_CMD} http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW} || true

    if [ -e ${qtIFW} ]; then
        chmod +x ${qtIFW}

        QT_QPA_PLATFORM=minimal \
        ./QtInstallerFramework-linux-x64.run \
            ${qtIinstallerVerbose} \
            --script "$PWD/ports/ci/travis/qtifw_non_interactive_install.qs" \
            --no-force-installations

        cd .local
        cp -rvf ~/Qt/QtIFW-${QTIFWVER/-*/}/* .
        cd ..
    fi

    appimage=appimagetool-x86_64.AppImage

    # Install AppImageTool
    wget -c -O .local/${appimage} https://github.com/AppImage/AppImageKit/releases/download/${APPIMAGEVER}/${appimage} || true

    if [ -e .local/${appimage} ]; then
        chmod +x .local/${appimage}

        cd .local
        ./${appimage} --appimage-extract
        cp -rvf squashfs-root/usr/* .
        cd ..
    fi
fi

if [ "${TRAVIS_OS_NAME}" = linux ] && [ "${ANDROID_BUILD}" != 1 ]; then
    if [ "${ARCH_ROOT_BUILD}" = 1 ]; then
        EXEC='sudo ./root.x86_64/bin/arch-chroot root.x86_64'
    else
        EXEC="docker exec ${DOCKERSYS}"
    fi
fi

if [ "${ANDROID_BUILD}" = 1 ]; then
    sudo apt-get -qq -y update
    sudo apt-get -qq -y upgrade

    # Install dev tools
    sudo apt-get -qq -y install \
        cmake \
        libxkbcommon-x11-0 \
        make \
        openjdk-8-jdk \
        openjdk-8-jre

    sudo update-java-alternatives --set java-1.8.0-openjdk-amd64
    mkdir -p build
    cd build

    # Install Android SDK
    fileName="commandlinetools-linux-${SDKVER}_latest.zip"
    ${DOWNLOAD_CMD} "https://dl.google.com/android/repository/${fileName}"

    mkdir -p android-sdk
    unzip -q -d android-sdk ${fileName}

    # Install Android NDK
    fileName="android-ndk-${NDKVER}-linux-x86_64.zip"
    ${DOWNLOAD_CMD} "https://dl.google.com/android/repository/${fileName}"
    unzip -q ${fileName}
    mv -vf android-ndk-${NDKVER} android-ndk

    # Install Qt for Android
    fileName=qt-opensource-linux-x64-${QTVER_ANDROID}.run
    ${DOWNLOAD_CMD} "https://download.qt.io/archive/qt/${QTVER_ANDROID:0:4}/${QTVER_ANDROID}/${fileName}"
    chmod +x ${fileName}

    # Shutdown network connection so Qt installer does not ask for credentials.
    netName=$(ifconfig -s | grep BMRU | awk '{print $1}' | sed 's/.*://g')
    sudo ifconfig ${netName} down

    export QT_QPA_PLATFORM=minimal

    ./qt-opensource-linux-x64-${QTVER_ANDROID}.run \
        ${qtIinstallerVerbose} \
        --script "$PWD/../ports/ci/travis/qt_non_interactive_install.qs" \
        --no-force-installations

    # Get network connection up again.
    sudo ifconfig ${netName} up

    cd ..

    # Set environment variables for Android build
    export JAVA_HOME=$(readlink -f /usr/bin/java | sed 's:bin/java::')
    export ANDROID_HOME="${PWD}/build/android-sdk"
    export ANDROID_NDK="${PWD}/build/android-ndk"
    export ANDROID_NDK_HOME=${ANDROID_NDK}
    export PATH="${JAVA_HOME}/bin/java:${PATH}"
    export PATH="${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/tools/bin"
    export PATH="${PATH}:${ANDROID_HOME}/platform-tools"
    export PATH="${PATH}:${ANDROID_HOME}/emulator"
    export PATH="${PATH}:${ANDROID_NDK}"

    # Install Android things
    echo y | sdkmanager \
        --sdk_root=${ANDROID_HOME} \
        "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" \
        "platform-tools" \
        "platforms;android-${ANDROID_PLATFORM}" \
        "tools" > /dev/null
elif [ "${ARCH_ROOT_BUILD}" = 1 ]; then
    # Download chroot image
    archImage=archlinux-bootstrap-${ARCH_ROOT_DATE}-x86_64.tar.gz
    ${DOWNLOAD_CMD} ${ARCH_ROOT_URL}/iso/${ARCH_ROOT_DATE}/$archImage
    sudo tar xzf $archImage

    # Configure mirrors
    cp -vf root.x86_64/etc/pacman.conf .
    cat << EOF >> pacman.conf

[multilib]
Include = /etc/pacman.d/mirrorlist

[ownstuff]
Server = https://ftp.f3l.de/~martchus/\$repo/os/\$arch
Server = http://martchus.no-ip.biz/repo/arch/\$repo/os/\$arch
EOF
    sed -i 's/Required DatabaseOptional/Never/g' pacman.conf
    sed -i 's/#TotalDownload/TotalDownload/g' pacman.conf
    sudo cp -vf pacman.conf root.x86_64/etc/pacman.conf

    cp -vf root.x86_64/etc/pacman.d/mirrorlist .
    cat << EOF >> mirrorlist

Server = ${ARCH_ROOT_URL}/\$repo/os/\$arch
EOF
    sudo cp -vf mirrorlist root.x86_64/etc/pacman.d/mirrorlist

    # Install packages
    sudo mkdir -pv root.x86_64/$HOME
    sudo mount --bind root.x86_64 root.x86_64
    sudo mount --bind $HOME root.x86_64/$HOME

    ${EXEC} pacman-key --init
    ${EXEC} pacman-key --populate archlinux
    ${EXEC} pacman -Syu \
        --noconfirm \
        --ignore linux,linux-api-headers,linux-docs,linux-firmware,linux-headers,pacman

    ${EXEC} pacman --noconfirm --needed -S \
        ccache \
        clang \
        cmake \
        file \
        git \
        make \
        pkgconf \
        python \
        sed \
        xorg-server-xvfb

    if [ -z "${ARCH_ROOT_MINGW}" ]; then
        ${EXEC} pacman --noconfirm --needed -S \
            qt5-quickcontrols2 \
            qt5-svg \
            v4l-utils \
            qt5-tools \
            ffmpeg \
            gst-plugins-base-libs \
            libpulse \
            alsa-lib \
            jack
    else
        ${EXEC} pacman --noconfirm --needed -S \
            gst-plugins-base-libs \
            mpg123 \
            lib32-gst-plugins-base-libs \
            lib32-mpg123 \
            wine \
            mingw-w64-pkg-config \
            mingw-w64-cmake \
            mingw-w64-gcc \
            mingw-w64-qt5-quickcontrols2 \
            mingw-w64-qt5-svg \
            mingw-w64-qt5-tools \
            mingw-w64-ffmpeg

        qtIFW=QtInstallerFramework-win-x86.exe

        # Install Qt Installer Framework
        ${DOWNLOAD_CMD} http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW} || true

        if [ -e ${qtIFW} ]; then
            INSTALLSCRIPT=installscript.sh

            cat << EOF > ${INSTALLSCRIPT}
#!/bin/sh

export LC_ALL=C
export HOME=$HOME
export WINEPREFIX=/opt/.wine
cd $TRAVIS_BUILD_DIR

wine ./${qtIFW} \
    ${qtIinstallerVerbose} \
    --script "ports/ci/travis/qtifw_non_interactive_install.qs" \
    --no-force-installations
EOF

            chmod +x ${INSTALLSCRIPT}
            sudo cp -vf ${INSTALLSCRIPT} root.x86_64/$HOME/
            ${EXEC} bash $HOME/${INSTALLSCRIPT}
        fi
    fi

    # Finish
    sudo umount root.x86_64/$HOME
    sudo umount root.x86_64
elif [ "${DOCKERSYS}" = debian ]; then
    ${EXEC} apt-get -y update
    ${EXEC} apt-get -y upgrade

    # Install dev tools
    ${EXEC} bash -c "DEBIAN_FRONTEND=noninteractive apt-get -y install \
        ccache \
        clang \
        cmake \
        g++ \
        git \
        libasound2-dev \
        libavcodec-dev \
        libavdevice-dev \
        libavformat-dev \
        libavresample-dev \
        libavutil-dev \
        libgl1-mesa-dev \
        libjack-dev \
        libpulse-dev \
        libswresample-dev \
        libswscale-dev \
        libv4l-dev \
        linux-libc-dev \
        make \
        pkg-config \
        xvfb"

    if [ -z "${DAILY_BUILD}" ] && [ -z "${RELEASE_BUILD}" ]; then
        ${EXEC} apt-get -y install \
            libgstreamer-plugins-base1.0-dev \
            libusb-dev \
            libuvc-dev
    fi

    # Install Qt dev
    ${EXEC} apt-get -y install \
        qt5-qmake \
        qttools5-dev-tools \
        qtdeclarative5-dev \
        libqt5opengl5-dev \
        libqt5svg5-dev \
        qtquickcontrols2-5-dev \
        qml-module-qt-labs-folderlistmodel \
        qml-module-qt-labs-settings \
        qml-module-qtqml-models2 \
        qml-module-qtquick-controls2 \
        qml-module-qtquick-dialogs \
        qml-module-qtquick-extras \
        qml-module-qtquick-privatewidgets \
        qml-module-qtquick-templates2
elif [ "${DOCKERSYS}" = fedora ]; then
    ${EXEC} dnf install -y --skip-broken https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-${FEDORAVER}.noarch.rpm
    ${EXEC} dnf install -y --skip-broken https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-${FEDORAVER}.noarch.rpm
    ${EXEC} dnf -y update

    ${EXEC} dnf -y install \
        alsa-lib-devel \
        ccache \
        clang \
        cmake \
        ffmpeg-devel \
        file \
        gcc-c++ \
        git \
        gstreamer1-plugins-base-devel \
        jack-audio-connection-kit-devel \
        libv4l-devel \
        make \
        pulseaudio-libs-devel \
        qt5-linguist \
        qt5-qtdeclarative-devel \
        qt5-qtquickcontrols2-devel \
        qt5-qtsvg-devel \
        qt5-qttools-devel \
        which \
        xorg-x11-server-Xvfb \
        xorg-x11-xauth
elif [ "${DOCKERSYS}" = opensuse ]; then
    ${EXEC} zypper -n dup
    ${EXEC} zypper -n in \
        alsa-devel \
        ccache \
        clang \
        cmake \
        ffmpeg-devel \
        git \
        gstreamer-plugins-base-devel \
        gzip \
        libjack-devel \
        libQt5QuickControls2-devel \
        libpulse-devel \
        libqt5-linguist \
        libqt5-qtbase-devel \
        libqt5-qtdeclarative-devel \
        libqt5-qtquickcontrols2 \
        libqt5-qtsvg-devel \
        libv4l-devel \
        python3 \
        which \
        xauth \
        xvfb-run
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    brew update
    brew upgrade
    brew link --overwrite numpy
    brew install \
        ccache \
        cmake \
        ffmpeg \
        p7zip \
        pkg-config \
        python \
        qt@5
    brew link --overwrite python
    brew link --force qt@5

    if [ -z "${DAILY_BUILD}" ] && [ -z "${RELEASE_BUILD}" ]; then
        brew install \
            gstreamer \
            gst-plugins-base \
            pulseaudio \
            jack \
            libuvc
    fi

    brew link python
    qtIFW=QtInstallerFramework-mac-x64.dmg

    # Install Qt Installer Framework
    ${DOWNLOAD_CMD} http://download.qt.io/official_releases/qt-installer-framework/${QTIFWVER}/${qtIFW} || true

    if [ -e "${qtIFW}" ]; then
        hdiutil convert ${qtIFW} -format UDZO -o qtifw
        7z x -oqtifw qtifw.dmg -bb
        7z x -oqtifw qtifw/5.hfsx -bb
        chmod +x qtifw/QtInstallerFramework-mac-x64/QtInstallerFramework-mac-x64.app/Contents/MacOS/QtInstallerFramework-mac-x64

        qtifw/QtInstallerFramework-mac-x64/QtInstallerFramework-mac-x64.app/Contents/MacOS/QtInstallerFramework-mac-x64 \
            ${qtIinstallerVerbose} \
            --script "$PWD/ports/ci/travis/qtifw_non_interactive_install.qs" \
            --no-force-installations
    fi
fi
