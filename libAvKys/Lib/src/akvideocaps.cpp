/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <QDataStream>
#include <QDebug>
#include <QMetaEnum>
#include <QSize>
#include <QVector>
#include <QQmlEngine>
#include <QtMath>

#include "akvideocaps.h"
#include "akfrac.h"
#include "akcaps.h"
#include "akvideoformatspec.h"

#define VFT_Unknown AkVideoFormatSpec::VFT_Unknown
#define VFT_RGB     AkVideoFormatSpec::VFT_RGB
#define VFT_YUV     AkVideoFormatSpec::VFT_YUV
#define VFT_Gray    AkVideoFormatSpec::VFT_Gray

#define CT_END AkColorComponent::CT_Unknown
#define CT_R   AkColorComponent::CT_R
#define CT_G   AkColorComponent::CT_G
#define CT_B   AkColorComponent::CT_B
#define CT_Y   AkColorComponent::CT_Y
#define CT_U   AkColorComponent::CT_U
#define CT_V   AkColorComponent::CT_V
#define CT_A   AkColorComponent::CT_A

#define MAX_PLANES 4
#define MAX_COMPONENTS 4

struct Component
{
    AkColorComponent::ComponentType type;
    size_t step;
    size_t offset;
    size_t shift;
    size_t byteLength;
    size_t length;
    size_t widthDiv;
    size_t heightDiv;
};

struct Plane
{
    size_t ncomponents;
    Component components[MAX_COMPONENTS];
    size_t bitsSize;
};

struct VideoFormat
{
    AkVideoCaps::PixelFormat format;
    AkVideoFormatSpec::VideoFormatType type;
    int endianness;
    size_t nplanes;
    Plane planes[MAX_PLANES];

