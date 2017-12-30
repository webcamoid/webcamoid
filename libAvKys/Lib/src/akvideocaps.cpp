/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <QDebug>
#include <QSize>
#include <QVector>
#include <QMetaEnum>

#include "akvideocaps.h"
#include "akfrac.h"
#include "akcaps.h"

class VideoFormat
{
    public:
        AkVideoCaps::PixelFormat format;
        int bpp;
        quint32 fourCC;

        static inline const QVector<VideoFormat> &formats()
        {
            static const QVector<VideoFormat> videoFormats = {
                {AkVideoCaps::Format_none,             0, AK_FOURCC_NULL},
                {AkVideoCaps::Format_yuv420p,         12, AkFourCC('I', '4', '2', '0')},
                {AkVideoCaps::Format_yuyv422,         16, AkFourCC('Y', 'U', 'Y', '2')},
                {AkVideoCaps::Format_rgb24,           24, AkFourCC('R', 'G', 'B', 24)},
                {AkVideoCaps::Format_bgr24,           24, AkFourCC('B', 'G', 'R', 24)},
                {AkVideoCaps::Format_yuv422p,         16, AkFourCC('Y', '4', '2', 'B')},
                {AkVideoCaps::Format_yuv444p,         24, AkFourCC('4', '4', '4', 'P')},
                {AkVideoCaps::Format_yuv410p,          9, AkFourCC('Y', 'U', 'V', '9')},
                {AkVideoCaps::Format_yuv411p,         12, AkFourCC('Y', '4', '1', 'B')},
                {AkVideoCaps::Format_gray,             8, AkFourCC('Y', '8', '0', '0')},
                {AkVideoCaps::Format_monow,            1, AkFourCC('B', '1', 'W', '0')},
                {AkVideoCaps::Format_monob,            1, AkFourCC('B', '0', 'W', '1')},
                {AkVideoCaps::Format_pal8,             8, AkFourCC('P', 'A', 'L', 8)},
                {AkVideoCaps::Format_yuvj420p,        12, AkFourCC('I', '4', '2', '0')},
                {AkVideoCaps::Format_yuvj422p,        16, AkFourCC('Y', '4', '2', 'B')},
                {AkVideoCaps::Format_yuvj444p,        24, AkFourCC('4', '4', '4', 'P')},
                {AkVideoCaps::Format_uyvy422,         16, AkFourCC('U', 'Y', 'V', 'Y')},
                {AkVideoCaps::Format_uyyvyy411,       12, AkFourCC('Y', '4', '1', '1')},
                {AkVideoCaps::Format_bgr8,             8, AkFourCC('B', 'G', 'R', 8)},
                {AkVideoCaps::Format_bgr4,             4, AkFourCC('B', 'G', 'R', 4)},
                {AkVideoCaps::Format_bgr4_byte,        4, AkFourCC('R', '4', 'B', 'Y')},
                {AkVideoCaps::Format_rgb8,             8, AkFourCC('R', 'G', 'B', 8)},
                {AkVideoCaps::Format_rgb4,             4, AkFourCC('R', 'G', 'B', 4)},
                {AkVideoCaps::Format_rgb4_byte,        4, AkFourCC('B', '4', 'B', 'Y')},
                {AkVideoCaps::Format_nv12,            12, AkFourCC('N', 'V', '1', '2')},
                {AkVideoCaps::Format_nv21,            12, AkFourCC('N', 'V', '2', '1')},
                {AkVideoCaps::Format_argb,            32, AkFourCC('A', 'R', 'G', 'B')},
                {AkVideoCaps::Format_rgba,            32, AkFourCC('R', 'G', 'B', 'A')},
                {AkVideoCaps::Format_abgr,            32, AkFourCC('A', 'B', 'G', 'R')},
                {AkVideoCaps::Format_bgra,            32, AkFourCC('B', 'G', 'R', 'A')},
                {AkVideoCaps::Format_gray16be,        16, AkFourCC(16, 0, '1', 'Y')},
                {AkVideoCaps::Format_gray16le,        16, AkFourCC('Y', '1', 0, 16)},
                {AkVideoCaps::Format_yuv440p,         16, AkFourCC('4', '4', '0', 'P')},
                {AkVideoCaps::Format_yuvj440p,        16, AkFourCC('4', '4', '0', 'P')},
                {AkVideoCaps::Format_yuva420p,        20, AkFourCC('Y', '4', 11, 8)},
                {AkVideoCaps::Format_rgb48be,         48, AkFourCC('0', 'R', 'G', 'B')},
                {AkVideoCaps::Format_rgb48le,         48, AkFourCC('R', 'G', 'B', '0')},
                {AkVideoCaps::Format_rgb565be,        16, AkFourCC(16, 'B', 'G', 'R')},
                {AkVideoCaps::Format_rgb565le,        16, AkFourCC('R', 'G', 'B', 16)},
                {AkVideoCaps::Format_rgb555be,        15, AkFourCC(15, 'B', 'G', 'R')},
                {AkVideoCaps::Format_rgb555le,        15, AkFourCC('R', 'G', 'B', 15)},
                {AkVideoCaps::Format_bgr565be,        16, AkFourCC(16, 'R', 'G', 'B')},
                {AkVideoCaps::Format_bgr565le,        16, AkFourCC('B', 'G', 'R', 16)},
                {AkVideoCaps::Format_bgr555be,        15, AkFourCC(15, 'R', 'G', 'B')},
                {AkVideoCaps::Format_bgr555le,        15, AkFourCC('B', 'G', 'R', 15)},
                {AkVideoCaps::Format_yuv420p16le,     24, AkFourCC('Y', '3', 11, 16)},
                {AkVideoCaps::Format_yuv420p16be,     24, AkFourCC(16, 11, '3', 'Y')},
                {AkVideoCaps::Format_yuv422p16le,     32, AkFourCC('Y', '3', 10, 16)},
                {AkVideoCaps::Format_yuv422p16be,     32, AkFourCC(16, 10, '3', 'Y')},
                {AkVideoCaps::Format_yuv444p16le,     48, AkFourCC('Y', '3', 0, 16)},
                {AkVideoCaps::Format_yuv444p16be,     48, AkFourCC(16, 0, '3', 'Y')},
                {AkVideoCaps::Format_rgb444le,        12, AkFourCC('R', 'G', 'B', 12)},
                {AkVideoCaps::Format_rgb444be,        12, AkFourCC(12, 'B', 'G', 'R')},
                {AkVideoCaps::Format_bgr444le,        12, AkFourCC('B', 'G', 'R', 12)},
                {AkVideoCaps::Format_bgr444be,        12, AkFourCC(12, 'R', 'G', 'B')},
                {AkVideoCaps::Format_ya8,             16, AkFourCC('Y', '2', 0, 8)},
                {AkVideoCaps::Format_bgr48be,         48, AkFourCC('0', 'B', 'G', 'R')},
                {AkVideoCaps::Format_bgr48le,         48, AkFourCC('B', 'G', 'R', '0')},
                {AkVideoCaps::Format_yuv420p9be,      13, AkFourCC(9, 11, '3', 'Y')},
                {AkVideoCaps::Format_yuv420p9le,      13, AkFourCC('Y', '3', 11, 9)},
                {AkVideoCaps::Format_yuv420p10be,     15, AkFourCC(10, 11, '3', 'Y')},
                {AkVideoCaps::Format_yuv420p10le,     15, AkFourCC('Y', '3', 11, 10)},
                {AkVideoCaps::Format_yuv422p10be,     20, AkFourCC(10, 10, '3', 'Y')},
                {AkVideoCaps::Format_yuv422p10le,     20, AkFourCC('Y', '3', 10, 10)},
                {AkVideoCaps::Format_yuv444p9be,      27, AkFourCC(9, 0, '3', 'Y')},
                {AkVideoCaps::Format_yuv444p9le,      27, AkFourCC('Y', '3', 0, 9)},
                {AkVideoCaps::Format_yuv444p10be,     30, AkFourCC(10, 0, '3', 'Y')},
                {AkVideoCaps::Format_yuv444p10le,     30, AkFourCC('Y', '3', 0, 10)},
                {AkVideoCaps::Format_yuv422p9be,      18, AkFourCC(9, 10, '3', 'Y')},
                {AkVideoCaps::Format_yuv422p9le,      18, AkFourCC('Y', '3', 10, 9)},
                {AkVideoCaps::Format_gbrp,            24, AkFourCC('G', '3', 0, 8)},
                {AkVideoCaps::Format_gbrp9be,         27, AkFourCC(9, 0, '3', 'G')},
                {AkVideoCaps::Format_gbrp9le,         27, AkFourCC('G', '3', 0, 9)},
                {AkVideoCaps::Format_gbrp10be,        30, AkFourCC(10, 0, '3', 'G')},
                {AkVideoCaps::Format_gbrp10le,        30, AkFourCC('G', '3', 0, 10)},
                {AkVideoCaps::Format_gbrp16be,        48, AkFourCC(16, 0, '3', 'G')},
                {AkVideoCaps::Format_gbrp16le,        48, AkFourCC('G', '3', 0, 16)},
                {AkVideoCaps::Format_yuva422p,        24, AkFourCC('Y', '4', 10, 8)},
                {AkVideoCaps::Format_yuva444p,        32, AkFourCC('Y', '4', 0, 8)},
                {AkVideoCaps::Format_yuva420p9be,     22, AkFourCC(9, 11, '4', 'Y')},
                {AkVideoCaps::Format_yuva420p9le,     22, AkFourCC('Y', '4', 11, 9)},
                {AkVideoCaps::Format_yuva422p9be,     27, AkFourCC(9, 10, '4', 'Y')},
                {AkVideoCaps::Format_yuva422p9le,     27, AkFourCC('Y', '4', 10, 9)},
                {AkVideoCaps::Format_yuva444p9be,     36, AkFourCC(9, 0, '4', 'Y')},
                {AkVideoCaps::Format_yuva444p9le,     36, AkFourCC('Y', '4', 0, 9)},
                {AkVideoCaps::Format_yuva420p10be,    25, AkFourCC(10, 11, '4', 'Y')},
                {AkVideoCaps::Format_yuva420p10le,    25, AkFourCC('Y', '4', 11, 10)},
                {AkVideoCaps::Format_yuva422p10be,    30, AkFourCC(10, 10, '4', 'Y')},
                {AkVideoCaps::Format_yuva422p10le,    30, AkFourCC('Y', '4', 10, 10)},
                {AkVideoCaps::Format_yuva444p10be,    40, AkFourCC(10, 0, '4', 'Y')},
                {AkVideoCaps::Format_yuva444p10le,    40, AkFourCC('Y', '4', 0, 10)},
                {AkVideoCaps::Format_yuva420p16be,    40, AkFourCC(16, 11, '4', 'Y')},
                {AkVideoCaps::Format_yuva420p16le,    40, AkFourCC('Y', '4', 11, 16)},
                {AkVideoCaps::Format_yuva422p16be,    48, AkFourCC(16, 10, '4', 'Y')},
                {AkVideoCaps::Format_yuva422p16le,    48, AkFourCC('Y', '4', 10, 16)},
                {AkVideoCaps::Format_yuva444p16be,    64, AkFourCC(16, 0, '4', 'Y')},
                {AkVideoCaps::Format_yuva444p16le,    64, AkFourCC('Y', '4', 0, 16)},
                {AkVideoCaps::Format_xyz12le,         36, AkFourCC('X', 'Y', 'Z', '$')},
                {AkVideoCaps::Format_xyz12be,         36, AkFourCC('$', 'Z', 'Y', 'X')},
                {AkVideoCaps::Format_nv16,            16, AK_FOURCC_NULL},
                {AkVideoCaps::Format_nv20le,          20, AK_FOURCC_NULL},
                {AkVideoCaps::Format_nv20be,          20, AK_FOURCC_NULL},
                {AkVideoCaps::Format_rgba64be,        64, AkFourCC('@', 'R', 'B', 'A')},
                {AkVideoCaps::Format_rgba64le,        64, AkFourCC('R', 'B', 'A', '@')},
                {AkVideoCaps::Format_bgra64be,        64, AkFourCC('@', 'B', 'R', 'A')},
                {AkVideoCaps::Format_bgra64le,        64, AkFourCC('B', 'R', 'A', '@')},
                {AkVideoCaps::Format_yvyu422,         16, AkFourCC('Y', 'V', 'Y', 'U')},
                {AkVideoCaps::Format_ya16be,          32, AK_FOURCC_NULL},
                {AkVideoCaps::Format_ya16le,          32, AK_FOURCC_NULL},
                {AkVideoCaps::Format_gbrap,           32, AkFourCC('G', '4', 0, 8)},
                {AkVideoCaps::Format_gbrap16be,       64, AkFourCC(16, 0, '4', 'G')},
                {AkVideoCaps::Format_gbrap16le,       64, AkFourCC('G', '4', 0, 16)},
                {AkVideoCaps::Format_0rgb,            24, AkFourCC(0, 'R', 'G', 'B')},
                {AkVideoCaps::Format_rgb0,            24, AkFourCC('R', 'G', 'B', 0)},
                {AkVideoCaps::Format_0bgr,            24, AkFourCC(0, 'B', 'G', 'R')},
                {AkVideoCaps::Format_bgr0,            24, AkFourCC('B', 'G', 'R', 0)},
                {AkVideoCaps::Format_yuv420p12be,     18, AkFourCC(12, 11, '3', 'Y')},
                {AkVideoCaps::Format_yuv420p12le,     18, AkFourCC('Y', '3', 11, 12)},
                {AkVideoCaps::Format_yuv420p14be,     21, AkFourCC(14, 11, '3', 'Y')},
                {AkVideoCaps::Format_yuv420p14le,     21, AkFourCC('Y', '3', 11, 14)},
                {AkVideoCaps::Format_yuv422p12be,     24, AkFourCC(12, 10, '3', 'Y')},
                {AkVideoCaps::Format_yuv422p12le,     24, AkFourCC('Y', '3', 10, 12)},
                {AkVideoCaps::Format_yuv422p14be,     28, AkFourCC(14, 10, '3', 'Y')},
                {AkVideoCaps::Format_yuv422p14le,     28, AkFourCC('Y', '3', 10, 14)},
                {AkVideoCaps::Format_yuv444p12be,     36, AkFourCC(12, 0, '3', 'Y')},
                {AkVideoCaps::Format_yuv444p12le,     36, AkFourCC('Y', '3', 0, 12)},
                {AkVideoCaps::Format_yuv444p14be,     42, AkFourCC(14, 0, '3', 'Y')},
                {AkVideoCaps::Format_yuv444p14le,     42, AkFourCC('Y', '3', 0, 14)},
                {AkVideoCaps::Format_gbrp12be,        36, AkFourCC(12, 0, '3', 'G')},
                {AkVideoCaps::Format_gbrp12le,        36, AkFourCC('G', '3', 0, 12)},
                {AkVideoCaps::Format_gbrp14be,        42, AkFourCC(14, 0, '3', 'G')},
                {AkVideoCaps::Format_gbrp14le,        42, AkFourCC('G', '3', 0, 14)},
                {AkVideoCaps::Format_yuvj411p,        12, AK_FOURCC_NULL},
                {AkVideoCaps::Format_bayer_bggr8,      8, AkFourCC(0xBA, 'B', 'G',    8)},
                {AkVideoCaps::Format_bayer_rggb8,      8, AkFourCC(0xBA, 'R', 'G',    8)},
                {AkVideoCaps::Format_bayer_gbrg8,      8, AkFourCC(0xBA, 'G', 'B',    8)},
                {AkVideoCaps::Format_bayer_grbg8,      8, AkFourCC(0xBA, 'G', 'R',    8)},
                {AkVideoCaps::Format_bayer_bggr16le,  16, AkFourCC(0xBA, 'B', 'G',   16)},
                {AkVideoCaps::Format_bayer_bggr16be,  16, AkFourCC(  16, 'G', 'B', 0xBA)},
                {AkVideoCaps::Format_bayer_rggb16le,  16, AkFourCC(0xBA, 'R', 'G',   16)},
                {AkVideoCaps::Format_bayer_rggb16be,  16, AkFourCC(  16, 'G', 'R', 0xBA)},
                {AkVideoCaps::Format_bayer_gbrg16le,  16, AkFourCC(0xBA, 'G', 'B',   16)},
                {AkVideoCaps::Format_bayer_gbrg16be,  16, AkFourCC(  16, 'B', 'G', 0xBA)},
                {AkVideoCaps::Format_bayer_grbg16le,  16, AkFourCC(0xBA, 'G', 'R',   16)},
                {AkVideoCaps::Format_bayer_grbg16be,  16, AkFourCC(  16, 'R', 'G', 0xBA)},
                {AkVideoCaps::Format_yuv440p10le,     20, AK_FOURCC_NULL},
                {AkVideoCaps::Format_yuv440p10be,     20, AK_FOURCC_NULL},
                {AkVideoCaps::Format_yuv440p12le,     24, AK_FOURCC_NULL},
                {AkVideoCaps::Format_yuv440p12be,     24, AK_FOURCC_NULL},
                {AkVideoCaps::Format_ayuv64le,        64, AK_FOURCC_NULL},
                {AkVideoCaps::Format_ayuv64be,        64, AK_FOURCC_NULL},
                {AkVideoCaps::Format_p010le,          15, AK_FOURCC_NULL},
                {AkVideoCaps::Format_p010be,          15, AK_FOURCC_NULL},
                {AkVideoCaps::Format_gbrap12be,       48, AkFourCC(12, 0, '4', 'G')},
                {AkVideoCaps::Format_gbrap12le,       48, AkFourCC('G', '4', 0, 12)},
                {AkVideoCaps::Format_gbrap10be,       40, AkFourCC(10, 0, '4', 'G')},
                {AkVideoCaps::Format_gbrap10le,       40, AkFourCC('G', '4', 0, 10)},
                {AkVideoCaps::Format_gray12be,        12, AkFourCC(12, 0, '1', 'Y')},
                {AkVideoCaps::Format_gray12le,        12, AkFourCC('Y', '1', 0, 12)},
                {AkVideoCaps::Format_gray10be,        10, AkFourCC(10, 0, '1', 'Y')},
                {AkVideoCaps::Format_gray10le,        10, AkFourCC('Y', '1', 0, 10)},
                {AkVideoCaps::Format_p016le,          24, AK_FOURCC_NULL},
                {AkVideoCaps::Format_p016be,          24, AK_FOURCC_NULL},
                {AkVideoCaps::Format_gray9be,          9, AkFourCC(9, 0, '1', 'Y')},
                {AkVideoCaps::Format_gray9le,          9, AkFourCC('Y', '1', 0, 9)},
                {AkVideoCaps::Format_gbrpf32be,       96, AK_FOURCC_NULL},
                {AkVideoCaps::Format_gbrpf32le,       96, AK_FOURCC_NULL},
                {AkVideoCaps::Format_gbrapf32be,     128, AK_FOURCC_NULL},
                {AkVideoCaps::Format_gbrapf32le,     128, AK_FOURCC_NULL},
            };

            return videoFormats;
        }

