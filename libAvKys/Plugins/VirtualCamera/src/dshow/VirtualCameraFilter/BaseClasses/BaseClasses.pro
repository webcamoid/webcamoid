# Webcamoid, webcam capture application.
# Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

exists(commons.pri) {
    include(commons.pri)
} else {
    exists(../../../../../../commons.pri) {
        include(../../../../../../commons.pri)
    } else {
        error("commons.pri file not found.")
    }
}

CONFIG += staticlib c++11
CONFIG -= qt

DESTDIR = $${OUT_PWD}

CONFIG(debug, debug|release) {
    TARGET = "strmbasd"
} else {
    TARGET = "strmbase"
}

TEMPLATE = lib

SOURCES = \
    src/amextra.cpp \
    src/amfilter.cpp \
    src/amvideo.cpp \
    src/arithutil.cpp \
    src/combase.cpp \
    src/cprop.cpp \
    src/ctlutil.cpp \
    src/ddmm.cpp \
    src/dllentry.cpp \
    src/dllsetup.cpp \
    src/mtype.cpp \
    src/outputq.cpp \
    src/perflog.cpp \
    src/pstream.cpp \
    src/pullpin.cpp \
    src/refclock.cpp \
    src/renbase.cpp \
    src/schedule.cpp \
    src/seekpt.cpp \
    src/source.cpp \
    src/strmctl.cpp \
    src/sysclock.cpp \
    src/transfrm.cpp \
    src/transip.cpp \
    src/videoctl.cpp \
    src/vtrans.cpp \
    src/winctrl.cpp \
    src/winutil.cpp \
    src/wxdebug.cpp \
    src/wxlist.cpp \
    src/wxutil.cpp

HEADERS =  \
    src/amextra.h \
    src/amfilter.h \
    src/cache.h \
    src/checkbmi.h \
    src/combase.h \
    src/cprop.h \
    src/ctlutil.h \
    src/ddmm.h \
    src/dllsetup.h \
    src/dxmperf.h \
    src/fourcc.h \
    src/measure.h \
    src/msgthrd.h \
    src/mtype.h \
    src/outputq.h \
    src/perflog.h \
    src/perfstruct.h \
    src/pstream.h \
    src/pullpin.h \
    src/refclock.h \
    src/reftime.h \
    src/renbase.h \
    src/schedule.h \
    src/seekpt.h \
    src/source.h \
    src/stdafx.h \
    src/streams.h \
    src/strmctl.h \
    src/sysclock.h \
    src/transfrm.h \
    src/transip.h \
    src/videoctl.h \
    src/vtrans.h \
    src/winctrl.h \
    src/winutil.h \
    src/wxdebug.h \
    src/wxlist.h \
    src/wxutil.h

DEFINES += __STDC_CONSTANT_MACROS NO_DSHOW_STRSAFE

INCLUDEPATH += \
    src

LIBS += \
    -lstrmiids \
    -luuid \
    -lole32 \
    -loleaut32 \
    -ladvapi32 \
    -luser32 \
    -lwinmm \
    -lgdi32

isEmpty(STATIC_BUILD) | isEqual(STATIC_BUILD, 0) {
    win32-g++: QMAKE_LFLAGS = -static-libgcc -static-libstdc++
}