    static inline const VideoFormat *formats()
    {
        static const VideoFormat akVideoFormatSpecTable[] {
            {AkVideoCaps::Format_0bgr,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_B, 4, 1, 0, 1, 8, 0, 0},
                   {CT_G, 4, 2, 0, 1, 8, 0, 0},
                   {CT_R, 4, 3, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_0bgrpackbe,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_B, 4, 0, 16, 4, 8, 0, 0},
                   {CT_G, 4, 0,  8, 4, 8, 0, 0},
                   {CT_R, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_0bgrpackle,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_B, 4, 0, 16, 4, 8, 0, 0},
                   {CT_G, 4, 0,  8, 4, 8, 0, 0},
                   {CT_R, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_0bgr444be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0,  8, 2, 4, 0, 0},
                   {CT_G, 2, 0,  4, 2, 4, 0, 0},
                   {CT_R, 2, 0,  0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_0bgr444le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0,  8, 2, 4, 0, 0},
                   {CT_G, 2, 0,  4, 2, 4, 0, 0},
                   {CT_R, 2, 0,  0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_0rgb,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_R, 4, 1, 0, 1, 8, 0, 0},
                   {CT_G, 4, 2, 0, 1, 8, 0, 0},
                   {CT_B, 4, 3, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_0rgbpackbe,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_R, 4, 0, 16, 4, 8, 0, 0},
                   {CT_G, 4, 0,  8, 4, 8, 0, 0},
                   {CT_B, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_0rgbpackle,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_R, 4, 0, 16, 4, 8, 0, 0},
                   {CT_G, 4, 0,  8, 4, 8, 0, 0},
                   {CT_B, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_0yuv,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_Y, 4, 1, 0, 1, 8, 0, 0},
                   {CT_U, 4, 2, 0, 1, 8, 0, 0},
                   {CT_V, 4, 3, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_abgr,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{4, {{CT_A, 4, 0, 0, 1, 8, 0, 0},
                   {CT_B, 4, 1, 0, 1, 8, 0, 0},
                   {CT_G, 4, 2, 0, 1, 8, 0, 0},
                   {CT_R, 4, 3, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_abgr1555be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_A, 2, 0, 15, 2, 1, 0, 0},
                   {CT_B, 2, 0, 10, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 5, 0, 0},
                   {CT_R, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_abgr1555le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_A, 2, 0, 15, 2, 1, 0, 0},
                   {CT_B, 2, 0, 10, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 5, 0, 0},
                   {CT_R, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_abgr4444be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_A, 2, 0, 12, 2, 4, 0, 0},
                   {CT_B, 2, 0,  8, 2, 4, 0, 0},
                   {CT_G, 2, 0,  4, 2, 4, 0, 0},
                   {CT_R, 2, 0,  0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_abgr4444le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_A, 2, 0, 12, 2, 4, 0, 0},
                   {CT_B, 2, 0,  8, 2, 4, 0, 0},
                   {CT_G, 2, 0,  4, 2, 4, 0, 0},
                   {CT_R, 2, 0,  0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_argb,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{4, {{CT_A, 4, 0, 0, 1, 8, 0, 0},
                   {CT_R, 4, 1, 0, 1, 8, 0, 0},
                   {CT_G, 4, 2, 0, 1, 8, 0, 0},
                   {CT_B, 4, 3, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_argbpackbe,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_A, 4, 0, 24, 4, 8, 0, 0},
                   {CT_R, 4, 0, 16, 4, 8, 0, 0},
                   {CT_G, 4, 0,  8, 4, 8, 0, 0},
                   {CT_B, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_argbpackle,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_A, 4, 0, 24, 4, 8, 0, 0},
                   {CT_R, 4, 0, 16, 4, 8, 0, 0},
                   {CT_G, 4, 0,  8, 4, 8, 0, 0},
                   {CT_B, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_abgrpackbe,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_A, 4, 0, 24, 4, 8, 0, 0},
                   {CT_B, 4, 0, 16, 4, 8, 0, 0},
                   {CT_G, 4, 0,  8, 4, 8, 0, 0},
                   {CT_R, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_abgrpackle,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_A, 4, 0, 24, 4, 8, 0, 0},
                   {CT_B, 4, 0, 16, 4, 8, 0, 0},
                   {CT_G, 4, 0,  8, 4, 8, 0, 0},
                   {CT_R, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_argb1555be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_A, 2, 0, 15, 2, 1, 0, 0},
                   {CT_R, 2, 0, 10, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 5, 0, 0},
                   {CT_B, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_argb1555le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_A, 2, 0, 15, 2, 1, 0, 0},
                   {CT_R, 2, 0, 10, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 5, 0, 0},
                   {CT_B, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_argb4444be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_A, 2, 0, 12, 2, 4, 0, 0},
                   {CT_R, 2, 0,  8, 2, 4, 0, 0},
                   {CT_G, 2, 0,  4, 2, 4, 0, 0},
                   {CT_B, 2, 0,  0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_argb4444le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_A, 2, 0, 12, 2, 4, 0, 0},
                   {CT_R, 2, 0,  8, 2, 4, 0, 0},
                   {CT_G, 2, 0,  4, 2, 4, 0, 0},
                   {CT_B, 2, 0,  0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_argb64be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_A, 8, 0, 0, 2, 16, 0, 0},
                   {CT_R, 8, 2, 0, 2, 16, 0, 0},
                   {CT_G, 8, 4, 0, 2, 16, 0, 0},
                   {CT_B, 8, 6, 0, 2, 16, 0, 0}}, 64}
             }},
            {AkVideoCaps::Format_argb64le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_A, 8, 0, 0, 2, 16, 0, 0},
                   {CT_R, 8, 2, 0, 2, 16, 0, 0},
                   {CT_G, 8, 4, 0, 2, 16, 0, 0},
                   {CT_B, 8, 6, 0, 2, 16, 0, 0}}, 64}
             }},
            {AkVideoCaps::Format_argb2101010be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_A, 8, 0, 30, 2,  2, 0, 0},
                   {CT_R, 8, 0, 20, 2, 10, 0, 0},
                   {CT_G, 8, 0, 10, 2, 10, 0, 0},
                   {CT_B, 8, 0,  0, 2, 10, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_argb2101010le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_A, 8, 0, 30, 2,  2, 0, 0},
                   {CT_R, 8, 0, 20, 2, 10, 0, 0},
                   {CT_G, 8, 0, 10, 2, 10, 0, 0},
                   {CT_B, 8, 0,  0, 2, 10, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgba1010102be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_R, 8, 0, 22, 2, 10, 0, 0},
                   {CT_G, 8, 0, 12, 2, 10, 0, 0},
                   {CT_B, 8, 0,  2, 2, 10, 0, 0},
                   {CT_A, 8, 0,  0, 2,  2, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgba1010102le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_R, 8, 0, 22, 2, 10, 0, 0},
                   {CT_G, 8, 0, 12, 2, 10, 0, 0},
                   {CT_B, 8, 0,  2, 2, 10, 0, 0},
                   {CT_A, 8, 0,  0, 2,  2, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgb0444be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 12, 2, 4, 0, 0},
                   {CT_G, 2, 0,  8, 2, 4, 0, 0},
                   {CT_B, 2, 0,  4, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb0444le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 12, 2, 4, 0, 0},
                   {CT_G, 2, 0,  8, 2, 4, 0, 0},
                   {CT_B, 2, 0,  4, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_ayuvpackbe,
             VFT_YUV,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_A, 4, 0, 24, 4, 8, 0, 0},
                   {CT_Y, 4, 0, 16, 4, 8, 0, 0},
                   {CT_U, 4, 0,  8, 4, 8, 0, 0},
                   {CT_V, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_ayuvpackle,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_A, 4, 0, 24, 4, 8, 0, 0},
                   {CT_Y, 4, 0, 16, 4, 8, 0, 0},
                   {CT_U, 4, 0,  8, 4, 8, 0, 0},
                   {CT_V, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_ayuv,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{4, {{CT_A, 4, 0, 0, 1, 8, 0, 0},
                   {CT_Y, 4, 1, 0, 1, 8, 0, 0},
                   {CT_U, 4, 2, 0, 1, 8, 0, 0},
                   {CT_V, 4, 3, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_ayuv64be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_A, 8, 0, 0, 2, 16, 0, 0},
                   {CT_Y, 8, 2, 0, 2, 16, 0, 0},
                   {CT_U, 8, 4, 0, 2, 16, 0, 0},
                   {CT_V, 8, 6, 0, 2, 16, 0, 0}}, 64}
             }},
            {AkVideoCaps::Format_ayuv64le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_A, 8, 0, 0, 2, 16, 0, 0},
                   {CT_Y, 8, 2, 0, 2, 16, 0, 0},
                   {CT_U, 8, 4, 0, 2, 16, 0, 0},
                   {CT_V, 8, 6, 0, 2, 16, 0, 0}}, 64}
             }},
            {AkVideoCaps::Format_bgr0,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_B, 4, 0, 0, 1, 8, 0, 0},
                   {CT_G, 4, 1, 0, 1, 8, 0, 0},
                   {CT_R, 4, 2, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_bgr24,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_B, 3, 0, 0, 1, 8, 0, 0},
                   {CT_G, 3, 1, 0, 1, 8, 0, 0},
                   {CT_R, 3, 2, 0, 1, 8, 0, 0}}, 24}
             }},
            {AkVideoCaps::Format_bgr0packbe,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_B, 4, 0, 24, 4, 8, 0, 0},
                   {CT_G, 4, 0, 16, 4, 8, 0, 0},
                   {CT_R, 4, 0,  8, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_bgr0packle,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_B, 4, 0, 24, 4, 8, 0, 0},
                   {CT_G, 4, 0, 16, 4, 8, 0, 0},
                   {CT_R, 4, 0,  8, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_bgr0444be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0, 12, 2, 4, 0, 0},
                   {CT_G, 2, 0,  8, 2, 4, 0, 0},
                   {CT_R, 2, 0,  4, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgr0444le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0, 12, 2, 4, 0, 0},
                   {CT_G, 2, 0,  8, 2, 4, 0, 0},
                   {CT_R, 2, 0,  4, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgr0555be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0, 11, 2, 4, 0, 0},
                   {CT_G, 2, 0,  6, 2, 4, 0, 0},
                   {CT_R, 2, 0,  1, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgr0555le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0, 11, 2, 4, 0, 0},
                   {CT_G, 2, 0,  6, 2, 4, 0, 0},
                   {CT_R, 2, 0,  1, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgr444be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0, 8, 2, 4, 0, 0},
                   {CT_G, 2, 0, 4, 2, 4, 0, 0},
                   {CT_R, 2, 0, 0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgr444le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0, 8, 2, 4, 0, 0},
                   {CT_G, 2, 0, 4, 2, 4, 0, 0},
                   {CT_R, 2, 0, 0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgr48be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_B, 6, 0, 0, 2, 16, 0, 0},
                   {CT_G, 6, 2, 0, 2, 16, 0, 0},
                   {CT_R, 6, 4, 0, 2, 16, 0, 0}}, 48}
             }},
            {AkVideoCaps::Format_bgr48le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_B, 6, 0, 0, 2, 16, 0, 0},
                   {CT_G, 6, 2, 0, 2, 16, 0, 0},
                   {CT_R, 6, 4, 0, 2, 16, 0, 0}}, 48}
             }},
            {AkVideoCaps::Format_bgr555be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0, 10, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 5, 0, 0},
                   {CT_R, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgr555le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0, 10, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 5, 0, 0},
                   {CT_R, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgr565be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 6, 0, 0},
                   {CT_R, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgr565le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_B, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 6, 0, 0},
                   {CT_R, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgrapackbe,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_B, 4, 0, 24, 4, 8, 0, 0},
                   {CT_G, 4, 0, 16, 4, 8, 0, 0},
                   {CT_R, 4, 0,  8, 4, 8, 0, 0},
                   {CT_A, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_bgrapackle,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_B, 4, 0, 24, 4, 8, 0, 0},
                   {CT_G, 4, 0, 16, 4, 8, 0, 0},
                   {CT_R, 4, 0,  8, 4, 8, 0, 0},
                   {CT_A, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_bgr233,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_B, 1, 0, 6, 1, 2, 0, 0},
                   {CT_G, 1, 0, 3, 1, 3, 0, 0},
                   {CT_R, 1, 0, 0, 1, 3, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_bgr332,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_B, 1, 0, 5, 1, 3, 0, 0},
                   {CT_G, 1, 0, 2, 1, 3, 0, 0},
                   {CT_R, 1, 0, 0, 1, 2, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_bgra,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{4, {{CT_B, 4, 0, 0, 1, 8, 0, 0},
                   {CT_G, 4, 1, 0, 1, 8, 0, 0},
                   {CT_R, 4, 2, 0, 1, 8, 0, 0},
                   {CT_A, 4, 3, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_bgra4444be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_B, 2, 0, 12, 2, 4, 0, 0},
                   {CT_G, 2, 0,  8, 2, 4, 0, 0},
                   {CT_R, 2, 0,  4, 2, 4, 0, 0},
                   {CT_A, 2, 0,  0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgra4444le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_B, 2, 0, 12, 2, 4, 0, 0},
                   {CT_G, 2, 0,  8, 2, 4, 0, 0},
                   {CT_R, 2, 0,  4, 2, 4, 0, 0},
                   {CT_A, 2, 0,  0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgra5551be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_B, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  6, 2, 5, 0, 0},
                   {CT_R, 2, 0,  1, 2, 5, 0, 0},
                   {CT_A, 2, 0,  0, 2, 1, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgra5551le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_B, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  6, 2, 5, 0, 0},
                   {CT_R, 2, 0,  1, 2, 5, 0, 0},
                   {CT_A, 2, 0,  0, 2, 1, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgra64be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_B, 8, 0, 0, 2, 16, 0, 0},
                   {CT_G, 8, 2, 0, 2, 16, 0, 0},
                   {CT_R, 8, 4, 0, 2, 16, 0, 0},
                   {CT_A, 8, 6, 0, 2, 16, 0, 0}}, 64}
             }},
            {AkVideoCaps::Format_bgra64le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_B, 8, 0, 0, 2, 16, 0, 0},
                   {CT_G, 8, 2, 0, 2, 16, 0, 0},
                   {CT_R, 8, 4, 0, 2, 16, 0, 0},
                   {CT_A, 8, 6, 0, 2, 16, 0, 0}}, 64}
             }},
            {AkVideoCaps::Format_gbr24p,
             VFT_RGB,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_G, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_B, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_R, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_gbrap,
             VFT_RGB,
             Q_BYTE_ORDER,
             4,
             {{1, {{CT_G, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_B, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_R, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_A, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_gbrap10be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_G, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrap10le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_G, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrap12be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_G, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrap12le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_G, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrap16be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_G, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrap16le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_G, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrp,
             VFT_RGB,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_G, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_B, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_R, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_gbrp10be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_G, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrp10le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_G, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrp12be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_G, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrp12le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_G, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrp14be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_G, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 14, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrp14le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_G, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 14, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrp16be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_G, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrp16le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_G, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrp9be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_G, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gbrp9le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_G, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_B, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_R, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray9be,
             VFT_Gray,
             Q_BIG_ENDIAN,
             1,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray9le,
             VFT_Gray,
             Q_LITTLE_ENDIAN,
             1,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray10be,
             VFT_Gray,
             Q_BIG_ENDIAN,
             1,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray10le,
             VFT_Gray,
             Q_LITTLE_ENDIAN,
             1,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray12be,
             VFT_Gray,
             Q_BIG_ENDIAN,
             1,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray12le,
             VFT_Gray,
             Q_LITTLE_ENDIAN,
             1,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray14be,
             VFT_Gray,
             Q_BIG_ENDIAN,
             1,
             {{1, {{CT_Y, 2, 0, 0, 2, 14, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray14le,
             VFT_Gray,
             Q_LITTLE_ENDIAN,
             1,
             {{1, {{CT_Y, 2, 0, 0, 2, 14, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray16be,
             VFT_Gray,
             Q_BIG_ENDIAN,
             1,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray16le,
             VFT_Gray,
             Q_LITTLE_ENDIAN,
             1,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_gray32be,
             VFT_Gray,
             Q_BIG_ENDIAN,
             1,
             {{1, {{CT_Y, 4, 0, 0, 4, 32, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_gray32le,
             VFT_Gray,
             Q_LITTLE_ENDIAN,
             1,
             {{1, {{CT_Y, 4, 0, 0, 4, 32, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_gray4,
             VFT_Gray,
             Q_BYTE_ORDER,
             1,
             {{1, {{CT_Y, 1, 0, 0, 1, 4, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_gray6,
             VFT_Gray,
             Q_BYTE_ORDER,
             1,
             {{1, {{CT_Y, 1, 0, 0, 1, 6, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_gray8,
             VFT_Gray,
             Q_BYTE_ORDER,
             1,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_graya8,
             VFT_Gray,
             Q_BYTE_ORDER,
             1,
             {{2, {{CT_Y, 2, 0, 0, 1, 8, 0, 0},
                   {CT_A, 2, 1, 0, 1, 8, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_graya8packbe,
             VFT_Gray,
             Q_BIG_ENDIAN,
             1,
             {{2, {{CT_Y, 2, 0, 8, 2, 8, 0, 0},
                   {CT_A, 2, 0, 0, 2, 8, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_graya8packle,
             VFT_Gray,
             Q_LITTLE_ENDIAN,
             1,
             {{2, {{CT_Y, 2, 0, 8, 2, 8, 0, 0},
                   {CT_A, 2, 0, 0, 2, 8, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_graya16be,
             VFT_Gray,
             Q_BIG_ENDIAN,
             1,
             {{2, {{CT_Y, 4, 0, 0, 2, 16, 0, 0},
                   {CT_A, 4, 2, 0, 2, 16, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_graya16le,
             VFT_Gray,
             Q_LITTLE_ENDIAN,
             1,
             {{2, {{CT_Y, 4, 0, 0, 2, 16, 0, 0},
                   {CT_A, 4, 2, 0, 2, 16, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_nv12,
             VFT_YUV,
             Q_BYTE_ORDER,
             2,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {2, {{CT_U, 2, 0, 0, 1, 8, 1, 1},
                   {CT_V, 2, 1, 0, 1, 8, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_nv12a,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {2, {{CT_U, 2, 0, 0, 1, 8, 1, 1},
                   {CT_V, 2, 1, 0, 1, 8, 1, 1}}, 8},
              {1, {{CT_A, 1, 0, 0, 1, 8, 0, 0}}, 8},
             }},
            {AkVideoCaps::Format_nv16,
             VFT_YUV,
             Q_BYTE_ORDER,
             2,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {2, {{CT_U, 2, 0, 0, 1, 8, 1, 0},
                   {CT_V, 2, 1, 0, 1, 8, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_nv20be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {2, {{CT_V, 4, 0, 0, 2, 10, 1, 0},
                   {CT_U, 4, 2, 0, 2, 10, 1, 0}}, 16}
             }},
            {AkVideoCaps::Format_nv20le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {2, {{CT_V, 4, 0, 0, 2, 10, 1, 0},
                   {CT_U, 4, 2, 0, 2, 10, 1, 0}}, 16}
             }},
            {AkVideoCaps::Format_nv21,
             VFT_YUV,
             Q_BYTE_ORDER,
             2,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {2, {{CT_V, 2, 0, 0, 1, 8, 1, 1},
                   {CT_U, 2, 1, 0, 1, 8, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_nv24,
             VFT_YUV,
             Q_BYTE_ORDER,
             2,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {2, {{CT_U, 2, 0, 0, 1, 8, 0, 0},
                   {CT_V, 2, 1, 0, 1, 8, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_nv42,
             VFT_YUV,
             Q_BYTE_ORDER,
             2,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {2, {{CT_V, 2, 0, 0, 1, 8, 0, 0},
                   {CT_U, 2, 1, 0, 1, 8, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_nv61,
             VFT_YUV,
             Q_BYTE_ORDER,
             2,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {2, {{CT_V, 2, 0, 0, 1, 8, 1, 0},
                   {CT_U, 2, 1, 0, 1, 8, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_p010be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 6, 2, 10, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 6, 2, 10, 1, 1},
                   {CT_V, 4, 2, 6, 2, 10, 1, 1}}, 16}
             }},
            {AkVideoCaps::Format_p010le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 6, 2, 10, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 6, 2, 10, 1, 1},
                   {CT_V, 4, 2, 6, 2, 10, 1, 1}}, 16}
             }},
            {AkVideoCaps::Format_p016be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 0, 2, 16, 1, 1},
                   {CT_V, 4, 2, 0, 2, 16, 1, 1}}, 16}
             }},
            {AkVideoCaps::Format_p016le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 0, 2, 16, 1, 1},
                   {CT_V, 4, 2, 0, 2, 16, 1, 1}}, 16}
             }},
            {AkVideoCaps::Format_p210be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 6, 2, 10, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 6, 2, 10, 1, 0},
                   {CT_V, 4, 2, 6, 2, 10, 1, 0}}, 16}
             }},
            {AkVideoCaps::Format_p210le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 6, 2, 10, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 6, 2, 10, 1, 0},
                   {CT_V, 4, 2, 6, 2, 10, 1, 0}}, 16}
             }},
            {AkVideoCaps::Format_p216be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 0, 2, 16, 1, 0},
                   {CT_V, 4, 2, 0, 2, 16, 1, 0}}, 16}
             }},
            {AkVideoCaps::Format_p216le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 0, 2, 16, 1, 0},
                   {CT_V, 4, 2, 0, 2, 16, 1, 0}}, 16}
             }},
            {AkVideoCaps::Format_p410be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 6, 2, 10, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 6, 2, 10, 0, 0},
                   {CT_V, 4, 2, 6, 2, 10, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_p410le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 6, 2, 10, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 6, 2, 10, 0, 0},
                   {CT_V, 4, 2, 6, 2, 10, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_p416be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 0, 2, 16, 0, 0},
                   {CT_V, 4, 2, 0, 2, 16, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_p416le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             2,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {2, {{CT_U, 4, 0, 0, 2, 16, 0, 0},
                   {CT_V, 4, 2, 0, 2, 16, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgb0,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_R, 4, 0, 0, 1, 8, 0, 0},
                   {CT_G, 4, 1, 0, 1, 8, 0, 0},
                   {CT_B, 4, 2, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgb24,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_R, 3, 0, 0, 1, 8, 0, 0},
                   {CT_G, 3, 1, 0, 1, 8, 0, 0},
                   {CT_B, 3, 2, 0, 1, 8, 0, 0}}, 24}
             }},
            {AkVideoCaps::Format_rgb24p,
             VFT_RGB,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_R, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_G, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_B, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_rgb444be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 8, 2, 4, 0, 0},
                   {CT_G, 2, 0, 4, 2, 4, 0, 0},
                   {CT_B, 2, 0, 0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb444le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 8, 2, 4, 0, 0},
                   {CT_G, 2, 0, 4, 2, 4, 0, 0},
                   {CT_B, 2, 0, 0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb48be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_R, 6, 0, 0, 2, 16, 0, 0},
                   {CT_G, 6, 2, 0, 2, 16, 0, 0},
                   {CT_B, 6, 4, 0, 2, 16, 0, 0}}, 48}
             }},
            {AkVideoCaps::Format_rgb48le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_R, 6, 0, 0, 2, 16, 0, 0},
                   {CT_G, 6, 2, 0, 2, 16, 0, 0},
                   {CT_B, 6, 4, 0, 2, 16, 0, 0}}, 48}
             }},
            {AkVideoCaps::Format_rgb555be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 10, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 5, 0, 0},
                   {CT_B, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb555le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 10, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 5, 0, 0},
                   {CT_B, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb5550be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  6, 2, 5, 0, 0},
                   {CT_B, 2, 0,  1, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb5550le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  6, 2, 5, 0, 0},
                   {CT_B, 2, 0,  1, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb565be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 6, 0, 0},
                   {CT_B, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb565le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  5, 2, 6, 0, 0},
                   {CT_B, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb233,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_R, 1, 0, 6, 1, 2, 0, 0},
                   {CT_G, 1, 0, 3, 1, 3, 0, 0},
                   {CT_B, 1, 0, 0, 1, 3, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_rgb332,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_R, 1, 0, 5, 1, 3, 0, 0},
                   {CT_G, 1, 0, 2, 1, 3, 0, 0},
                   {CT_B, 1, 0, 0, 1, 2, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_rgba,
             VFT_RGB,
             Q_BYTE_ORDER,
             1,
             {{4, {{CT_R, 4, 0, 0, 1, 8, 0, 0},
                   {CT_G, 4, 1, 0, 1, 8, 0, 0},
                   {CT_B, 4, 2, 0, 1, 8, 0, 0},
                   {CT_A, 4, 3, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgbapackbe,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_R, 4, 0, 24, 4, 8, 0, 0},
                   {CT_G, 4, 0, 16, 4, 8, 0, 0},
                   {CT_B, 4, 0,  8, 4, 8, 0, 0},
                   {CT_A, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgbapackle,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_R, 4, 0, 24, 4, 8, 0, 0},
                   {CT_G, 4, 0, 16, 4, 8, 0, 0},
                   {CT_B, 4, 0,  8, 4, 8, 0, 0},
                   {CT_A, 4, 0,  0, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgb0packbe,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_R, 4, 0, 24, 4, 8, 0, 0},
                   {CT_G, 4, 0, 16, 4, 8, 0, 0},
                   {CT_B, 4, 0,  8, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgb0packle,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_R, 4, 0, 24, 4, 8, 0, 0},
                   {CT_G, 4, 0, 16, 4, 8, 0, 0},
                   {CT_B, 4, 0,  8, 4, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgba4444be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_R, 2, 0, 12, 2, 4, 0, 0},
                   {CT_G, 2, 0,  8, 2, 4, 0, 0},
                   {CT_B, 2, 0,  4, 2, 4, 0, 0},
                   {CT_A, 2, 0,  0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgba4444le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_R, 2, 0, 12, 2, 4, 0, 0},
                   {CT_G, 2, 0,  8, 2, 4, 0, 0},
                   {CT_B, 2, 0,  4, 2, 4, 0, 0},
                   {CT_A, 2, 0,  0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb0555be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  6, 2, 5, 0, 0},
                   {CT_B, 2, 0,  1, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgb0555le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  6, 2, 5, 0, 0},
                   {CT_B, 2, 0,  1, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgba5551be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  6, 2, 5, 0, 0},
                   {CT_B, 2, 0,  1, 2, 5, 0, 0},
                   {CT_A, 2, 0,  0, 2, 1, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgba5551le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                   {CT_G, 2, 0,  6, 2, 5, 0, 0},
                   {CT_B, 2, 0,  1, 2, 5, 0, 0},
                   {CT_A, 2, 0,  0, 2, 1, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_rgba64be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{4, {{CT_R, 8, 0, 0, 2, 16, 0, 0},
                   {CT_G, 8, 2, 0, 2, 16, 0, 0},
                   {CT_B, 8, 4, 0, 2, 16, 0, 0},
                   {CT_A, 8, 6, 0, 2, 16, 0, 0}}, 64}
             }},
            {AkVideoCaps::Format_rgba64le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{4, {{CT_R, 8, 0, 0, 2, 16, 0, 0},
                   {CT_G, 8, 2, 0, 2, 16, 0, 0},
                   {CT_B, 8, 4, 0, 2, 16, 0, 0},
                   {CT_A, 8, 6, 0, 2, 16, 0, 0}}, 64}
             }},
            {AkVideoCaps::Format_rgbap,
             VFT_RGB,
             Q_BYTE_ORDER,
             4,
             {{1, {{CT_R, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_G, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_B, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_A, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_uyvy411,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_U, 4, 0, 0, 1, 8, 2, 0},
                   {CT_Y, 2, 1, 0, 1, 8, 0, 0},
                   {CT_V, 4, 2, 0, 1, 8, 2, 0}}, 12}
             }},
            {AkVideoCaps::Format_uyvy422,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_U, 4, 0, 0, 1, 8, 1, 0},
                   {CT_Y, 2, 1, 0, 1, 8, 0, 0},
                   {CT_V, 4, 2, 0, 1, 8, 1, 0}}, 16}
             }},
            {AkVideoCaps::Format_uyvy422a,
             VFT_YUV,
             Q_BYTE_ORDER,
             2,
             {{3, {{CT_U, 4, 0, 0, 1, 8, 1, 0},
                   {CT_Y, 2, 1, 0, 1, 8, 0, 0},
                   {CT_V, 4, 2, 0, 1, 8, 1, 0}}, 16},
              {1, {{CT_A, 1, 0, 0, 1, 8, 0, 0}},  8}
             }},
            {AkVideoCaps::Format_uyva,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{4, {{CT_U, 4, 0, 0, 1, 8, 0, 0},
                   {CT_Y, 4, 1, 0, 1, 8, 0, 0},
                   {CT_V, 4, 2, 0, 1, 8, 0, 0},
                   {CT_A, 4, 3, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_y210be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_V, 4, 0, 6, 2, 10, 0, 0},
                   {CT_U, 8, 2, 6, 2, 10, 1, 0},
                   {CT_Y, 8, 6, 6, 2, 10, 1, 0}}, 24}
             }},
            {AkVideoCaps::Format_y210le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_V, 4, 0, 6, 2, 10, 0, 0},
                   {CT_U, 8, 2, 6, 2, 10, 1, 0},
                   {CT_Y, 8, 6, 6, 2, 10, 1, 0}}, 24}
             }},
            {AkVideoCaps::Format_vuya,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{4, {{CT_V, 4, 0, 0, 1, 8, 0, 0},
                   {CT_U, 4, 1, 0, 1, 8, 0, 0},
                   {CT_Y, 4, 2, 0, 1, 8, 0, 0},
                   {CT_A, 4, 3, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_vuy0,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_V, 4, 0, 0, 1, 8, 0, 0},
                   {CT_U, 4, 1, 0, 1, 8, 0, 0},
                   {CT_Y, 4, 2, 0, 1, 8, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_vyuy422,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_V, 4, 0, 0, 1, 8, 1, 0},
                   {CT_Y, 2, 1, 0, 1, 8, 0, 0},
                   {CT_U, 4, 2, 0, 1, 8, 1, 0}}, 16}
             }},
            {AkVideoCaps::Format_bgr30be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_B, 4, 0, 20, 4, 10, 0, 0},
                   {CT_G, 4, 0, 10, 4, 10, 0, 0},
                   {CT_R, 4, 0,  0, 4, 10, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_bgr30le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_B, 4, 0, 20, 4, 10, 0, 0},
                   {CT_G, 4, 0, 10, 4, 10, 0, 0},
                   {CT_R, 4, 0,  0, 4, 10, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgb30be,
             VFT_RGB,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_R, 4, 0, 20, 4, 10, 0, 0},
                   {CT_G, 4, 0, 10, 4, 10, 0, 0},
                   {CT_B, 4, 0,  0, 4, 10, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_rgb30le,
             VFT_RGB,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_R, 4, 0, 20, 4, 10, 0, 0},
                   {CT_G, 4, 0, 10, 4, 10, 0, 0},
                   {CT_B, 4, 0,  0, 4, 10, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_yuv24,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_Y, 3, 0, 0, 1, 8, 0, 0},
                   {CT_U, 3, 1, 0, 1, 8, 0, 0},
                   {CT_V, 3, 2, 0, 1, 8, 0, 0}}, 24}
             }},
            {AkVideoCaps::Format_yuv30be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_Y, 4, 0, 20, 4, 10, 0, 0},
                   {CT_U, 4, 0, 10, 4, 10, 0, 0},
                   {CT_V, 4, 0,  0, 4, 10, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_yuv30le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_Y, 4, 0, 20, 4, 10, 0, 0},
                   {CT_U, 4, 0, 10, 4, 10, 0, 0},
                   {CT_V, 4, 0,  0, 4, 10, 0, 0}}, 32}
             }},
            {AkVideoCaps::Format_yuv410p,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_U, 1, 0, 0, 1, 8, 2, 2}}, 2},
              {1, {{CT_V, 1, 0, 0, 1, 8, 2, 2}}, 2}
             }},
            {AkVideoCaps::Format_yuv411p,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_U, 1, 0, 0, 1, 8, 2, 0}}, 2},
              {1, {{CT_V, 1, 0, 0, 1, 8, 2, 0}}, 2}
             }},
            {AkVideoCaps::Format_yuv420p,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_U, 1, 0, 0, 1, 8, 1, 1}}, 4},
              {1, {{CT_V, 1, 0, 0, 1, 8, 1, 1}}, 4}
             }},
            {AkVideoCaps::Format_yuv420p10be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 10, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv420p10le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 10, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv420p12be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 12, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv420p12le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 12, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv420p14be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 14, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 14, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv420p14le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 14, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 14, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv420p16be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 16, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv420p16le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 16, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv420p9be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 9, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv420p9le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 9, 1, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv422p,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_U, 1, 0, 0, 1, 8, 1, 0}}, 4},
              {1, {{CT_V, 1, 0, 0, 1, 8, 1, 0}}, 4}
             }},
            {AkVideoCaps::Format_yuv422p10be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 10, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv422p10le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 10, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv422p12be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 12, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv422p12le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 12, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv422p14be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 14, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 14, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv422p14le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 14, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 14, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv422p16be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 16, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv422p16le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 16, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv422p9be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 9, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv422p9le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 9, 1, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv440p,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_U, 1, 0, 0, 1, 8, 0, 1}}, 8},
              {1, {{CT_V, 1, 0, 0, 1, 8, 0, 1}}, 8}
             }},
            {AkVideoCaps::Format_yuv440p10be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 0, 1}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 10, 0, 1}}, 16}
             }},
            {AkVideoCaps::Format_yuv440p10le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 0, 1}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 10, 0, 1}}, 16}
             }},
            {AkVideoCaps::Format_yuv440p12be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 0, 1}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 12, 0, 1}}, 16}
             }},
            {AkVideoCaps::Format_yuv440p12le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 0, 1}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 12, 0, 1}}, 16}
             }},
            {AkVideoCaps::Format_yuv444,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_Y, 1, 0, 0, 1, 8, 0, 0},
                   {CT_U, 1, 1, 0, 1, 8, 0, 0},
                   {CT_V, 1, 2, 0, 1, 8, 0, 0}}, 24}
             }},
            {AkVideoCaps::Format_yuv444p,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_U, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_V, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuv444p10be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444p10le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444p12be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444p12le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444p14be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 14, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444p14le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 14, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 14, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444p16be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444p16le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444p9be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444p9le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             3,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444packbe,
             VFT_YUV,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_Y, 2, 0, 8, 2, 4, 0, 0},
                   {CT_U, 2, 0, 4, 2, 4, 0, 0},
                   {CT_V, 2, 0, 0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv444packle,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_Y, 2, 0, 8, 2, 4, 0, 0},
                   {CT_U, 2, 0, 4, 2, 4, 0, 0},
                   {CT_V, 2, 0, 0, 2, 4, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv555packbe,
             VFT_YUV,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_Y, 2, 0, 10, 2, 5, 0, 0},
                   {CT_U, 2, 0,  5, 2, 5, 0, 0},
                   {CT_V, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv555packle,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_Y, 2, 0, 10, 2, 5, 0, 0},
                   {CT_U, 2, 0,  5, 2, 5, 0, 0},
                   {CT_V, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv565packbe,
             VFT_YUV,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_Y, 2, 0, 11, 2, 5, 0, 0},
                   {CT_U, 2, 0,  5, 2, 6, 0, 0},
                   {CT_V, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuv565packle,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_Y, 2, 0, 11, 2, 5, 0, 0},
                   {CT_U, 2, 0,  5, 2, 6, 0, 0},
                   {CT_V, 2, 0,  0, 2, 5, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva420p,
             VFT_YUV,
             Q_BYTE_ORDER,
             4,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_U, 1, 0, 0, 1, 8, 1, 1}}, 4},
              {1, {{CT_V, 1, 0, 0, 1, 8, 1, 1}}, 4},
              {1, {{CT_A, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuva420p10be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 10, 1, 1}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva420p10le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 10, 1, 1}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva420p16be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 16, 1, 1}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva420p16le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 16, 1, 1}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva420p9be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 9, 1, 1}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva420p9le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 1, 1}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 9, 1, 1}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva422p,
             VFT_YUV,
             Q_BYTE_ORDER,
             4,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_U, 1, 0, 0, 1, 8, 1, 0}}, 4},
              {1, {{CT_V, 1, 0, 0, 1, 8, 1, 0}}, 4},
              {1, {{CT_A, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuva422p10be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 10, 1, 0}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva422p10le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 10, 1, 0}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva422p12be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 12, 1, 0}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva422p12le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 12, 1, 0}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva422p16be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 16, 1, 0}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva422p16le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 16, 1, 0}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva422p9be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 9, 1, 0}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva422p9le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 1, 0}}, 8},
              {1, {{CT_V, 2, 0, 0, 2, 9, 1, 0}}, 8},
              {1, {{CT_A, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva444p,
             VFT_YUV,
             Q_BYTE_ORDER,
             4,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_U, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_V, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_A, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuva444p10be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva444p10le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 10, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 10, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva444p12be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva444p12le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 12, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 12, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva444p16be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva444p16le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 16, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 16, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva444p9be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuva444p9le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             4,
             {{1, {{CT_Y, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_U, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_V, 2, 0, 0, 2, 9, 0, 0}}, 16},
              {1, {{CT_A, 2, 0, 0, 2, 9, 0, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuyv211,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_Y, 2, 0, 0, 1, 8, 1, 0},
                   {CT_U, 4, 1, 0, 1, 8, 2, 0},
                   {CT_V, 4, 3, 0, 1, 8, 2, 0}}, 8}
             }},
            {AkVideoCaps::Format_yuyv422,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_Y, 2, 0, 0, 1, 8, 0, 0},
                   {CT_U, 4, 1, 0, 1, 8, 1, 0},
                   {CT_V, 4, 3, 0, 1, 8, 1, 0}}, 16}
             }},
            {AkVideoCaps::Format_yuyv422_32be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_Y, 4, 0, 0, 2, 16, 0, 0},
                   {CT_U, 8, 2, 0, 2, 16, 1, 0},
                   {CT_V, 8, 6, 0, 2, 16, 1, 0}}, 32}
             }},
            {AkVideoCaps::Format_yuyv422_32le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_Y, 4, 0, 0, 2, 16, 0, 0},
                   {CT_U, 8, 2, 0, 2, 16, 1, 0},
                   {CT_V, 8, 6, 0, 2, 16, 1, 0}}, 32}
             }},
            {AkVideoCaps::Format_yuyv422_32_10be,
             VFT_YUV,
             Q_BIG_ENDIAN,
             1,
             {{3, {{CT_Y, 4, 0, 0, 2, 10, 0, 0},
                   {CT_U, 8, 2, 0, 2, 10, 1, 0},
                   {CT_V, 8, 6, 0, 2, 10, 1, 0}}, 32}
             }},
            {AkVideoCaps::Format_yuyv422_32_10le,
             VFT_YUV,
             Q_LITTLE_ENDIAN,
             1,
             {{3, {{CT_Y, 4, 0, 0, 2, 10, 0, 0},
                   {CT_U, 8, 2, 0, 2, 10, 1, 0},
                   {CT_V, 8, 6, 0, 2, 10, 1, 0}}, 32}
             }},
            {AkVideoCaps::Format_yvu410p,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_V, 1, 0, 0, 1, 8, 2, 2}}, 2},
              {1, {{CT_U, 1, 0, 0, 1, 8, 2, 2}}, 2}
             }},
            {AkVideoCaps::Format_yvu420p,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_V, 1, 0, 0, 1, 8, 1, 1}}, 4},
              {1, {{CT_U, 1, 0, 0, 1, 8, 1, 1}}, 4}
             }},
            {AkVideoCaps::Format_yvu422p,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_V, 1, 0, 0, 1, 8, 1, 0}}, 4},
              {1, {{CT_U, 1, 0, 0, 1, 8, 1, 0}}, 4}
             }},
            {AkVideoCaps::Format_yvu444p,
             VFT_YUV,
             Q_BYTE_ORDER,
             3,
             {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_V, 1, 0, 0, 1, 8, 0, 0}}, 8},
              {1, {{CT_U, 1, 0, 0, 1, 8, 0, 0}}, 8}
             }},
            {AkVideoCaps::Format_yvyu422,
             VFT_YUV,
             Q_BYTE_ORDER,
             1,
             {{3, {{CT_Y, 2, 0, 0, 1, 8, 0, 0},
                   {CT_V, 4, 1, 0, 1, 8, 1, 0},
                   {CT_U, 4, 3, 0, 1, 8, 1, 0}}, 16}
             }},
            {AkVideoCaps::Format_none,
             VFT_Unknown,
             Q_BYTE_ORDER,
             0,
             {}},
        };

        return akVideoFormatSpecTable;
    }

    static AkVideoFormatSpec formatSpecs(AkVideoCaps::PixelFormat format)
    {
        auto fmt = formats();

        for (; fmt->format != AkVideoCaps::Format_none; fmt++)
            if (fmt->format == format) {
                AkColorPlanes planes;

                for (size_t i = 0; i < fmt->nplanes; ++i) {
                    auto &plane = fmt->planes[i];
                    AkColorComponentList components;

                    for (size_t i = 0; i < plane.ncomponents; ++i) {
                        auto &component = plane.components[i];
                        components << AkColorComponent(component.type,
                                                       component.step,
                                                       component.offset,
                                                       component.shift,
                                                       component.byteLength,
                                                       component.length,
                                                       component.widthDiv,
                                                       component.heightDiv);
                    }

                    planes << AkColorPlane(components, plane.bitsSize);
                }

                return AkVideoFormatSpec(fmt->type,
                                         fmt->endianness,
                                         planes);
            }

        return {};
    }
};

