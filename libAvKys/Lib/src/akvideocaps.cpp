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

enum VideoFormatType
{
    VFT_Unknown,
    VFT_RGB,
    VFT_YUV,
    VFT_Gray
};

enum ComponentType
{
    CT_R,
    CT_G,
    CT_B,
    CT_Y,
    CT_U,
    CT_V,
    CT_A
};

class ColorComponent
{
    public:
        ComponentType type;
        size_t step;        // Bytes to increment for reading th next pixel.
        size_t offset;      // Bytes to skip before reading the component.
        size_t shift;       // Shift the value n-bits to the left before reading the component.
        size_t rlength;     // Read n-bytes for the value.
        size_t length;      // Size of the component in bits.
        size_t widthDiv;    // Plane width should be divided by 2^widthDiv
        size_t heightDiv;   // Plane height should be divided by 2^heightDiv

        bool operator ==(const ColorComponent &other) const
        {
            return this->type == other.type
                   && this->step == other.step
                   && this->offset == other.offset
                   && this->shift == other.shift
                   && this->rlength == other.rlength
                   && this->length == other.length
                   && this->widthDiv == other.widthDiv
                   && this->heightDiv == other.heightDiv;
        }
};

using ColorComponents = QVector<ColorComponent>;

class VideoFormat
{
    public:
        AkVideoCaps::PixelFormat format;
        QString formatStr;
        VideoFormatType type;
        int endianness;
        QVector<ColorComponents> planes;

