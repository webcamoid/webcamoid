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

component=WebcamoidSrc

rm -rf "${PWD}/webcamoid-packages"
mkdir -p /tmp/webcamoid-data
cp -rf . /tmp/webcamoid-data/${component}
rm -rf /tmp/webcamoid-data/${component}/.git

mkdir -p /tmp/installScripts
cat << EOF > /tmp/installScripts/postinstall
# Install XCode command line tools and homebrew

if [ ! -d /Applications/Xcode.app ]; then
    xcode-select --install
fi

if ! command -v brew >/dev/null 2>&1; then
    /bin/bash -c "\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# Install the build dependencies for Webcamoid

brew update
brew upgrade
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

# Setup the build environment

HOMEBREW_PATH=/usr/local
export PATH="\${HOMEBREW_PATH}/opt/qt@6/bin:\${PATH}"
export LDFLAGS="\${LDFLAGS} -L\${HOMEBREW_PATH}/opt/qt@6/lib"
export CPPFLAGS="\${CPPFLAGS} -I\${HOMEBREW_PATH}/opt/qt@6/include"
export PKG_CONFIG_PATH="\${HOMEBREW_PATH}/opt/qt@6/lib/pkgconfig:\${PKG_CONFIG_PATH}"
export MACOSX_DEPLOYMENT_TARGET="10.14"

# Build Webcamoid

TEMP_PATH=/Applications/tmp
BUILD_PATH=\${TEMP_PATH}/build-Webcamoid-Release
mkdir -p "\${BUILD_PATH}"
mkdir -p "\${TEMP_PATH}"
cmake \
    -S /Applications/${component} \
    -B "\${BUILD_PATH}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=\${TEMP_PATH}/webcamoid-data \
    -DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}" \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_OBJCXX_COMPILER_LAUNCHER=ccache \
    -DNOGSTREAMER=ON \
    -DNOJACK=ON \
    -DNOLIBUVC=ON \
    -DNOPULSEAUDIO=ON
cmake --build "\${BUILD_PATH}" --parallel 4
cmake --install "\${BUILD_PATH}"

# Deploy the application bundle

git clone https://github.com/webcamoid/DeployTools.git \${TEMP_PATH}/DeployTools

export PYTHONPATH="\${TEMP_PATH}/DeployTools"
export DYLD_LIBRARY_PATH=\$(dirname \$(readlink /usr/local/bin/vlc))/VLC.app/Contents/MacOS/lib

# Only solve the dependencies, do not package into another pkg

python3 \${TEMP_PATH}/DeployTools/deploy.py \
    -r \
    -d \${TEMP_PATH}/webcamoid-data \
    -c "\${BUILD_PATH}/package_info.conf"

# Copy the bundle to the /Applications folder

cp -rf \${TEMP_PATH}/webcamoid-data/Webcamoid.app /Applications

# Remove the sources file

rm -rf /Applications/${component}
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
outputFormats = MacPkg
dailyBuild = ${DAILY_BUILD}
buildType = Release
writeBuildInfo = false

[Git]
hideCommitCount = true

[MacPkg]
name = webcamoid-installer
appName = Webcamoid
productTitle = Webcamoid
description = Webcamoid, the ultimate webcam suite!
installScripts = /tmp/installScripts
hideArch = true
verbose = true
EOF

chmod +x /tmp/installScripts/postinstall

git clone https://github.com/webcamoid/DeployTools.git

PACKAGES_DIR="${PWD}/webcamoid-packages/mac"

python3 DeployTools/deploy.py \
    -d "/tmp/webcamoid-data" \
    -c "/tmp/package_info.conf" \
    -o "${PACKAGES_DIR}"

echo
echo "Testing the package install"
echo

for pkg in "${PACKAGES_DIR}"/*.pkg; do
    sudo installer -pkg "${pkg}" -target / -verboseR || true
done

echo
echo "Listing /Applications"
echo
ls -l /Applications
echo
echo "Listing /tmp"
echo
ls -l /Applications/tmp