class AkVideoCapsPrivate
{
    public:
        AkVideoCaps::PixelFormat m_format {AkVideoCaps::Format_none};
        int m_width {0};
        int m_height {0};
        AkFrac m_fps;
};

AkVideoCaps::AkVideoCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoCapsPrivate();
}

AkVideoCaps::AkVideoCaps(AkVideoCaps::PixelFormat format,
                         int width,
                         int height,
                         const AkFrac &fps):
    QObject()
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_format = format;
    this->d->m_width = width;
    this->d->m_height = height;
    this->d->m_fps = fps;
}

AkVideoCaps::AkVideoCaps(PixelFormat format,
                         const QSize &size,
                         const AkFrac &fps):
    QObject()
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_format = format;
    this->d->m_width = size.width();
    this->d->m_height = size.height();
    this->d->m_fps = fps;
}

AkVideoCaps::AkVideoCaps(const AkCaps &other):
    QObject()
{
    this->d = new AkVideoCapsPrivate();

    if (other.type() == AkCaps::CapsVideo) {
        auto data = reinterpret_cast<AkVideoCaps *>(other.privateData());
        this->d->m_format = data->d->m_format;
        this->d->m_width = data->d->m_width;
        this->d->m_height = data->d->m_height;
        this->d->m_fps = data->d->m_fps;
    }
}