        static inline const QVector<VideoFormat> &formats()
        {
            static const QVector<VideoFormat> videoFormats = {
                {AkVideoCaps::Format_none        , ""            , VFT_Unknown, Q_BYTE_ORDER   , {}},
                {AkVideoCaps::Format_0bgr        , "0bgr"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_B, 4, 1, 0, 1, 8, 0, 0},
                      {CT_G, 4, 2, 0, 1, 8, 0, 0},
                      {CT_R, 4, 3, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_0rgb        , "0rgb"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_R, 4, 1, 0, 1, 8, 0, 0},
                      {CT_G, 4, 2, 0, 1, 8, 0, 0},
                      {CT_B, 4, 3, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_abgr        , "abgr"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_A, 4, 0, 0, 1, 8, 0, 0},
                      {CT_B, 4, 1, 0, 1, 8, 0, 0},
                      {CT_G, 4, 2, 0, 1, 8, 0, 0},
                      {CT_R, 4, 3, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_argb        , "argb"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_A, 4, 0, 0, 1, 8, 0, 0},
                      {CT_R, 4, 1, 0, 1, 8, 0, 0},
                      {CT_G, 4, 2, 0, 1, 8, 0, 0},
                      {CT_B, 4, 3, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_argb1555le  , "argb1555le"  , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_A, 2, 0, 15, 2, 1, 0, 0},
                      {CT_R, 2, 0, 10, 2, 5, 0, 0},
                      {CT_G, 2, 0,  5, 2, 5, 0, 0},
                      {CT_B, 2, 0,  0, 2, 5, 0, 0}}
                 }},
                {AkVideoCaps::Format_argb4444le  , "argb4444le"  , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_A, 2, 0, 12, 2, 4, 0, 0},
                      {CT_R, 2, 0,  8, 2, 4, 0, 0},
                      {CT_G, 2, 0,  4, 2, 4, 0, 0},
                      {CT_B, 2, 0,  0, 2, 4, 0, 0}}
                 }},
                {AkVideoCaps::Format_ayuv64le    , "ayuv64le"    , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_A, 8, 0, 0, 2, 16, 0, 0},
                      {CT_Y, 8, 2, 0, 2, 16, 0, 0},
                      {CT_U, 8, 4, 0, 2, 16, 0, 0},
                      {CT_V, 8, 6, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr0        , "bgr0"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_B, 4, 0, 0, 1, 8, 0, 0},
                      {CT_G, 4, 1, 0, 1, 8, 0, 0},
                      {CT_R, 4, 2, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr24       , "bgr24"       , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_B, 3, 0, 0, 1, 8, 0, 0},
                      {CT_G, 3, 1, 0, 1, 8, 0, 0},
                      {CT_R, 3, 2, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr444be    , "bgr444be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_B, 2, 0, 8, 2, 4, 0, 0},
                      {CT_G, 2, 0, 4, 2, 4, 0, 0},
                      {CT_R, 2, 0, 0, 2, 4, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr444le    , "bgr444le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_B, 2, 0, 8, 2, 4, 0, 0},
                      {CT_G, 2, 0, 4, 2, 4, 0, 0},
                      {CT_R, 2, 0, 0, 2, 4, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr48be     , "bgr48be"     , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_B, 6, 0, 0, 2, 16, 0, 0},
                      {CT_G, 6, 2, 0, 2, 16, 0, 0},
                      {CT_R, 6, 4, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr48le     , "bgr48le"     , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_B, 6, 0, 0, 2, 16, 0, 0},
                      {CT_G, 6, 2, 0, 2, 16, 0, 0},
                      {CT_R, 6, 4, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr555be    , "bgr555be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_B, 2, 0, 10, 2, 5, 0, 0},
                      {CT_G, 2, 0,  5, 2, 5, 0, 0},
                      {CT_R, 2, 0,  0, 2, 5, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr555le    , "bgr555le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_B, 2, 0, 10, 2, 5, 0, 0},
                      {CT_G, 2, 0,  5, 2, 5, 0, 0},
                      {CT_R, 2, 0,  0, 2, 5, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr565be    , "bgr565be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_B, 2, 0, 11, 2, 5, 0, 0},
                      {CT_G, 2, 0,  5, 2, 6, 0, 0},
                      {CT_R, 2, 0,  0, 2, 5, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr565le    , "bgr565le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_B, 2, 0, 11, 2, 5, 0, 0},
                      {CT_G, 2, 0,  5, 2, 6, 0, 0},
                      {CT_R, 2, 0,  0, 2, 5, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgr8        , "bgr8"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_B, 1, 0, 5, 1, 3, 0, 0},
                      {CT_G, 1, 0, 2, 1, 3, 0, 0},
                      {CT_R, 1, 0, 0, 1, 2, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgra        , "bgra"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_B, 4, 0, 0, 1, 8, 0, 0},
                      {CT_G, 4, 1, 0, 1, 8, 0, 0},
                      {CT_R, 4, 2, 0, 1, 8, 0, 0},
                      {CT_A, 4, 3, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgra64be    , "bgra64be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_B, 8, 0, 0, 2, 16, 0, 0},
                      {CT_G, 8, 2, 0, 2, 16, 0, 0},
                      {CT_R, 8, 4, 0, 2, 16, 0, 0},
                      {CT_A, 8, 6, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_bgra64le    , "bgra64le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_B, 8, 0, 0, 2, 16, 0, 0},
                      {CT_G, 8, 2, 0, 2, 16, 0, 0},
                      {CT_R, 8, 4, 0, 2, 16, 0, 0},
                      {CT_A, 8, 6, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrap       , "gbrap"       , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_G, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_B, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_R, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_A, 1, 0, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrap10be   , "gbrap10be"   , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_G, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrap10le   , "gbrap10le"   , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_G, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrap12be   , "gbrap12be"   , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_G, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 12, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrap12le   , "gbrap12le"   , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_G, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 12, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrap16be   , "gbrap16be"   , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_G, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrap16le   , "gbrap16le"   , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_G, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp        , "gbrp"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_G, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_B, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_R, 1, 0, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp10be    , "gbrp10be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_G, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp10le    , "gbrp10le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_G, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp12be    , "gbrp12be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_G, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 12, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp12le    , "gbrp12le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_G, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 12, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp14be    , "gbrp14be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_G, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 14, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp14le    , "gbrp14le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_G, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 14, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp16be    , "gbrp16be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_G, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp16le    , "gbrp16le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_G, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp9be     , "gbrp9be"     , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_G, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 9, 0, 0}}
                 }},
                {AkVideoCaps::Format_gbrp9le     , "gbrp9le"     , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_G, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_B, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_R, 2, 0, 0, 2, 9, 0, 0}}
                 }},
                {AkVideoCaps::Format_gray16be    , "gray16be"    , VFT_Gray   , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_gray16le    , "gray16le"    , VFT_Gray   , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_gray32be    , "gray32be"    , VFT_Gray   , Q_BIG_ENDIAN   , {
                     {{CT_Y, 4, 0, 0, 4, 32, 0, 0}}
                 }},
                {AkVideoCaps::Format_gray32le    , "gray32le"    , VFT_Gray   , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 4, 0, 0, 4, 32, 0, 0}}
                 }},
                {AkVideoCaps::Format_gray8       , "gray8"       , VFT_Gray   , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_graya16be   , "graya16be"   , VFT_Gray   , Q_BIG_ENDIAN   , {
                     {{CT_Y, 4, 0, 0, 2, 16, 0, 0},
                      {CT_A, 4, 2, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_graya16le   , "graya16le"   , VFT_Gray   , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 4, 0, 0, 2, 16, 0, 0},
                      {CT_A, 4, 2, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_graya8      , "graya8"      , VFT_Gray   , Q_BYTE_ORDER   , {
                     {{CT_Y, 2, 0, 0, 1, 8, 0, 0},
                      {CT_A, 2, 1, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_nv12        , "nv12"        , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 2, 0, 0, 1, 8, 1, 1},
                      {CT_V, 2, 1, 0, 1, 8, 1, 1}}
                 }},
                {AkVideoCaps::Format_nv16        , "nv16"        , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 2, 0, 0, 1, 8, 1, 0},
                      {CT_V, 2, 1, 0, 1, 8, 1, 0}}
                 }},
                {AkVideoCaps::Format_nv21        , "nv21"        , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_V, 2, 0, 0, 1, 8, 1, 1},
                      {CT_U, 2, 1, 0, 1, 8, 1, 1}}
                 }},
                {AkVideoCaps::Format_nv24        , "nv24"        , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 2, 0, 0, 1, 8, 0, 0},
                      {CT_V, 2, 1, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_nv42        , "nv42"        , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_V, 2, 0, 0, 1, 8, 0, 0},
                      {CT_U, 2, 1, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_p010be      , "p010be"      , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 6, 2, 10, 0, 0}},
                     {{CT_U, 4, 0, 6, 2, 10, 1, 1},
                      {CT_V, 4, 2, 6, 2, 10, 1, 1}}
                 }},
                {AkVideoCaps::Format_p010le      , "p010le"      , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 6, 2, 10, 0, 0}},
                     {{CT_U, 4, 0, 6, 2, 10, 1, 1},
                      {CT_V, 4, 2, 6, 2, 10, 1, 1}}
                 }},
                {AkVideoCaps::Format_p016be      , "p016be"      , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 4, 0, 0, 2, 16, 1, 1},
                      {CT_V, 4, 2, 0, 2, 16, 1, 1}}
                 }},
                {AkVideoCaps::Format_p016le      , "p016le"      , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 4, 0, 0, 2, 16, 1, 1},
                      {CT_V, 4, 2, 0, 2, 16, 1, 1}}
                 }},
                {AkVideoCaps::Format_p210be      , "p210be"      , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 2, 0, 6, 2, 10, 0, 0}},
                     {{CT_U, 4, 0, 6, 2, 10, 1, 0},
                      {CT_V, 4, 2, 6, 2, 10, 1, 0}}
                 }},
                {AkVideoCaps::Format_p210le      , "p210le"      , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 6, 2, 10, 0, 0}},
                     {{CT_U, 4, 0, 6, 2, 10, 1, 0},
                      {CT_V, 4, 2, 6, 2, 10, 1, 0}}
                 }},
                {AkVideoCaps::Format_p216be      , "p216be"      , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 4, 0, 0, 2, 16, 1, 0},
                      {CT_V, 4, 2, 0, 2, 16, 1, 0}}
                 }},
                {AkVideoCaps::Format_p216le      , "p216le"      , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 4, 0, 0, 2, 16, 1, 0},
                      {CT_V, 4, 2, 0, 2, 16, 1, 0}}
                 }},
                {AkVideoCaps::Format_p410be      , "p410be"      , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 2, 0, 6, 2, 10, 0, 0}},
                     {{CT_U, 4, 0, 6, 2, 10, 0, 0},
                      {CT_V, 4, 2, 6, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_p410le      , "p410le"      , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 6, 2, 10, 0, 0}},
                     {{CT_U, 4, 0, 6, 2, 10, 0, 0},
                      {CT_V, 4, 2, 6, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_p416be      , "p416be"      , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 4, 0, 0, 2, 16, 0, 0},
                      {CT_V, 4, 2, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_p416le      , "p416le"      , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 4, 0, 0, 2, 16, 0, 0},
                      {CT_V, 4, 2, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb0        , "rgb0"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_R, 4, 0, 0, 1, 8, 0, 0},
                      {CT_G, 4, 1, 0, 1, 8, 0, 0},
                      {CT_B, 4, 2, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb24       , "rgb24"       , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_R, 3, 0, 0, 1, 8, 0, 0},
                      {CT_G, 3, 1, 0, 1, 8, 0, 0},
                      {CT_B, 3, 2, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb24p      , "rgb24p"      , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_R, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_G, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_B, 1, 0, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb444be    , "rgb444be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_R, 2, 0, 8, 2, 4, 0, 0},
                      {CT_G, 2, 0, 4, 2, 4, 0, 0},
                      {CT_B, 2, 0, 0, 2, 4, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb444le    , "rgb444le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_R, 2, 0, 8, 2, 4, 0, 0},
                      {CT_G, 2, 0, 4, 2, 4, 0, 0},
                      {CT_B, 2, 0, 0, 2, 4, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb48be     , "rgb48be"     , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_R, 6, 0, 0, 2, 16, 0, 0},
                      {CT_G, 6, 2, 0, 2, 16, 0, 0},
                      {CT_B, 6, 4, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb48le     , "rgb48le"     , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_R, 6, 0, 0, 2, 16, 0, 0},
                      {CT_G, 6, 2, 0, 2, 16, 0, 0},
                      {CT_B, 6, 4, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb555be    , "rgb555be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_R, 2, 0, 10, 2, 5, 0, 0},
                      {CT_G, 2, 0,  5, 2, 5, 0, 0},
                      {CT_B, 2, 0,  0, 2, 5, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb555le    , "rgb555le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_R, 2, 0, 10, 2, 5, 0, 0},
                      {CT_G, 2, 0,  5, 2, 5, 0, 0},
                      {CT_B, 2, 0,  0, 2, 5, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb565be    , "rgb565be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                      {CT_G, 2, 0,  5, 2, 6, 0, 0},
                      {CT_B, 2, 0,  0, 2, 5, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb565le    , "rgb565le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                      {CT_G, 2, 0,  5, 2, 6, 0, 0},
                      {CT_B, 2, 0,  0, 2, 5, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgb8        , "rgb8"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_R, 1, 0, 6, 1, 2, 0, 0},
                      {CT_G, 1, 0, 3, 1, 3, 0, 0},
                      {CT_B, 1, 0, 0, 1, 3, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgba        , "rgba"        , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_R, 4, 0, 0, 1, 8, 0, 0},
                      {CT_G, 4, 1, 0, 1, 8, 0, 0},
                      {CT_B, 4, 2, 0, 1, 8, 0, 0},
                      {CT_A, 4, 3, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgba64be    , "rgba64be"    , VFT_RGB    , Q_BIG_ENDIAN   , {
                     {{CT_R, 8, 0, 0, 2, 16, 0, 0},
                      {CT_G, 8, 2, 0, 2, 16, 0, 0},
                      {CT_B, 8, 4, 0, 2, 16, 0, 0},
                      {CT_A, 8, 6, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgba64le    , "rgba64le"    , VFT_RGB    , Q_LITTLE_ENDIAN, {
                     {{CT_R, 8, 0, 0, 2, 16, 0, 0},
                      {CT_G, 8, 2, 0, 2, 16, 0, 0},
                      {CT_B, 8, 4, 0, 2, 16, 0, 0},
                      {CT_A, 8, 6, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_rgbap       , "rgbap"       , VFT_RGB    , Q_BYTE_ORDER   , {
                     {{CT_R, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_G, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_B, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_A, 1, 0, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_uyvy422     , "uyvy422"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_U, 4, 0, 0, 1, 8, 1, 0},
                      {CT_Y, 2, 1, 0, 1, 8, 0, 0},
                      {CT_V, 4, 2, 0, 1, 8, 1, 0}}
                 }},
                {AkVideoCaps::Format_vyuy422     , "vyuy422"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_V, 4, 0, 0, 1, 8, 1, 0},
                      {CT_Y, 2, 1, 0, 1, 8, 0, 0},
                      {CT_U, 4, 2, 0, 1, 8, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv410p     , "yuv410p"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 1, 0, 0, 1, 8, 2, 2}},
                     {{CT_V, 1, 0, 0, 1, 8, 2, 2}}
                 }},
                {AkVideoCaps::Format_yuv411p     , "yuv411p"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 1, 0, 0, 1, 8, 2, 0}},
                     {{CT_V, 1, 0, 0, 1, 8, 2, 0}}
                 }},
                {AkVideoCaps::Format_yuv420p     , "yuv420p"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 1, 0, 0, 1, 8, 1, 1}},
                     {{CT_V, 1, 0, 0, 1, 8, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv420p10be , "yuv420p10be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 10, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv420p10le , "yuv420p10le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 10, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv420p12be , "yuv420p12be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 12, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv420p12le , "yuv420p12le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 12, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv420p14be , "yuv420p14be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 14, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 14, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv420p14le , "yuv420p14le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 14, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 14, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv420p16be , "yuv420p16be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 16, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv420p16le , "yuv420p16le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 16, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv420p9be  , "yuv420p9be"  , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 9, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv420p9le  , "yuv420p9le"  , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 9, 1, 1}}
                 }},
                {AkVideoCaps::Format_yuv422p     , "yuv422p"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 1, 0, 0, 1, 8, 1, 0}},
                     {{CT_V, 1, 0, 0, 1, 8, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv422p10be , "yuv422p10be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 10, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv422p10le , "yuv422p10le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 10, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv422p12be , "yuv422p12be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 12, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv422p12le , "yuv422p12le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 12, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv422p14be , "yuv422p14be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 14, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 14, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv422p14le , "yuv422p14le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 14, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 14, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv422p16be , "yuv422p16be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 16, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv422p16le , "yuv422p16le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 16, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv422p9be  , "yuv422p9be"  , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 9, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv422p9le  , "yuv422p9le"  , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 9, 1, 0}}
                 }},
                {AkVideoCaps::Format_yuv440p     , "yuv440p"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 1, 0, 0, 1, 8, 0, 1}},
                     {{CT_V, 1, 0, 0, 1, 8, 0, 1}}
                 }},
                {AkVideoCaps::Format_yuv440p10be , "yuv440p10be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 0, 1}},
                     {{CT_V, 2, 0, 0, 2, 10, 0, 1}}
                 }},
                {AkVideoCaps::Format_yuv440p10le , "yuv440p10le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 0, 1}},
                     {{CT_V, 2, 0, 0, 2, 10, 0, 1}}
                 }},
                {AkVideoCaps::Format_yuv440p12be , "yuv440p12be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 0, 1}},
                     {{CT_V, 2, 0, 0, 2, 12, 0, 1}}
                 }},
                {AkVideoCaps::Format_yuv440p12le , "yuv440p12le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 0, 1}},
                     {{CT_V, 2, 0, 0, 2, 12, 0, 1}}
                 }},
                {AkVideoCaps::Format_yuv444      , "yuv444"      , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0},
                      {CT_U, 1, 1, 0, 1, 8, 0, 0},
                      {CT_V, 1, 2, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p     , "yuv444p"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_V, 1, 0, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p10be , "yuv444p10be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p10le , "yuv444p10le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p12be , "yuv444p12be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 12, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p12le , "yuv444p12le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 12, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p14be , "yuv444p14be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 14, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p14le , "yuv444p14le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 14, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 14, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p16be , "yuv444p16be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p16le , "yuv444p16le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p9be  , "yuv444p9be"  , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 9, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuv444p9le  , "yuv444p9le"  , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 9, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva420p    , "yuva420p"    , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 1, 0, 0, 1, 8, 1, 1}},
                     {{CT_V, 1, 0, 0, 1, 8, 1, 1}},
                     {{CT_A, 1, 0, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva420p10be, "yuva420p10be", VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 10, 1, 1}},
                     {{CT_A, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva420p10le, "yuva420p10le", VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 10, 1, 1}},
                     {{CT_A, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva420p16be, "yuva420p16be", VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 16, 1, 1}},
                     {{CT_A, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva420p16le, "yuva420p16le", VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 16, 1, 1}},
                     {{CT_A, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva420p9be , "yuva420p9be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 9, 1, 1}},
                     {{CT_A, 2, 0, 0, 2, 9, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva420p9le , "yuva420p9le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 1, 1}},
                     {{CT_V, 2, 0, 0, 2, 9, 1, 1}},
                     {{CT_A, 2, 0, 0, 2, 9, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva422p    , "yuva422p"    , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 1, 0, 0, 1, 8, 1, 0}},
                     {{CT_V, 1, 0, 0, 1, 8, 1, 0}},
                     {{CT_A, 1, 0, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva422p10be, "yuva422p10be", VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 10, 1, 0}},
                     {{CT_A, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva422p10le, "yuva422p10le", VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 10, 1, 0}},
                     {{CT_A, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva422p12be, "yuva422p12be", VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 12, 1, 0}},
                     {{CT_A, 2, 0, 0, 2, 12, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva422p12le, "yuva422p12le", VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 12, 1, 0}},
                     {{CT_A, 2, 0, 0, 2, 12, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva422p16be, "yuva422p16be", VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 16, 1, 0}},
                     {{CT_A, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva422p16le, "yuva422p16le", VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 16, 1, 0}},
                     {{CT_A, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva422p9be , "yuva422p9be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 9, 1, 0}},
                     {{CT_A, 2, 0, 0, 2, 9, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva422p9le , "yuva422p9le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 1, 0}},
                     {{CT_V, 2, 0, 0, 2, 9, 1, 0}},
                     {{CT_A, 2, 0, 0, 2, 9, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva444p    , "yuva444p"    , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_U, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_V, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_A, 1, 0, 0, 1, 8, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva444p10be, "yuva444p10be", VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva444p10le, "yuva444p10le", VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 10, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 10, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva444p12be, "yuva444p12be", VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 12, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva444p12le, "yuva444p12le", VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 12, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 12, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva444p16be, "yuva444p16be", VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva444p16le, "yuva444p16le", VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 16, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 16, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva444p9be , "yuva444p9be" , VFT_YUV    , Q_BIG_ENDIAN   , {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 9, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuva444p9le , "yuva444p9le" , VFT_YUV    , Q_LITTLE_ENDIAN, {
                     {{CT_Y, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_U, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_V, 2, 0, 0, 2, 9, 0, 0}},
                     {{CT_A, 2, 0, 0, 2, 9, 0, 0}}
                 }},
                {AkVideoCaps::Format_yuyv422     , "yuyv422"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 2, 0, 0, 1, 8, 0, 0},
                      {CT_U, 4, 1, 0, 1, 8, 1, 0},
                      {CT_V, 4, 3, 0, 1, 8, 1, 0}}
                 }},
                {AkVideoCaps::Format_yvu420p     , "yvu420p"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 1, 0, 0, 1, 8, 0, 0}},
                     {{CT_V, 1, 0, 0, 1, 8, 1, 1}},
                     {{CT_U, 1, 0, 0, 1, 8, 1, 1}}
                 }},
                {AkVideoCaps::Format_yvyu422     , "yvyu422"     , VFT_YUV    , Q_BYTE_ORDER   , {
                     {{CT_Y, 2, 0, 0, 1, 8, 0, 0},
                      {CT_V, 4, 1, 0, 1, 8, 1, 0},
                      {CT_U, 4, 3, 0, 1, 8, 1, 0}}
                 }},
            };

            return videoFormats;
        }

        static inline const VideoFormat *byFormat(AkVideoCaps::PixelFormat format)
        {
            for (auto &format_: formats())
                if (format_.format == format)
                    return &format_;

            return &formats().front();
        }

        static inline const VideoFormat *byFormatStr(const QString &format)
        {
            for (auto &format_: formats())
                if (format_.formatStr == format)
                    return &format_;

            return &formats().front();
        }

        inline int bpp() const
        {
            static const int k = 16;
            int bpp = 0;

            for (auto &plane: planes)
                for (auto &component: plane)
                    bpp += k * component.length
                           / (1 << (component.widthDiv + component.heightDiv));

            return bpp / k;
        }
};

class AkVideoCapsPrivate
{
    public:
        AkVideoCaps::PixelFormat m_format {AkVideoCaps::Format_none};
        int m_width {0};
        int m_height {0};
        int m_align {1};
        AkFrac m_fps;
        QVector<size_t> m_bypl;
        QVector<size_t> m_planeSize;
        QVector<size_t> m_offset;

        void updateParams();
        template<typename T>
        static inline T alignUp(const T &value, const T &align)
        {
            return (value + align - 1) & ~(align - 1);
        }
};

AkVideoCaps::AkVideoCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoCapsPrivate();
}