        static inline const VideoFormat *byFormat(AkVideoCaps::PixelFormat format)
        {
            for (int i = 0; i < formats().size(); i++)
                if (formats()[i].format == format)
                    return &formats()[i];

            return &formats()[0];
        }

        static inline const VideoFormat *byBpp(int bpp)
        {
            for (int i = 0; i < formats().size(); i++)
                if (formats()[i].bpp == bpp)
                    return &formats()[i];

            return &formats()[0];
        }

        static inline const VideoFormat *byFourCC(quint32 fourCC)
        {
            for (int i = 0; i < formats().size(); i++)
                if (formats()[i].fourCC == fourCC)
                    return &formats()[i];

            return &formats()[0];
        }
};

class AkVideoCapsPrivate
{
    public:
        bool m_isValid;
        AkVideoCaps::PixelFormat m_format;
        int m_bpp;
        int m_width;
        int m_height;
        AkFrac m_fps;
};

AkVideoCaps::AkVideoCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_format = AkVideoCaps::Format_none;
    this->d->m_bpp = 0;
    this->d->m_width = 0;
    this->d->m_height = 0;
}

AkVideoCaps::AkVideoCaps(const QVariantMap &caps)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_isValid = true;
    this->d->m_format = AkVideoCaps::Format_none;
    this->d->m_bpp = 0;
    this->d->m_width = 0;
    this->d->m_height = 0;

    this->fromMap(caps);
}