AkVideoCaps::AkVideoCaps(const AkVideoCaps &other):
    QObject()
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_format = other.d->m_format;
    this->d->m_width = other.d->m_width;
    this->d->m_height = other.d->m_height;
    this->d->m_fps = other.d->m_fps;
}

AkVideoCaps::~AkVideoCaps()
{
    delete this->d;
}

AkVideoCaps &AkVideoCaps::operator =(const AkCaps &other)
{
    if (other.type() == AkCaps::CapsVideo) {
        auto data = reinterpret_cast<AkVideoCaps *>(other.privateData());
        this->d->m_format = data->d->m_format;
        this->d->m_width = data->d->m_width;
        this->d->m_height = data->d->m_height;
        this->d->m_fps = data->d->m_fps;
    } else {
        this->d->m_format = Format_none;
        this->d->m_width = 0;
        this->d->m_height = 0;
        this->d->m_fps = {};
    }

    return *this;
}

AkVideoCaps &AkVideoCaps::operator =(const AkVideoCaps &other)
{
    if (this != &other) {
        this->d->m_format = other.d->m_format;
        this->d->m_width = other.d->m_width;
        this->d->m_height = other.d->m_height;
        this->d->m_fps = other.d->m_fps;
    }

    return *this;
}

bool AkVideoCaps::operator ==(const AkVideoCaps &other) const
{
    return this->d->m_format == other.d->m_format
            && this->d->m_width == other.d->m_width
            && this->d->m_height == other.d->m_height
            && this->d->m_fps == other.d->m_fps;
}