AkVideoCaps::AkVideoCaps(AkVideoCaps::PixelFormat format,
                         int width,
                         int height,
                         const AkFrac &fps,
                         int align)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_format = format;
    this->d->m_width = width;
    this->d->m_height = height;
    this->d->m_fps = fps;
    this->d->m_align = align;
    this->d->updateParams();
}

AkVideoCaps::AkVideoCaps(AkVideoCaps::PixelFormat format,
                         const QSize &size,
                         const AkFrac &fps,
                         int align)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_format = format;
    this->d->m_width = size.width();
    this->d->m_height = size.height();
    this->d->m_fps = fps;
    this->d->m_align = align;
    this->d->updateParams();
}


AkVideoCaps::AkVideoCaps(const AkCaps &caps)
{
    this->d = new AkVideoCapsPrivate();

    if (caps.mimeType() == "video/x-raw")
        this->update(caps);
}

AkVideoCaps::AkVideoCaps(const AkVideoCaps &other):
    QObject()
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_format = other.d->m_format;
    this->d->m_width = other.d->m_width;
    this->d->m_height = other.d->m_height;
    this->d->m_fps = other.d->m_fps;
    this->d->m_align = other.d->m_align;
    this->d->m_bypl = other.d->m_bypl;
    this->d->m_planeSize = other.d->m_planeSize;
    this->d->m_offset = other.d->m_offset;
    auto properties = other.dynamicPropertyNames();

    for (auto &property: properties)
        this->setProperty(property, other.property(property));
}

