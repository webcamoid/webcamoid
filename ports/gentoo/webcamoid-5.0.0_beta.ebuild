# Copyright 2011-2013 Gonzalo Exequiel Pedone
# Distributed under the terms of the GNU General Public License v3

DESCRIPTION="Webcamoid, the full webcam and multimedia suite."
HOMEPAGE="http://kde-apps.org/content/show.php/Webcamoid?content=144796"
LICENSE="GPLv3"
VER="5.0.0b2"
SRC_URI="https://github.com/hipersayanX/Webcamoid/archive/v${VER}.tar.gz"
SLOT= "0"
KEYWORDS="amd64 x86"

RDEPEND="dev-qt/qtgui
         kde-base/kdelibs
         media-video/ffmpeg
         media-plugins/frei0r-plugins
         media-libs/qimageblitz"

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