bool AkVideoCaps::operator !=(const AkVideoCaps &other) const
{
    return !(*this == other);
}

AkVideoCaps::operator bool() const
{
    return this->d->m_format != AkVideoCaps::Format_none
           && this->d->m_width > 0
           && this->d->m_height > 0;
}

AkVideoCaps::operator AkCaps() const
{
    AkCaps caps;
    caps.setType(AkCaps::CapsVideo);
    caps.setPrivateData(new AkVideoCaps(*this),
                        [] (void *data) -> void * {
                            return new AkVideoCaps(*reinterpret_cast<AkVideoCaps *>(data));
                        },
                        [] (void *data) {
                            delete reinterpret_cast<AkVideoCaps *>(data);
                        });

    return caps;
}

QObject *AkVideoCaps::create()
{
    return new AkVideoCaps();
}

QObject *AkVideoCaps::create(const AkCaps &caps)
{
    return new AkVideoCaps(caps);
}

QObject *AkVideoCaps::create(const AkVideoCaps &caps)
{
    return new AkVideoCaps(caps);
}

QObject *AkVideoCaps::create(PixelFormat format,
                             int width,
                             int height,
                             const AkFrac &fps)
{
    return new AkVideoCaps(format, width, height, fps);
}

QObject *AkVideoCaps::create(PixelFormat format,
                             const QSize &size,
                             const AkFrac &fps)
{
    return new AkVideoCaps(format, size, fps);
}