AkVideoCaps::~AkVideoCaps()
{
    delete this->d;
}

AkVideoCaps &AkVideoCaps::operator =(const AkVideoCaps &other)
{
    if (this != &other) {
        this->d->m_format = other.d->m_format;
        this->d->m_width = other.d->m_width;
        this->d->m_height = other.d->m_height;
        this->d->m_fps = other.d->m_fps;
        this->d->m_align = other.d->m_align;
        this->d->m_bypl = other.d->m_bypl;
        this->d->m_planeSize = other.d->m_planeSize;
        this->d->m_offset = other.d->m_offset;

        this->clear();
        auto properties = other.dynamicPropertyNames();

        for (auto &property: properties)
            this->setProperty(property, other.property(property));

        this->d->updateParams();
    }

    return *this;
}

AkVideoCaps &AkVideoCaps::operator =(const AkCaps &caps)
{
    if (caps.mimeType() == "video/x-raw") {
        this->update(caps);
    } else {
        this->d->m_format = AkVideoCaps::Format_none;
        this->d->m_width = 0;
        this->d->m_height = 0;
        this->d->m_fps = AkFrac();
        this->d->m_align = 1;
    }

    return *this;
}

bool AkVideoCaps::operator ==(const AkVideoCaps &other) const
{
    if (this->dynamicPropertyNames() != other.dynamicPropertyNames())
        return false;

    for (auto &property: this->dynamicPropertyNames())
        if (this->property(property) != other.property(property))
            return false;

    return this->d->m_format == other.d->m_format
            && this->d->m_width == other.d->m_width
            && this->d->m_height == other.d->m_height
            && this->d->m_fps == other.d->m_fps
            && this->d->m_align == other.d->m_align;
}

