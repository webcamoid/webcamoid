# Webcamoid, webcam capture application.
# Copyright (C) 2021  Gonzalo Exequiel Pedone
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

cmake_minimum_required(VERSION 3.16)

project(Tests)

include(CheckSourceCompiles)

set(NOALSA OFF CACHE BOOL "Disable ALSA support")
message(CHECK_START "Checking for ALSA")

include(FindPkgConfig)

if (NOT NOALSA)
    pkg_check_modules(ALSA alsa)
endif ()

if (ALSA_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_ALSA TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOAVFOUNDATION OFF CACHE BOOL "Disable AVFoundation support")
message(CHECK_START "Checking for AVFoundation")

if (NOT NOAVFOUNDATION)
    set(AVFOUNDATION_FOUND TRUE)
    set(FRAMEWORKS
        CoreMedia
        CoreVideo
        Foundation
        AVFoundation)

    foreach (FRAMEWORK IN LISTS FRAMEWORKS)
        find_library(FOUND ${FRAMEWORK})

        if (NOT FOUND)
            set(AVFOUNDATION_FOUND FALSE)
            break()
        endif ()

        unset(FOUND CACHE)
    endforeach ()
endif ()

if (AVFOUNDATION_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_AVFOUNDATION TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOCOREMEDIAIO OFF CACHE BOOL "Disable CoreMediaIO support")
message(CHECK_START "Checking for CoreMediaIO")

if (NOT NOCOREMEDIAIO)
    set(COREMEDIAIO_FOUND TRUE)
    set(FRAMEWORKS
        CoreFoundation
        CoreMedia
        CoreMediaIO
        CoreVideo
        IOKit
        IOSurface)

    foreach (FRAMEWORK IN LISTS FRAMEWORKS)
        find_library(FOUND ${FRAMEWORK})

        if (NOT FOUND)
            set(COREMEDIAIO_FOUND FALSE)
            break()
        endif ()

        unset(FOUND CACHE)
    endforeach ()
endif ()

if (COREMEDIAIO_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_COREMEDIAIO TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOCOREAUDIO OFF CACHE BOOL "Disable Core Audio support")
message(CHECK_START "Checking for Core Audio")

if (NOT NOCOREAUDIO)
    set(COREAUDIO_FOUND TRUE)
    set(FRAMEWORKS
        CoreFoundation
        CoreAudio
        AudioUnit)

    foreach (FRAMEWORK IN LISTS FRAMEWORKS)
        find_library(FOUND ${FRAMEWORK})

        if (NOT FOUND)
            set(COREAUDIO_FOUND FALSE)
            break()
        endif ()

        unset(FOUND CACHE)
    endforeach ()
endif ()

if (COREAUDIO_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_COREAUDIO TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NODSHOW OFF CACHE BOOL "Disable DirectShow support")
message(CHECK_START "Checking for DirectShow")