AkVideoCaps::AkVideoCaps(const QString &caps)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_format = AkVideoCaps::Format_none;
    this->d->m_bpp = 0;
    this->d->m_width = 0;
    this->d->m_height = 0;

    this->fromString(caps);
}

AkVideoCaps::AkVideoCaps(const AkCaps &caps)
{
    this->d = new AkVideoCapsPrivate();

    if (caps.mimeType() == "video/x-raw") {
        this->d->m_isValid = caps.isValid();
        this->update(caps);
    } else {
        this->d->m_isValid = false;
        this->d->m_format = AkVideoCaps::Format_none;
        this->d->m_bpp = 0;
        this->d->m_width = 0;
        this->d->m_height = 0;
    }
}

AkVideoCaps::AkVideoCaps(const AkVideoCaps &other):
    QObject()
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_isValid = other.d->m_isValid;
    this->d->m_format = other.d->m_format;
    this->d->m_bpp = other.d->m_bpp;
    this->d->m_width = other.d->m_width;
    this->d->m_height = other.d->m_height;
    this->d->m_fps = other.d->m_fps;

    QList<QByteArray> properties = other.dynamicPropertyNames();

    for (const QByteArray &property: properties)
        this->setProperty(property, other.property(property));
}

AkVideoCaps::~AkVideoCaps()
{
    delete this->d;
}