bool AkVideoCaps::operator !=(const AkVideoCaps &other) const
{
    return !(*this == other);
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

QObject *AkVideoCaps::create(AkVideoCaps::PixelFormat format,
                             int width,
                             int height,
                             const AkFrac &fps,
                             int align)
{
    return new AkVideoCaps(format, width, height, fps, align);
}

QObject *AkVideoCaps::create(const QString &format,
                             int width,
                             int height,
                             const AkFrac &fps,
                             int align)
{
    return new AkVideoCaps(AkVideoCaps::pixelFormatFromString(format),
                           width,
                           height,
                           fps,
                           align);
}

QObject *AkVideoCaps::create(AkVideoCaps::PixelFormat format,
                             const QSize &size,
                             const AkFrac &fps,
                             int align)
{
    return new AkVideoCaps(format, size, fps, align);
}

QObject *AkVideoCaps::create(const QString &format,
                             const QSize &size,
                             const AkFrac &fps,
                             int align)
{
    return new AkVideoCaps(AkVideoCaps::pixelFormatFromString(format),
                           size,
                           fps,
                           align);
}

QVariant AkVideoCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkVideoCaps::operator AkCaps() const
{
    AkCaps caps("video/x-raw");
    caps.setProperty("format", this->d->m_format);
    caps.setProperty("width" , this->d->m_width);
    caps.setProperty("height", this->d->m_height);
    caps.setProperty("fps"   , QVariant::fromValue(this->d->m_fps));
    caps.setProperty("align" , this->d->m_align);

    return caps;
}

AkVideoCaps::operator bool() const
{
    return this->pictureSize() > 0;
}

AkVideoCaps::PixelFormat AkVideoCaps::format() const
{
    return this->d->m_format;
}

int AkVideoCaps::bpp() const
{
    return VideoFormat::byFormat(this->d->m_format)->bpp();
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

int AkVideoCaps::align() const
{
    return this->d->m_align;
}

size_t AkVideoCaps::pictureSize() const
{
    auto vf = VideoFormat::byFormat(this->d->m_format);

    if (!vf)
        return 0;

    size_t size = 0;

    for (int i = 0; i < vf->planes.size(); i++)
        size += this->planeSize(i);

    return size;
}

AkVideoCaps AkVideoCaps::fromMap(const QVariantMap &caps)
{
    AkVideoCaps videoCaps;

    if (!caps.contains("mimeType") || caps["mimeType"] != "video/x-raw")
        return videoCaps;

    for (auto it = caps.begin(); it != caps.end(); it++) {
        auto value = it.value();

        if (it.key() == "mimeType")
            continue;

        videoCaps.setProperty(it.key().toStdString().c_str(), value);
    }

    return videoCaps;
}

QVariantMap AkVideoCaps::toMap() const
{
    QVariantMap map {
        {"mimeType", "video/x-raw"                      },
        {"format"  , this->d->m_format                  },
        {"width"   , this->d->m_width                   },
        {"height"  , this->d->m_height                  },
        {"fps"     , QVariant::fromValue(this->d->m_fps)},
        {"align"   , this->d->m_align                   },
    };

    for (auto &property: this->dynamicPropertyNames()) {
        auto key = QString::fromUtf8(property.constData());
        map[key] = this->property(property);
    }

    return map;
}

AkVideoCaps &AkVideoCaps::update(const AkCaps &caps)
{
    if (caps.mimeType() != "video/x-raw")
        return *this;

    this->clear();

    for (auto &property: caps.dynamicPropertyNames()) {
        int i = this->metaObject()->indexOfProperty(property);

        if (this->metaObject()->property(i).isWritable())
            this->setProperty(property, caps.property(property));
    }

    this->d->updateParams();

    return *this;
}

size_t AkVideoCaps::planeOffset(int plane) const
{
    return this->d->m_offset.value(plane, 0);
}

size_t AkVideoCaps::lineOffset(int plane, int y) const
{
    return this->d->m_offset.value(plane, 0)
            + this->d->m_planeSize.value(plane, 0)
            * size_t(y) / this->d->m_height;
}

size_t AkVideoCaps::bytesPerLine(int plane) const
{
    return this->d->m_bypl.value(plane, 0);
}

int AkVideoCaps::planes() const
{
    auto vf = VideoFormat::byFormat(this->d->m_format);

    return vf? vf->planes.size(): 0;
}

size_t AkVideoCaps::planeSize(int plane) const
{
    return this->d->m_planeSize.value(plane, 0);
}

AkVideoCaps AkVideoCaps::nearest(const AkVideoCapsList &caps) const
{
    AkVideoCaps nearestCap;
    auto q = std::numeric_limits<uint64_t>::max();
    auto svf = VideoFormat::byFormat(this->d->m_format);

    for (auto &cap: caps) {
        auto vf = VideoFormat::byFormat(cap.d->m_format);
        uint64_t diffFourcc = cap.d->m_format == this->d->m_format? 0: 1;
        auto diffWidth = cap.d->m_width - this->d->m_width;
        auto diffHeight = cap.d->m_height - this->d->m_height;
        auto diffBpp = vf->bpp() - svf->bpp();
        auto diffPlanes = vf->planes.size() - svf->planes.size();
        int diffPlanesBits = 0;

        if (vf->planes != svf->planes) {
            for (auto &plane: vf->planes)
                for (auto &component: plane)
                    diffPlanesBits += component.length;

            for (auto &plane: svf->planes)
                for (auto &component: plane)
                    diffPlanesBits -= component.length;
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

int AkVideoCaps::bitsPerPixel(AkVideoCaps::PixelFormat pixelFormat)
{
    return VideoFormat::byFormat(pixelFormat)->bpp();
}

int AkVideoCaps::bitsPerPixel(const QString &pixelFormat)
{
    return AkVideoCaps::bitsPerPixel(AkVideoCaps::pixelFormatFromString(pixelFormat));
}

QString AkVideoCaps::pixelFormatToString(AkVideoCaps::PixelFormat pixelFormat)
{
    AkVideoCaps caps;
    int formatIndex = caps.metaObject()->indexOfEnumerator("PixelFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    QString format(formatEnum.valueToKey(pixelFormat));
    format.remove("Format_");

    return format;
}

AkVideoCaps::PixelFormat AkVideoCaps::pixelFormatFromString(const QString &pixelFormat)
{
    AkVideoCaps caps;
    QString format = "Format_" + pixelFormat;
    int enumIndex = caps.metaObject()->indexOfEnumerator("PixelFormat");
    QMetaEnum enumType = caps.metaObject()->enumerator(enumIndex);
    int enumValue = enumType.keyToValue(format.toStdString().c_str());

    return static_cast<PixelFormat>(enumValue);
}

void AkVideoCaps::setFormat(AkVideoCaps::PixelFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    this->d->updateParams();
    emit this->formatChanged(format);
}

void AkVideoCaps::setSize(const QSize &size)
{
    QSize curSize(this->d->m_width, this->d->m_height);

    if (curSize == size)
        return;

    this->d->m_width = size.width();
    this->d->m_height = size.height();
    this->d->updateParams();
    emit this->widthChanged(size.width());
    emit this->heightChanged(size.height());
    emit sizeChanged(size);
}

void AkVideoCaps::setWidth(int width)
{
    if (this->d->m_width == width)
        return;

    this->d->m_width = width;
    this->d->updateParams();
    emit this->widthChanged(width);
}

void AkVideoCaps::setHeight(int height)
{
    if (this->d->m_height == height)
        return;

    this->d->m_height = height;
    this->d->updateParams();
    emit this->heightChanged(height);
}

void AkVideoCaps::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_fps = fps;
    emit this->fpsChanged(fps);
}

void AkVideoCaps::setAlign(int align)
{
    if (this->d->m_align == align)
        return;

    this->d->m_align = align;
    this->d->updateParams();
    emit this->alignChanged(align);
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

void AkVideoCaps::resetAlign()
{
    this->setAlign(1);
}

void AkVideoCaps::clear()
{
    for (auto &property: this->dynamicPropertyNames())
        this->setProperty(property.constData(), {});
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

void AkVideoCapsPrivate::updateParams()
{
    auto vf = VideoFormat::byFormat(this->m_format);
    this->m_planeSize.clear();
    this->m_offset.clear();
    this->m_bypl.clear();

    if (!vf)
        return;

    size_t offset = 0;
    static const size_t k = 16;

    for (auto &plane: vf->planes) {
        this->m_offset << offset;
        size_t bpp = 0;
        size_t step = 0;
        size_t heightDiv = 0;

        for (auto &component: plane) {
            step = qMax(step, component.step);
            heightDiv = qMax(heightDiv, component.heightDiv);
            bpp += k * component.rlength / (1 << component.widthDiv);
        }

        size_t bypl =
                vf->type == VFT_YUV?
                    AkVideoCapsPrivate::alignUp(bpp * this->m_width / k,
                                                size_t(this->m_align)):
                    AkVideoCapsPrivate::alignUp(step * this->m_width,
                                                size_t(this->m_align));
        this->m_bypl << bypl;
        size_t planeSize =
                vf->type == VFT_YUV?
                    bypl * size_t(this->m_height) / (1 << heightDiv):
                    bypl * size_t(this->m_height);
        this->m_planeSize << planeSize;
        offset += planeSize;
    }
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
                    << ",align="
                    << caps.align();

    QStringList properties;

    for (auto &property: caps.dynamicPropertyNames())
        properties << QString::fromUtf8(property.constData());

    properties.sort();

    for (auto &property: properties)
        debug.nospace() << ","
                        << property.toStdString().c_str()
                        << "="
                        << caps.property(property.toStdString().c_str());

    debug.nospace() << ")";

    return debug.space();
}

QDebug operator <<(QDebug debug, AkVideoCaps::PixelFormat format)
{
    debug.nospace() << AkVideoCaps::pixelFormatToString(format).toStdString().c_str();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkVideoCaps &caps)
{
    int nProperties;
    istream >> nProperties;

    for (int i = 0; i < nProperties; i++) {
        QByteArray key;
        QVariant value;
        istream >> key;
        istream >> value;

        caps.setProperty(key.toStdString().c_str(), value);
    }

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkVideoCaps &caps)
{
    QVariantMap staticProperties {
        {"format", caps.format()                  },
        {"width" , caps.width()                   },
        {"height", caps.height()                  },
        {"fps"   , QVariant::fromValue(caps.fps())},
        {"align" , caps.align()                   },
    };

    int nProperties =
            staticProperties.size() + caps.dynamicPropertyNames().size();
    ostream << nProperties;

    for (auto &key: caps.dynamicPropertyNames()) {
        ostream << key;
        ostream << caps.property(key);
    }

    return ostream;
}

#include "moc_akvideocaps.cpp"