QVariant AkVideoCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkVideoCaps::PixelFormat AkVideoCaps::format() const
{
    return this->d->m_format;
}

int AkVideoCaps::bpp() const
{
    return VideoFormat::formatSpecs(this->d->m_format).bpp();
}

QSize AkVideoCaps::size() const
{
    return {this->d->m_width, this->d->m_height};
}

int AkVideoCaps::width() const
{
    return this->d->m_width;
}

int AkVideoCaps::height() const
{
    return this->d->m_height;
}

AkFrac AkVideoCaps::fps() const
{
    return this->d->m_fps;
}

AkFrac &AkVideoCaps::fps()
{
    return this->d->m_fps;
}

AkVideoCaps AkVideoCaps::nearest(const AkVideoCapsList &caps) const
{
    AkVideoCaps nearestCap;
    auto q = std::numeric_limits<uint64_t>::max();
    auto sspecs = VideoFormat::formatSpecs(this->d->m_format);

    for (auto &cap: caps) {
        auto specs = VideoFormat::formatSpecs(cap.d->m_format);
        uint64_t diffFourcc = cap.d->m_format == this->d->m_format? 0: 1;
        auto diffWidth = cap.d->m_width - this->d->m_width;
        auto diffHeight = cap.d->m_height - this->d->m_height;
        auto diffBpp = specs.bpp() - sspecs.bpp();
        auto diffPlanes = specs.planes() - sspecs.planes();
        int diffPlanesBits = 0;

        if (specs.planes() != sspecs.planes()) {
            for (size_t j = 0; j < specs.planes(); ++j) {
                auto &plane = specs.plane(j);

                for (size_t i = 0; i < plane.components(); ++i)
                    diffPlanesBits += plane.component(i).length();
            }

            for (size_t j = 0; j < sspecs.planes(); ++j) {
                auto &plane = sspecs.plane(j);

                for (size_t i = 0; i < plane.components(); ++i)
                    diffPlanesBits -= plane.component(i).length();
            }
        }

        uint64_t k = diffFourcc
                   + uint64_t(diffWidth * diffWidth)
                   + uint64_t(diffHeight * diffHeight)
                   + diffBpp * diffBpp
                   + diffPlanes * diffPlanes
                   + diffPlanesBits * diffPlanesBits;

        if (k < q) {
            nearestCap = cap;
            q = k;
        }
    }

    return nearestCap;
}