AkVideoCaps &AkVideoCaps::operator =(const AkVideoCaps &other)
{
    if (this != &other) {
        this->d->m_isValid = other.d->m_isValid;
        this->d->m_format = other.d->m_format;
        this->d->m_bpp = other.d->m_bpp;
        this->d->m_width = other.d->m_width;
        this->d->m_height = other.d->m_height;
        this->d->m_fps = other.d->m_fps;

        this->clear();

        QList<QByteArray> properties = other.dynamicPropertyNames();

        for (const QByteArray &property: properties)
            this->setProperty(property, other.property(property));
    }

    return *this;
}

AkVideoCaps &AkVideoCaps::operator =(const AkCaps &caps)
{
    if (caps.mimeType() == "video/x-raw") {
        this->d->m_isValid = caps.isValid();
        this->update(caps);
    } else {
        this->d->m_isValid = false;
        this->d->m_format = AkVideoCaps::Format_none;
        this->d->m_bpp = 0;
        this->d->m_width = 0;
        this->d->m_height = 0;
        this->d->m_fps = AkFrac();
    }

    return *this;
}

AkVideoCaps &AkVideoCaps::operator =(const QString &caps)
{
    return this->operator =(AkCaps(caps));
}

bool AkVideoCaps::operator ==(const AkVideoCaps &other) const
{
    return this->toString() == other.toString();
}

