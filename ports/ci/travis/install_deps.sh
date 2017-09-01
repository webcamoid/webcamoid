#!/bin/bash

if [ "${TRAVIS_OS_NAME}" = linux ]; then
    EXEC="docker exec ${DOCKERSYS}"
fi

if [ "${ANDROID_BUILD}" = 1 ]; then
    sudo apt-get -y install make

    mkdir -p build
    cd build

    # Install Android NDK
    wget -c https://dl.google.com/android/repository/android-ndk-${NDKVER}-linux-x86_64.zip
    unzip -q android-ndk-${NDKVER}-linux-x86_64.zip

    # Install Qt for Android
    cat << EOF > non_interactive_install.qs
function Controller() {
    installer.autoRejectMessageBoxes();
    installer.setMessageBoxAutomaticAnswer("OverwriteTargetDirectory", QMessageBox.Yes);
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.NextButton);
    })
}

Controller.prototype.WelcomePageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.CredentialsPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.IntroductionPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.TargetDirectoryPageCallback = function()
{
    //gui.currentPageWidget().TargetDirectoryLineEdit.setText(installer.value("HomeDir") + "/Qt");
    gui.currentPageWidget().TargetDirectoryLineEdit.setText(installer.value("InstallerDirPath") + "/Qt");
    //gui.currentPageWidget().TargetDirectoryLineEdit.setText("/scratch/Qt");
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function() {
    var widget = gui.currentPageWidget();

    widget.deselectAll();
    widget.selectComponent("qt.58.android_armv7");
    widget.selectComponent("qt.58.android_x86");

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function() {
    gui.currentPageWidget().AcceptLicenseRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.StartMenuDirectoryPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function() {
var checkBoxForm = gui.currentPageWidget().LaunchQtCreatorCheckBoxForm
if (checkBoxForm && checkBoxForm.launchQtCreatorCheckBox) {
    checkBoxForm.launchQtCreatorCheckBox.checked = false;
}
    gui.clickButton(buttons.FinishButton);
}
EOF

    wget -c https://download.qt.io/archive/qt/${QTVER:0:3}/${QTVER}/qt-opensource-linux-x64-android-${QTVER}.run
    chmod +x qt-opensource-linux-x64-android-${QTVER}.run
    ./qt-opensource-linux-x64-android-${QTVER}.run \
        -platform minimal \
        --script non_interactive_install.qs \
        --no-force-installations
elif [ "${DOCKERSYS}" = debian ]; then
    ${EXEC} apt-get -y update

    if [ "${DOCKERIMG}" = ubuntu:trusty ]; then
        ${EXEC} apt-get -y install software-properties-common
        ${EXEC} add-apt-repository ppa:beineri/opt-qt58-trusty
    elif [ "${DOCKERIMG}" = ubuntu:xenial ]; then
        ${EXEC} apt-get -y install software-properties-common
        ${EXEC} add-apt-repository ppa:beineri/opt-qt58-xenial
    fi

    ${EXEC} apt-get -y update
    ${EXEC} apt-get -y upgrade

    # Install dev tools
    ${EXEC} apt-get -y install \
        xvfb \
        g++ \
        clang \
        ccache \
        make \
        pkg-config \
        linux-libc-dev \
        libgl1-mesa-dev \
        libpulse-dev \
        libjack-dev \
        libasound2-dev \
        libv4l-dev \
        libgstreamer-plugins-base1.0-dev

    # Install Qt dev
    if [ "${DOCKERIMG}" = ubuntu:trusty ] || \
         [ "${DOCKERIMG}" = ubuntu:xenial ]; then
        ${EXEC} apt-get -y install \
            qt58tools \
            qt58declarative \
            qt58multimedia \
            qt58svg
    else
        ${EXEC} apt-get -y install \
            qt5-qmake \
            qtdeclarative5-dev \
            qtmultimedia5-dev \
            libqt5opengl5-dev \
            libqt5svg5-dev
    fi

    # Install FFmpeg dev
    ${EXEC} apt-get -y install \
        libavcodec-dev \
        libavdevice-dev \
        libavformat-dev \
        libavutil-dev \
        libavresample-dev \
        libswscale-dev

    if [ "${DOCKERIMG}" != ubuntu:trusty ]; then
        ${EXEC} apt-get -y install \
            libswresample-dev
    fi
elif [ "${DOCKERSYS}" = fedora ]; then
    ${EXEC} dnf install -y https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-${FEDORAVER}.noarch.rpm
    ${EXEC} dnf install -y https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-${FEDORAVER}.noarch.rpm
    ${EXEC} dnf -y update

    ${EXEC} dnf -y install \
        file \
        which \
        xorg-x11-xauth \
        xorg-x11-server-Xvfb \
        ccache \
        clang \
        make \
        gcc-c++ \
        qt5-qttools-devel \
        qt5-qtdeclarative-devel \
        qt5-qtmultimedia-devel \
        qt5-qtsvg-devel \
        ffmpeg-devel \
        gstreamer1-plugins-base-devel \
        libv4l-devel \
        alsa-lib-devel \
        pulseaudio-libs-devel \
        jack-audio-connection-kit-devel
elif [ "${DOCKERSYS}" = opensuse ]; then
    ${EXEC} zypper -n update

    ${EXEC} zypper -n in \
        which \
        xauth \
        xvfb-run \
        python3 \
        ccache \
        clang \
        libqt5-linguist \
        libqt5-qtbase-devel \
        libqt5-qtdeclarative-devel \
        libqt5-qtmultimedia-devel \
        libqt5-qtsvg-devel \
        ffmpeg-devel \
        gstreamer-plugins-base-devel \
        libv4l-devel \
        alsa-devel \
        libpulse-devel \
        libjack-devel
elif [ "${TRAVIS_OS_NAME}" = osx ]; then
    wget -c https://github.com/Syphon/Simple/releases/download/version-3/Syphon.Simple.Apps.3.zip
    unzip Syphon.Simple.Apps.3.zip
    mkdir -p Syphon
    cp -Rvf "Syphon Simple Apps/Simple Client.app/Contents/Frameworks/Syphon.framework" ./Syphon

    brew install \
        python3 \
        ccache \
        pkg-config \
        qt5 \
        ffmpeg \
        gstreamer \
        gst-plugins-base \
        pulseaudio \
        jack \
        libuvc
fi