bool AkVideoCaps::isSameFormat(const AkVideoCaps &other) const
{
    return this->d->m_format == other.d->m_format
            && this->d->m_width == other.d->m_width
            && this->d->m_height == other.d->m_height;
}

int AkVideoCaps::bitsPerPixel(AkVideoCaps::PixelFormat pixelFormat)
{
    return VideoFormat::formatSpecs(pixelFormat).bpp();
}

QString AkVideoCaps::pixelFormatToString(PixelFormat pixelFormat)
{
    AkVideoCaps caps;
    int formatIndex = caps.metaObject()->indexOfEnumerator("PixelFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    QString format(formatEnum.valueToKey(pixelFormat));
    format.remove("Format_");

    return format;
}

AkVideoFormatSpec AkVideoCaps::formatSpecs(PixelFormat pixelFormat)
{
    return VideoFormat::formatSpecs(pixelFormat);
}

void AkVideoCaps::setFormat(PixelFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    emit this->formatChanged(format);
}

void AkVideoCaps::setSize(const QSize &size)
{
    QSize curSize(this->d->m_width, this->d->m_height);

    if (curSize == size)
        return;

    this->d->m_width = size.width();
    this->d->m_height = size.height();
    emit this->widthChanged(size.width());
    emit this->heightChanged(size.height());
    emit sizeChanged(size);
}

void AkVideoCaps::setWidth(int width)
{
    if (this->d->m_width == width)
        return;

    this->d->m_width = width;
    emit this->widthChanged(width);
}

void AkVideoCaps::setHeight(int height)
{
    if (this->d->m_height == height)
        return;

    this->d->m_height = height;
    emit this->heightChanged(height);
}

void AkVideoCaps::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_fps = fps;
    emit this->fpsChanged(fps);
}