bool AkVideoCaps::operator !=(const AkVideoCaps &other) const
{
    return !(*this == other);
}

AkVideoCaps::operator bool() const
{
    return this->d->m_isValid;
}

AkVideoCaps::operator AkCaps() const
{
    return this->toCaps();
}

bool AkVideoCaps::isValid() const
{
    return this->d->m_isValid;
}

bool &AkVideoCaps::isValid()
{
    return this->d->m_isValid;
}

AkVideoCaps::PixelFormat AkVideoCaps::format() const
{
    return this->d->m_format;
}

AkVideoCaps::PixelFormat &AkVideoCaps::format()
{
    return this->d->m_format;
}

quint32 AkVideoCaps::fourCC() const
{
    return this->fourCC(this->d->m_format);
}

int AkVideoCaps::bpp() const
{
    return this->d->m_bpp;
}

int &AkVideoCaps::bpp()
{
    return this->d->m_bpp;
}

QSize AkVideoCaps::size() const
{
    return QSize(this->d->m_width, this->d->m_height);
}

int AkVideoCaps::width() const
{
    return this->d->m_width;
}

int &AkVideoCaps::width()
{
    return this->d->m_width;
}

int AkVideoCaps::height() const
{
    return this->d->m_height;
}

int &AkVideoCaps::height()
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

