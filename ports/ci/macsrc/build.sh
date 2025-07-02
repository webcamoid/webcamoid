#!/bin/sh

set -e

if [ ! -z "${GITHUB_SHA}" ]; then
    export GIT_COMMIT_HASH="${GITHUB_SHA}"
elif [ ! -z "${CIRRUS_CHANGE_IN_REPO}" ]; then
    export GIT_COMMIT_HASH="${CIRRUS_CHANGE_IN_REPO}"
fi

export GIT_BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)

if [ -z "${GIT_BRANCH_NAME}" ]; then
    if [ ! -z "${GITHUB_REF_NAME}" ]; then
        export GIT_BRANCH_NAME="${GITHUB_REF_NAME}"
    elif [ ! -z "${CIRRUS_BRANCH}" ]; then
        export GIT_BRANCH_NAME="${CIRRUS_BRANCH}"
    else
        export GIT_BRANCH_NAME=master
    fi
fi

brew update
brew upgrade
brew install makeself

# Distribute a static version of DeployTools with Webcamoid source code

git clone https://github.com/webcamoid/DeployTools.git

component=WebcamoidSrc

rm -rf "${PWD}/webcamoid-packages"
mkdir -p /tmp/webcamoid-data
cp -rf . /tmp/webcamoid-data/${component}
rm -rf /tmp/webcamoid-data/${component}/.git
rm -rf /tmp/webcamoid-data/${component}/DeployTools/.git

mkdir -p /tmp/installScripts
cat << EOF > /tmp/installScripts/postinstall
# Install XCode command line tools and homebrew

if [ ! -d /Applications/Xcode.app ]; then
    echo "Installing XCode command line tools"
    echo
    xcode-select --install
    echo
fi

if ! command -v brew >/dev/null 2>&1; then
    echo "Installing Homebrew"
    echo
    /bin/bash -c "\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    echo
fi

# Install the build dependencies for Webcamoid

echo "Updating Homebrew database"
echo
brew update
brew upgrade
echo
echo "Installing dependencies"
echo
brew install \
    ccache \
    cmake \
    ffmpeg \
    git \
    pkg-config \
    portaudio \
    python \
    qt@6 \
    vlc \
    vulkan-headers

echo
echo "Building Webcamoid with Cmake"
echo

QT_VERSION=\$(ls /opt/homebrew/Cellar/qt | sort -V | tail -n 1)
QT_PATH=/opt/homebrew/Cellar/qt/\${QT_VERSION}
export PATH="\${QT_PATH}/bin:\${PATH}"
export LDFLAGS="\${LDFLAGS} -L\${QT_PATH}/lib"
export CPPFLAGS="\${CPPFLAGS} -I\${QT_PATH}/include"
export PKG_CONFIG_PATH="\${QT_PATH}/lib/pkgconfig:\${PKG_CONFIG_PATH}"
export MACOSX_DEPLOYMENT_TARGET="10.\$(sw_vers -productVersion | cut -d. -f1)"

INSTALL_PATH=/Applications/Webcamoid
BUILD_PATH=/tmp/build-Webcamoid-Release
mkdir -p "\${BUILD_PATH}"
cmake \
    -S /tmp/${component} \
    -B "\${BUILD_PATH}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="\${INSTALL_PATH}" \
    -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}" \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache \
    -DDAILY_BUILD=${DAILY_BUILD} \
    -DNOALSA=ON \
    -DNOGSTREAMER=ON \
    -DNOJACK=ON \
    -DNOLIBUSB=ON \
    -DNOLIBUVC=ON \
    -DNOPULSEAUDIO=ON \
    -DNOSDL=ON \
    -DNOLSMASH=ON \
    -DNOLIBMP4V2=ON \
    -DNOLIBWEBM=ON \
    -DNOLIBVPX=ON \
    -DNOSVTVP9=ON \
    -DNOAOMAV1=ON \
    -DNOSVTAV1=ON \
    -DNORAVIE=ON \
    -DNOX264=ON \
    -DNOLIBOPUS=ON \
    -DNOLIBVORBIS=ON \
    -DNOFDKAAC=ON \
    -DNOFAAC=ON \
    -DNOLAME=ON
cmake --build "\${BUILD_PATH}" --parallel \$(sysctl -n hw.ncpu)
cmake --install "\${BUILD_PATH}"

echo
echo "Packaging Webcamoid.app"
echo

patchesConf=/tmp/mac_patches.conf

cat << PATCHES_EOF > \${patchesConf}
[Vlc]
haveVLC = true
PATCHES_EOF

DT_PATH="/tmp/${component}/DeployTools"
export PYTHONPATH="\${DT_PATH}"
export DYLD_LIBRARY_PATH=\$(dirname \$(readlink /usr/local/bin/vlc))/VLC.app/Contents/MacOS/lib
export DYLD_FRAMEWORK_PATH=\${QT_PATH}/lib

# Only solve the dependencies, do not package into another pkg

python3 "\${DT_PATH}/deploy.py" \
    -r \
    -d "\${INSTALL_PATH}" \
    -c "\${BUILD_PATH}/package_info.conf" \
    -c "\${patchesConf}"

echo
echo "Webcamoid is ready to use at \${INSTALL_PATH}/Webcamoid.app"
EOF

verMaj=$(grep VER_MAJ libAvKys/cmake/ProjectCommons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
verMin=$(grep VER_MIN libAvKys/cmake/ProjectCommons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
verPat=$(grep VER_PAT libAvKys/cmake/ProjectCommons.cmake | awk '{print $2}' | tr -d ')' | head -n 1)
releaseVer=${verMaj}.${verMin}.${verPat}

cat << EOF > /tmp/package_info.conf
[Package]
name = webcamoid
identifier = io.github.webcamoid.Webcamoid
targetPlatform = mac
sourcesDir = ${PWD}
targetArch = any
version = ${releaseVer}
outputFormats = Makeself
dailyBuild = ${DAILY_BUILD}
buildType = Release
writeBuildInfo = false

[Git]
hideCommitCount = true

[Makeself]
name = webcamoid-installer
appName = Webcamoid
targetDir = /tmp
installScript = /tmp/installScripts/postinstall
hideArch = true
EOF

chmod +x /tmp/installScripts/postinstall

PACKAGES_DIR="${PWD}/webcamoid-packages/mac"

python3 DeployTools/deploy.py \
    -d "/tmp/webcamoid-data" \
    -c "/tmp/package_info.conf" \
    -o "${PACKAGES_DIR}"

echo
echo "Testing the package install"
echo

chmod +x "${PACKAGES_DIR}"/*.run
"${PACKAGES_DIR}"/*.run --accept

echo
echo "Test Webcamoid.app"
echo
/Applications/Webcamoid/Webcamoid.app/Contents/MacOS/Webcamoid --version