if (NOT NODSHOW)
    set(CMAKE_REQUIRED_QUIET TRUE)
    set(CMAKE_REQUIRED_LIBRARIES
        advapi32
        kernel32
        ole32
        oleaut32
        shell32
        strmiids
        user32
        uuid
        winmm)
    check_source_compiles(CXX "
    #include <dshow.h>
    #include <dbt.h>
    #include <strmif.h>
    #include <usbiodef.h>

    int main()
    {
        return 0;
    }" DSHOW_FOUND)
    unset(CMAKE_REQUIRED_LIBRARIES)
    unset(CMAKE_REQUIRED_QUIET)
endif ()

if (DSHOW_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_DSHOW TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOFFMPEG OFF CACHE BOOL "Disable FFmpeg support")
message(CHECK_START "Checking for FFmpeg")

if (NOT NOFFMPEG)
    pkg_check_modules(FFMPEG
                      libavformat
                      libavcodec
                      libswscale
                      libavutil)
endif ()

if (FFMPEG_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_FFMPEG TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

if (HAVE_FFMPEG)
    message(CHECK_START "Checking for FFmpeg's libavresample")
    pkg_check_modules(FFMPEG_LIBAVRESAMPLE libavresample)

    if (FFMPEG_LIBAVRESAMPLE_FOUND)
        message(CHECK_PASS "yes")
        set(HAVE_FFMPEG_LIBAVRESAMPLE TRUE CACHE INTERNAL "")
    else ()
        message(CHECK_FAIL "no")
    endif ()

    message(CHECK_START "Checking for FFmpeg's libswresample")
    pkg_check_modules(FFMPEG_LIBSWRESAMPLE libswresample)

    if (FFMPEG_LIBSWRESAMPLE_FOUND)
        message(CHECK_PASS "yes")
        set(HAVE_FFMPEG_LIBSWRESAMPLE TRUE CACHE INTERNAL "")
    else ()
        message(CHECK_FAIL "no")
    endif ()
endif ()

set(NOGSTREAMER OFF CACHE BOOL "Disable GStreamer support")
message(CHECK_START "Checking for GStreamer")

if (NOT NOGSTREAMER)
    pkg_check_modules(GSTREAMER
                      gstreamer-1.0
                      gstreamer-app-1.0
                      gstreamer-audio-1.0
                      gstreamer-video-1.0
                      gstreamer-pbutils-1.0)
endif ()

if (GSTREAMER_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_GSTREAMER TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOJACK OFF CACHE BOOL "Disable JACK support")
message(CHECK_START "Checking for JACK")

if (NOT NOJACK)
    pkg_check_modules(JACK jack)
endif ()

if (JACK_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_JACK TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOLIBUVC OFF CACHE BOOL "Disable libuvc  support")
message(CHECK_START "Checking for libuvc")

if (NOT NOLIBUVC)
    pkg_check_modules(LIBUVC
                      libusb-1.0
                      libuvc>=0.0.6)
endif ()

if (LIBUVC_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_LIBUVC TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOMEDIAFOUNDATION OFF CACHE BOOL "Disable Microsoft Media Foundation support")
message(CHECK_START "Checking for Microsoft Media Foundation")

if (NOT NOMEDIAFOUNDATION)
    set(CMAKE_REQUIRED_QUIET TRUE)
    set(CMAKE_REQUIRED_LIBRARIES
        mf
        mfplat
        mfreadwrite
        mfuuid
        ole32
        strmiids)
    check_source_compiles(CXX "
    #include <mfapi.h>
    #include <mfidl.h>
    #include <mfreadwrite.h>
    #include <strmif.h>
    #include <dbt.h>
    #include <uuids.h>

    int main()
    {
        MFEnumDeviceSources(nullptr, nullptr, nullptr);

        return 0;
    }" MEDIAFOUNDATION_FOUND)
    unset(CMAKE_REQUIRED_LIBRARIES)
    unset(CMAKE_REQUIRED_QUIET)
endif ()

if (MEDIAFOUNDATION_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_MEDIAFOUNDATION TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NONDKAUDIO OFF CACHE BOOL "Disable Android NDK Audio support")
message(CHECK_START "Checking for Android NDK Audio")

if (NOT NONDKAUDIO)
    find_library(NDKAUDIO aaudio)
endif ()

if (NDKAUDIO)
    message(CHECK_PASS "yes")
    set(HAVE_NDKAUDIO TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NONDKCAMERA OFF CACHE BOOL "Disable Android NDK Camera support")
message(CHECK_START "Checking for Android NDK Camera")

if (NOT NONDKCAMERA)
    set(NDKCAMERA_FOUND TRUE)
    set(LIBRARIES
        android
        camera2ndk
        mediandk)

    foreach (LIBRARY IN LISTS LIBRARIES)
        find_library(FOUND ${LIBRARY})

        if (NOT FOUND)
            set(NDKCAMERA_FOUND FALSE)
            break()
        endif ()

        unset(FOUND CACHE)
    endforeach ()
endif ()

if (NDKCAMERA_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_NDKCAMERA TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NONDKMEDIA OFF CACHE BOOL "Disable Android NDK Media support")
message(CHECK_START "Checking for Android NDK Media")

if (NOT NONDKMEDIA)
    find_library(NDKMEDIA mediandk)
endif ()

if (NDKMEDIA)
    message(CHECK_PASS "yes")
    set(HAVE_NDKMEDIA TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOOPENSL OFF CACHE BOOL "Disable OpenSL ES support")
message(CHECK_START "Checking for OpenSL ES")

if (NOT NOOPENSL)
    find_library(OPENSL OpenSLES)
endif ()

if (OPENSL)
    message(CHECK_PASS "yes")
    set(HAVE_OPENSL TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOPULSEAUDIO OFF CACHE BOOL "Disable PulseAudio support")
message(CHECK_START "Checking for PulseAudio")

if (NOT NOPULSEAUDIO)
    pkg_check_modules(PULSEAUDIO libpulse-simple)
endif ()

if (PULSEAUDIO_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_PULSEAUDIO TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOV4L2 OFF CACHE BOOL "Disable V4L2 support")
message(CHECK_START "Checking for V4L2")

if (NOT NOV4L2 AND NOT ANDROID)
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_source_compiles(CXX "
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/time.h>
    #include <linux/videodev2.h>

    int main()
    {
        return 0;
    }
    " V4L2_FOUND)
    unset(CMAKE_REQUIRED_QUIET)
endif ()

if (V4L2_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_V4L2 TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()

set(NOV4LUTILS OFF CACHE BOOL "Disable V4l-utils support")

if (NOT NOV4LUTILS AND V4L2_FOUND)
    message(CHECK_START "Checking for V4l-utils")
    pkg_check_modules(V4LUTILS libv4l2)

    if (V4LUTILS_FOUND)
        message(CHECK_PASS "yes")
        set(HAVE_V4LUTILS TRUE CACHE INTERNAL "")
    else ()
        message(CHECK_FAIL "no")
    endif ()
endif ()

set(NOWASAPI OFF CACHE BOOL "Disable WASAPI support")
message(CHECK_START "Checking for WASAPI")

if (NOT NOWASAPI)
    set(CMAKE_REQUIRED_QUIET TRUE)
    set(CMAKE_REQUIRED_LIBRARIES
        ole32
        winmm)
    check_source_compiles(CXX "
    #include <objbase.h>
    #include <initguid.h>
    #include <audioclient.h>
    #include <mmdeviceapi.h>
    #include <propkeydef.h>

    int main()
    {
        return 0;
    }" WASAPI_FOUND)
    unset(CMAKE_REQUIRED_LIBRARIES)
    unset(CMAKE_REQUIRED_QUIET)
endif ()

if (WASAPI_FOUND)
    message(CHECK_PASS "yes")
    set(HAVE_WASAPI TRUE CACHE INTERNAL "")
else ()
    message(CHECK_FAIL "no")
endif ()