int AkVideoCaps::pictureSize() const
{
    return this->d->m_bpp * this->d->m_width * this->d->m_height / 8;
}

AkVideoCaps &AkVideoCaps::fromMap(const QVariantMap &caps)
{
    QList<QByteArray> properties = this->dynamicPropertyNames();

    for (const QByteArray &property: properties)
        this->setProperty(property, QVariant());

    if (!caps.contains("mimeType")) {
        this->d->m_isValid = false;

        return *this;
    }

    for (const QString &key: caps.keys())
        if (key == "mimeType") {
            this->d->m_isValid = caps[key].toString() == "video/x-raw";

            if (!this->d->m_isValid)
                return *this;
        } else
            this->setProperty(key.trimmed().toStdString().c_str(), caps[key]);

    return *this;
}

AkVideoCaps &AkVideoCaps::fromString(const QString &caps)
{
    return *this = caps;
}

QVariantMap AkVideoCaps::toMap() const
{
    QVariantMap map = {
        {"format", this->pixelFormatToString(this->d->m_format)},
        {"bpp"   , this->d->m_bpp                              },
        {"width" , this->d->m_width                            },
        {"height", this->d->m_height                           },
        {"fps"   , QVariant::fromValue(this->d->m_fps)         }
    };

    for (const QByteArray &property: this->dynamicPropertyNames()) {
        QString key = QString::fromUtf8(property.constData());
        map[key] = this->property(property);
    }

    return map;
}

