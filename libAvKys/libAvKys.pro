# Webcamoid, webcam capture application.
# Copyright (C) 2016  Gonzalo Exequiel Pedone
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

exists(akcommons.pri) {
    include(akcommons.pri)
} else {
    error("akcommons.pri file not found.")
}

!isEmpty(BUILDDOCS):!isEqual(BUILDDOCS, 0) {
    DOCSOURCES += $${COMMONS_APPNAME}.qdocconf

    builddocs.input = DOCSOURCES
    builddocs.output = share/docs_auto/html/$${COMMONS_APPNAME}.index
    builddocs.commands = $${QDOCTOOL} ${QMAKE_FILE_IN}
    builddocs.variable_out = DOCSOUTPUT
    builddocs.name = Docs ${QMAKE_FILE_IN}
    builddocs.CONFIG += target_predeps

    QMAKE_EXTRA_COMPILERS += builddocs
    PRE_TARGETDEPS += compiler_builddocs_make_all
}

# Check what libraries and frameworks are available
load(configure)
QMAKE_CONFIG_TESTS_DIR=$$PWD/Tests
isEmpty(NOALSA): qtCompileTest(alsa)
isEmpty(NOAVFOUNDATION): qtCompileTest(avfoundation)
isEmpty(NOCOREMEDIAIO): qtCompileTest(cmio)
isEmpty(NOCOREAUDIO): qtCompileTest(coreaudio)
isEmpty(NODSHOW): qtCompileTest(dshow)

# Test FFmpeg
isEmpty(NOFFMPEG) {
    !isEmpty(FFMPEGINCLUDES): cache(FFMPEGINCLUDES)
    !isEmpty(FFMPEGLIBS): cache(FFMPEGLIBS)
    qtCompileTest(ffmpeg)
    CONFIG(config_ffmpeg): qtCompileTest(ffmpeg_avcodec_sendrecv)
    CONFIG(config_ffmpeg): qtCompileTest(ffmpeg_avcodec_subtitledata)
    CONFIG(config_ffmpeg): qtCompileTest(ffmpeg_avformat_codecpar)
    CONFIG(config_ffmpeg): qtCompileTest(ffmpeg_avutil_extraoptions)
    CONFIG(config_ffmpeg): qtCompileTest(ffmpeg_avutil_sampleformat64)
    CONFIG(config_ffmpeg): qtCompileTest(ffmpeg_avresample)
    CONFIG(config_ffmpeg): qtCompileTest(ffmpeg_swresample)
}

isEmpty(NOGSTREAMER) {
    !isEmpty(GSTREAMERINCLUDES): cache(GSTREAMERINCLUDES)
    !isEmpty(GSTREAMERLIBS): cache(GSTREAMERLIBS)
    qtCompileTest(gstreamer)
}

isEmpty(NOJACK): qtCompileTest(jack)

isEmpty(NOLIBUVC) {
    !isEmpty(LIBUSBINCLUDES): cache(LIBUSBINCLUDES)
    !isEmpty(LIBUSBLIBS): cache(LIBUSBLIBS)
    !isEmpty(LIBUVCINCLUDES): cache(LIBUVCINCLUDES)
    !isEmpty(LIBUVCLIBS): cache(LIBUVCLIBS)
    qtCompileTest(libuvc)
    CONFIG(config_libuvc): qtCompileTest(libusb_has_hotplug)
}

isEmpty(NOMEDIAFOUNDATION): qtCompileTest(mediafoundation)
isEmpty(NONDKAUDIO): qtCompileTest(ndk_audio)
isEmpty(NONDKCAMERA): qtCompileTest(ndk_camera)
isEmpty(NONDKMEDIA): qtCompileTest(ndk_media)
isEmpty(NOOPENSL): qtCompileTest(opensl)
isEmpty(NOPULSEAUDIO): qtCompileTest(pulseaudio)

isEmpty(NOV4L2): !android {
    qtCompileTest(v4l2)
    CONFIG(config_v4l2): qtCompileTest(v4l2_extendedcontrols)
    CONFIG(config_v4l2): qtCompileTest(v4l2_extraformats)
    isEmpty(NOV4LUTILS): CONFIG(config_v4l2): qtCompileTest(v4lutils)
}

isEmpty(NOWASAPI): qtCompileTest(wasapi)

TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += \
    Lib \
    Plugins

# Install rules

!android {
    !macx | !isEmpty(NOAPPBUNDLE) {
        INSTALLS += license
        license.files = ../COPYING
        license.path = $${LICENSEDIR}
    }
}

!isEmpty(BUILDDOCS): !isEqual(BUILDDOCS, 0) {
    INSTALLS += docs
    docs.files = share/docs_auto/html
    docs.path = $${HTMLDIR}
    docs.CONFIG += no_check_exist
}