void AkVideoCaps::resetFormat()
{
    this->setFormat(AkVideoCaps::Format_none);
}

void AkVideoCaps::resetSize()
{
    this->setSize(QSize());
}

void AkVideoCaps::resetWidth()
{
    this->setWidth(0);
}

void AkVideoCaps::resetHeight()
{
    this->setHeight(0);
}

void AkVideoCaps::resetFps()
{
    this->setFps(AkFrac());
}

void AkVideoCaps::registerTypes()
{
    qRegisterMetaType<AkVideoCaps>("AkVideoCaps");
    qRegisterMetaType<AkVideoCapsList>("AkVideoCapsList");
    qRegisterMetaTypeStreamOperators<AkVideoCaps>("AkVideoCaps");
    qRegisterMetaType<PixelFormat>("PixelFormat");
    qRegisterMetaType<PixelFormatList>("PixelFormatList");
    QMetaType::registerDebugStreamOperator<AkVideoCaps::PixelFormat>();
    qmlRegisterSingletonType<AkVideoCaps>("Ak", 1, 0, "AkVideoCaps",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkVideoCaps();
    });
}

QDebug operator <<(QDebug debug, const AkVideoCaps &caps)
{
    debug.nospace() << "AkVideoCaps("
                    << "format="
                    << caps.format()
                    << ",width="
                    << caps.width()
                    << ",height="
                    << caps.height()
                    << ",fps="
                    << caps.fps()
                    << ")";

    return debug.space();
}

QDebug operator <<(QDebug debug, AkVideoCaps::PixelFormat format)
{
    debug.nospace() << AkVideoCaps::pixelFormatToString(format).toStdString().c_str();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkVideoCaps &caps)
{
    AkVideoCaps::PixelFormat format = AkVideoCaps::Format_none;
    istream >> format;
    caps.setFormat(format);
    int width = 0;
    istream >> width;
    caps.setWidth(width);
    int height = 0;
    istream >> height;
    caps.setHeight(height);
    AkFrac fps;
    istream >> fps;
    caps.setFps(fps);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkVideoCaps &caps)
{
    ostream << caps.format();
    ostream << caps.width();
    ostream << caps.height();
    ostream << caps.fps();

    return ostream;
}

#include "moc_akvideocaps.cpp"
