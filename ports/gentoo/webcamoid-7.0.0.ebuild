# Copyright 2011-2016 Gonzalo Exequiel Pedone
# Distributed under the terms of the GNU General Public License v3

EAPI="5"

inherit eutils

DESCRIPTION="Webcamoid, the full webcam and multimedia suite."
HOMEPAGE="https://github.com/hipersayanX/webcamoid"
SRC_URI="https://github.com/hipersayanX/webcamoid/archive/${PV}.tar.gz"

LICENSE="GPLv3"
SLOT="0"
KEYWORDS="amd64 x86"
IUSE="+qt5"

RDEPEND="dev-qt/qtwidgets:5
	 dev-qt/qtgui:5
	 dev-qt/qtcore:5
	 dev-qt/qtsvg:5
	 dev-qt/qtquickcontrols
	 dev-qt/qtscript:5
	 dev-qt/qtgui
         media-video/ffmpeg"

src_unpack() {
        unpack "${PV}.tar.gz"

        mv -v "Webcamoid-${PV}" "${P}"

}

src_compile() {
    eqmake5  Webcamoid.pro
    emake || die
}

src_install() {
    emake INSTALL_ROOT="${D}" install || die
}
