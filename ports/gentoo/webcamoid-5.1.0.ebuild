# Copyright 2011-2014 Gonzalo Exequiel Pedone
# Distributed under the terms of the GNU General Public License v3

DESCRIPTION="Webcamoid, the full webcam and multimedia suite."
HOMEPAGE="https://github.com/hipersayanX/Webcamoid"
LICENSE="GPLv3"
VER="5.1.0"
SRC_URI="https://github.com/hipersayanX/Webcamoid/archive/v${VER}.tar.gz"
SLOT= "0"
KEYWORDS="amd64 x86"

RDEPEND="dev-qt/qtgui
         kde-base/kdelibs
         media-video/ffmpeg
         media-plugins/frei0r-plugins"

DEPEND="${RDEPEND}
        sys-devel/bison
        sys-devel/flex"

src_unpack() {
        unpack "v${VER}.tar.gz"

        mv -v "Webcamoid-${VER}" "${P}"
}

src_compile() {
    eqmake4  Webcamoid.pro
    emake || die
}

src_install() {
    emake INSTALL_ROOT="${D}" install || die
}