QString AkVideoCaps::toString() const
{
    if (!this->d->m_isValid)
        return QString();

    QString format = this->pixelFormatToString(this->d->m_format);

    QString caps = QString("video/x-raw,"
                           "format=%1,"
                           "bpp=%2,"
                           "width=%3,"
                           "height=%4,"
                           "fps=%5").arg(format)
                                    .arg(this->d->m_bpp)
                                    .arg(this->d->m_width)
                                    .arg(this->d->m_height)
                                    .arg(this->d->m_fps.toString());

    QStringList properties;

    for (const QByteArray &property: this->dynamicPropertyNames())
        properties << QString::fromUtf8(property.constData());

    properties.sort();

    for (const QString &property: properties)
        caps.append(QString(",%1=%2").arg(property)
                                     .arg(this->property(property.toStdString().c_str()).toString()));

    return caps;
}

AkVideoCaps &AkVideoCaps::update(const AkCaps &caps)
{
    if (caps.mimeType() != "video/x-raw")
        return *this;

    this->clear();

    QList<QByteArray> properties = caps.dynamicPropertyNames();

    for (const QByteArray &property: properties)
        if (property == "format")
            this->d->m_format = this->pixelFormatFromString(caps.property(property).toString());
        else if (property == "bpp")
            this->d->m_bpp = caps.property(property).toInt();
        else if (property == "width")
            this->d->m_width = caps.property(property).toInt();
        else if (property == "height")
            this->d->m_height = caps.property(property).toInt();
        else if (property == "fps")
            this->d->m_fps = caps.property("fps").toString();
        else
            this->setProperty(property, caps.property(property));

    return *this;
}

AkCaps AkVideoCaps::toCaps() const
{
    return AkCaps(this->toString());
}

int AkVideoCaps::bitsPerPixel(AkVideoCaps::PixelFormat pixelFormat)
{
    return VideoFormat::byFormat(pixelFormat)->bpp;
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

quint32 AkVideoCaps::fourCC(AkVideoCaps::PixelFormat pixelFormat)
{
    return VideoFormat::byFormat(pixelFormat)->fourCC;
}

quint32 AkVideoCaps::fourCC(const QString &pixelFormat)
{
    return AkVideoCaps::fourCC(AkVideoCaps::pixelFormatFromString(pixelFormat));
}

void AkVideoCaps::setFormat(AkVideoCaps::PixelFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    emit this->formatChanged(format);
}

void AkVideoCaps::setBpp(int bpp)
{
    if (this->d->m_bpp == bpp)
        return;

    this->d->m_bpp = bpp;
    emit this->bppChanged(bpp);
}

void AkVideoCaps::setSize(const QSize &size)
{
    QSize curSize(this->d->m_width, this->d->m_height);

    if (curSize == size)
        return;

    this->setWidth(size.width());
    this->setHeight(size.height());
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

void AkVideoCaps::resetBpp()
{
    this->setBpp(0);
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

void AkVideoCaps::clear()
{
    QList<QByteArray> properties = this->dynamicPropertyNames();

    for (const QByteArray &property: properties)
        this->setProperty(property.constData(), QVariant());
}

QDebug operator <<(QDebug debug, const AkVideoCaps &caps)
{
    debug.nospace() << caps.toString();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkVideoCaps &caps)
{
    QString capsStr;
    istream >> capsStr;
    caps.fromString(capsStr);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkVideoCaps &caps)
{
    ostream << caps.toString();

    return ostream;
}

#include "moc_akvideocaps.cpp"
