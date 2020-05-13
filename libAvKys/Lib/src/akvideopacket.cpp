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

#include <QDebug>
#include <QVariant>
#include <QImage>
#include <QQmlEngine>

#include "akvideopacket.h"
#include "akpacket.h"
#include "akcaps.h"
#include "akfrac.h"

struct RGBX
{
    uint8_t x;
    uint8_t b;
    uint8_t g;
    uint8_t r;
};

struct XRGB
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t x;
};

struct RGB24
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
};

struct RGB16
{
    uint16_t b: 5;
    uint16_t g: 6;
    uint16_t r: 5;
};

struct RGB15
{
    uint16_t b: 5;
    uint16_t g: 5;
    uint16_t r: 5;
    uint16_t x: 1;
};

struct XBGR
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t x;
};

struct BGRX
{
    uint8_t x;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct BGR24
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct BGR16
{
    uint16_t r: 5;
    uint16_t g: 6;
    uint16_t b: 5;
};

struct BGR15
{
    uint16_t r: 5;
    uint16_t g: 5;
    uint16_t b: 5;
    uint16_t x: 1;
};

struct UYVY
{
    uint8_t v0;
    uint8_t y0;
    uint8_t u0;
    uint8_t y1;
};

struct YUY2
{
    uint8_t y0;
    uint8_t v0;
    uint8_t y1;
    uint8_t u0;
};

struct UV
{
    uint8_t u;
    uint8_t v;
};

struct VU
{
    uint8_t v;
    uint8_t u;
};

using VideoConvertFuntion = AkVideoPacket (*)(const AkVideoPacket *src,
                                              int align);

struct VideoConvert
{
    AkVideoCaps::PixelFormat from;
    AkVideoCaps::PixelFormat to;
    VideoConvertFuntion convert;
};

using ImageToPixelFormatMap = QMap<QImage::Format, AkVideoCaps::PixelFormat>;

inline ImageToPixelFormatMap initImageToPixelFormatMap()
{
    ImageToPixelFormatMap imageToFormat {
        {QImage::Format_Mono      , AkVideoCaps::Format_monob   },
        {QImage::Format_RGB32     , AkVideoCaps::Format_0rgb    },
        {QImage::Format_ARGB32    , AkVideoCaps::Format_argb    },
        {QImage::Format_RGB16     , AkVideoCaps::Format_rgb565le},
        {QImage::Format_RGB555    , AkVideoCaps::Format_rgb555le},
        {QImage::Format_RGB888    , AkVideoCaps::Format_rgb24   },
        {QImage::Format_RGB444    , AkVideoCaps::Format_rgb444le},
        {QImage::Format_Grayscale8, AkVideoCaps::Format_gray    }
    };

    return imageToFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImageToPixelFormatMap, AkImageToFormat, (initImageToPixelFormatMap()))

class AkVideoPacketPrivate
{
    public:
        AkVideoCaps m_caps;
        QByteArray m_buffer;
        qint64 m_pts {0};
        AkFrac m_timeBase;
        qint64 m_id {-1};
        int m_index {-1};

        // YUV utility functions
        inline static uint8_t rgb_y(int r, int g, int b);
        inline static uint8_t rgb_u(int r, int g, int b);
        inline static uint8_t rgb_v(int r, int g, int b);
        inline static uint8_t yuv_r(int y, int u, int v);
        inline static uint8_t yuv_g(int y, int u, int v);
        inline static uint8_t yuv_b(int y, int u, int v);

        static AkVideoPacket bgr24_to_0rgb(const AkVideoPacket *src, int align);
        static AkVideoPacket bgr24_to_rgb24(const AkVideoPacket *src, int align);
        static AkVideoPacket bgr24_to_rgb565le(const AkVideoPacket *src, int align);
        static AkVideoPacket bgr24_to_rgb555le(const AkVideoPacket *src, int align);
        static AkVideoPacket bgr24_to_0bgr(const AkVideoPacket *src, int align);
        static AkVideoPacket bgr24_to_bgr565le(const AkVideoPacket *src, int align);
        static AkVideoPacket bgr24_to_bgr555le(const AkVideoPacket *src, int align);
        static AkVideoPacket bgr24_to_uyvy422(const AkVideoPacket *src, int align);
        static AkVideoPacket bgr24_to_yuyv422(const AkVideoPacket *src, int align);
        static AkVideoPacket bgr24_to_nv12(const AkVideoPacket *src, int align);
        static AkVideoPacket bgr24_to_nv21(const AkVideoPacket *src, int align);

        static AkVideoPacket rgb24_to_0rgb(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_rgb565le(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_rgb555le(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_0bgr(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_bgr24(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_bgr565le(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_bgr555le(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_uyvy422(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_yuyv422(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_nv12(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_nv21(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb24_to_yuv420p(const AkVideoPacket *src, int align);

        static AkVideoPacket rgba_to_rgb24(const AkVideoPacket *src, int align);
        static AkVideoPacket rgb0_to_rgb24(const AkVideoPacket *src, int align);
        static AkVideoPacket yuyv422_to_rgb24(const AkVideoPacket *src, int align);
        static AkVideoPacket yuv420p_to_rgb24(const AkVideoPacket *src, int align);
        static AkVideoPacket yvu420p_to_rgb24(const AkVideoPacket *src, int align);
        static AkVideoPacket nv12_to_rgb24(const AkVideoPacket *src, int align);
        static AkVideoPacket nv21_to_rgb24(const AkVideoPacket *src, int align);
        static AkVideoPacket rgbap_to_rgb24(const AkVideoPacket *src, int align);
        static AkVideoPacket _0bgr_to_rgb24(const AkVideoPacket *src, int align);
};

using VideoConvertFuncs = QVector<VideoConvert>;

VideoConvertFuncs initVideoConvertFuncs()
{
    VideoConvertFuncs convert {
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_0rgb    , AkVideoPacketPrivate::bgr24_to_0rgb    },
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::bgr24_to_rgb24   },
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_rgb565le, AkVideoPacketPrivate::bgr24_to_rgb565le},
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_rgb555le, AkVideoPacketPrivate::bgr24_to_rgb555le},
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_0bgr    , AkVideoPacketPrivate::bgr24_to_0bgr    },
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_bgr565le, AkVideoPacketPrivate::bgr24_to_bgr565le},
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_bgr555le, AkVideoPacketPrivate::bgr24_to_bgr555le},
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_uyvy422 , AkVideoPacketPrivate::bgr24_to_uyvy422 },
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_yuyv422 , AkVideoPacketPrivate::bgr24_to_yuyv422 },
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_nv12    , AkVideoPacketPrivate::bgr24_to_nv12    },
        {AkVideoCaps::Format_bgr24  , AkVideoCaps::Format_nv21    , AkVideoPacketPrivate::bgr24_to_nv21    },

        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_0rgb    , AkVideoPacketPrivate::rgb24_to_0rgb    },
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_rgb565le, AkVideoPacketPrivate::rgb24_to_rgb565le},
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_rgb555le, AkVideoPacketPrivate::rgb24_to_rgb555le},
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_0bgr    , AkVideoPacketPrivate::rgb24_to_0bgr    },
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_bgr24   , AkVideoPacketPrivate::rgb24_to_bgr24   },
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_bgr565le, AkVideoPacketPrivate::rgb24_to_bgr565le},
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_bgr555le, AkVideoPacketPrivate::rgb24_to_bgr555le},
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_uyvy422 , AkVideoPacketPrivate::rgb24_to_uyvy422 },
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_yuyv422 , AkVideoPacketPrivate::rgb24_to_yuyv422 },
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_nv12    , AkVideoPacketPrivate::rgb24_to_nv12    },
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_nv21    , AkVideoPacketPrivate::rgb24_to_nv21    },
        {AkVideoCaps::Format_rgb24  , AkVideoCaps::Format_yuv420p , AkVideoPacketPrivate::rgb24_to_yuv420p },

        {AkVideoCaps::Format_rgba   , AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::rgba_to_rgb24    },
        {AkVideoCaps::Format_rgb0   , AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::rgba_to_rgb24    },
        {AkVideoCaps::Format_yuyv422, AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::yuyv422_to_rgb24 },
        {AkVideoCaps::Format_yuv420p, AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::yuv420p_to_rgb24 },
        {AkVideoCaps::Format_yvu420p, AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::yvu420p_to_rgb24 },
        {AkVideoCaps::Format_yuv422p, AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::yuv420p_to_rgb24 },
        {AkVideoCaps::Format_nv12   , AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::nv12_to_rgb24    },
        {AkVideoCaps::Format_nv16   , AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::nv12_to_rgb24    },
        {AkVideoCaps::Format_nv21   , AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::nv21_to_rgb24    },
        {AkVideoCaps::Format_rgbap  , AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::rgbap_to_rgb24   },
        {AkVideoCaps::Format_0bgr   , AkVideoCaps::Format_rgb24   , AkVideoPacketPrivate::_0bgr_to_rgb24   },
    };

    return convert;
}

Q_GLOBAL_STATIC_WITH_ARGS(VideoConvertFuncs, videoConvert, (initVideoConvertFuncs()))

AkVideoPacket::AkVideoPacket(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoPacketPrivate();
}

AkVideoPacket::AkVideoPacket(const AkVideoCaps &caps)
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = caps;
    this->d->m_buffer = QByteArray(int(caps.pictureSize()), Qt::Uninitialized);
}

AkVideoPacket::AkVideoPacket(const AkPacket &other)
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = other.caps();
    this->d->m_buffer = other.buffer();
    this->d->m_pts = other.pts();
    this->d->m_timeBase = other.timeBase();
    this->d->m_index = other.index();
    this->d->m_id = other.id();
}

AkVideoPacket::AkVideoPacket(const AkVideoPacket &other):
    QObject()
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_buffer = other.d->m_buffer;
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

AkVideoPacket::~AkVideoPacket()
{
    delete this->d;
}

AkVideoPacket &AkVideoPacket::operator =(const AkPacket &other)
{
    this->d->m_caps = other.caps();
    this->d->m_buffer = other.buffer();
    this->d->m_pts = other.pts();
    this->d->m_timeBase = other.timeBase();
    this->d->m_index = other.index();
    this->d->m_id = other.id();

    return *this;
}

AkVideoPacket &AkVideoPacket::operator =(const AkVideoPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_buffer = other.d->m_buffer;
        this->d->m_pts = other.d->m_pts;
        this->d->m_timeBase = other.d->m_timeBase;
        this->d->m_index = other.d->m_index;
        this->d->m_id = other.d->m_id;
    }

    return *this;
}

AkVideoPacket::operator AkPacket() const
{
    AkPacket packet(this->d->m_caps);
    packet.buffer() = this->d->m_buffer;
    packet.pts() = this->d->m_pts;
    packet.timeBase() = this->d->m_timeBase;
    packet.index() = this->d->m_index;
    packet.id() = this->d->m_id;

    return packet;
}

AkVideoPacket::operator bool() const
{
    return this->d->m_caps && !this->d->m_buffer.isEmpty();
}

AkVideoCaps AkVideoPacket::caps() const
{
    return this->d->m_caps;
}

AkVideoCaps &AkVideoPacket::caps()
{
    return this->d->m_caps;
}

QByteArray AkVideoPacket::buffer() const
{
    return this->d->m_buffer;
}

QByteArray &AkVideoPacket::buffer()
{
    return this->d->m_buffer;
}

qint64 AkVideoPacket::id() const
{
    return this->d->m_id;
}

qint64 &AkVideoPacket::id()
{
    return this->d->m_id;
}

qint64 AkVideoPacket::pts() const
{
    return this->d->m_pts;
}

qint64 &AkVideoPacket::pts()
{
    return this->d->m_pts;
}

AkFrac AkVideoPacket::timeBase() const
{
    return this->d->m_timeBase;
}

AkFrac &AkVideoPacket::timeBase()
{
    return this->d->m_timeBase;
}

int AkVideoPacket::index() const
{
    return this->d->m_index;
}

int &AkVideoPacket::index()
{
    return this->d->m_index;
}

void AkVideoPacket::copyMetadata(const AkVideoPacket &other)
{
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

const quint8 *AkVideoPacket::constLine(int plane, int y) const
{
    return reinterpret_cast<const quint8 *>(this->d->m_buffer.constData())
            + this->d->m_caps.lineOffset(plane, y);
}

quint8 *AkVideoPacket::line(int plane, int y)
{
    return reinterpret_cast<quint8 *>(this->d->m_buffer.data())
            + this->d->m_caps.lineOffset(plane, y);
}

QImage AkVideoPacket::toImage() const
{
    if (!this->d->m_caps)
        return {};

    if (!AkImageToFormat->values().contains(this->d->m_caps.format()))
        return {};

    QImage image(this->d->m_caps.width(),
                 this->d->m_caps.height(),
                 AkImageToFormat->key(this->d->m_caps.format()));
    auto size = qMin(size_t(this->d->m_buffer.size()),
                     size_t(image.bytesPerLine()) * size_t(image.height()));

    if (size > 0)
        memcpy(image.bits(), this->d->m_buffer.constData(), size);

    if (this->d->m_caps.format() == AkVideoCaps::Format_gray)
        for (int i = 0; i < 256; i++)
            image.setColor(i, QRgb(i));

    return image;
}

AkVideoPacket AkVideoPacket::fromImage(const QImage &image,
                                       const AkVideoPacket &defaultPacket)
{
    if (!AkImageToFormat->contains(image.format()))
        return AkVideoPacket();

    auto imageSize = image.bytesPerLine() * image.height();
    QByteArray oBuffer(imageSize, Qt::Uninitialized);
    memcpy(oBuffer.data(), image.constBits(), size_t(imageSize));

    AkVideoPacket packet;
    packet.d->m_caps = {AkImageToFormat->value(image.format()),
                        image.width(),
                        image.height(),
                        defaultPacket.caps().fps()};
    packet.d->m_buffer = oBuffer;
    packet.copyMetadata(defaultPacket);

    return packet;
}

bool AkVideoPacket::canConvert(AkVideoCaps::PixelFormat input,
                               AkVideoCaps::PixelFormat output)
{
    if (input == output)
        return true;

    for (auto &convert: *videoConvert)
        if (convert.from == input
            && convert.to == output) {
            return true;
        }

    auto values = AkImageToFormat->values();

    if (values.contains(input) && values.contains(output))
        return true;

    return false;
}

bool AkVideoPacket::canConvert(AkVideoCaps::PixelFormat output) const
{
    return AkVideoPacket::canConvert(this->d->m_caps.format(), output);
}

AkVideoPacket AkVideoPacket::convert(AkVideoCaps::PixelFormat format) const
{
    return this->convert(format, this->d->m_caps.align());
}

AkVideoPacket AkVideoPacket::convert(AkVideoCaps::PixelFormat format,
                                     int align) const
{
    if (this->d->m_caps.format() == format) {
        if (this->d->m_caps.align() != align)
            return this->realign(align);

        return *this;
    }

    for (auto &convert: *videoConvert)
        if (convert.from == this->d->m_caps.format()
            && convert.to == format) {
            return convert.convert(this, align);
        }

    if (!AkImageToFormat->values().contains(format))
        return {};

    auto frame = this->toImage();

    if (frame.isNull())
        return *this;

    auto convertedFrame = frame.convertToFormat(AkImageToFormat->key(format));

    return AkVideoPacket::fromImage(convertedFrame, *this);
}

AkVideoPacket AkVideoPacket::scaled(int width, int height) const
{
    return AkVideoPacket::fromImage(this->toImage().scaled(width, height),
                                    *this);
}

AkVideoPacket AkVideoPacket::realign(int align) const
{
    if (this->d->m_caps.align() == align)
        return *this;

    auto caps = this->d->m_caps;
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*this);
    auto height = caps.height();

    for (int plane = 0; plane < caps.planes(); plane++) {
        auto bypl = qMin(caps.bytesPerLine(plane),
                         this->d->m_caps.bytesPerLine(plane));

        for (int y = 0; y < height; y++) {
            auto src_line = this->constLine(plane, y);
            auto dst_line = dst.line(plane, y);
            memcpy(dst_line, src_line, bypl);
        }
    }

    return dst;
}

void AkVideoPacket::setCaps(const AkVideoCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void AkVideoPacket::setBuffer(const QByteArray &buffer)
{
    if (this->d->m_buffer == buffer)
        return;

    this->d->m_buffer = buffer;
    emit this->bufferChanged(buffer);
}

void AkVideoPacket::setId(qint64 id)
{
    if (this->d->m_id == id)
        return;

    this->d->m_id = id;
    emit this->idChanged(id);
}

void AkVideoPacket::setPts(qint64 pts)
{
    if (this->d->m_pts == pts)
        return;

    this->d->m_pts = pts;
    emit this->ptsChanged(pts);
}

void AkVideoPacket::setTimeBase(const AkFrac &timeBase)
{
    if (this->d->m_timeBase == timeBase)
        return;

    this->d->m_timeBase = timeBase;
    emit this->timeBaseChanged(timeBase);
}

void AkVideoPacket::setIndex(int index)
{
    if (this->d->m_index == index)
        return;

    this->d->m_index = index;
    emit this->indexChanged(index);
}

void AkVideoPacket::resetCaps()
{
    this->setCaps(AkVideoCaps());
}

void AkVideoPacket::resetBuffer()
{
    this->setBuffer({});
}

void AkVideoPacket::resetId()
{
    this->setId(-1);
}

void AkVideoPacket::resetPts()
{
    this->setPts(0);
}

void AkVideoPacket::resetTimeBase()
{
    this->setTimeBase({});
}

void AkVideoPacket::resetIndex()
{
    this->setIndex(-1);
}

void AkVideoPacket::registerTypes()
{
    qmlRegisterSingletonType<AkVideoPacket>("Ak", 1, 0, "AkVideoPacket",
                                            [] (QQmlEngine *qmlEngine,
                                                QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkVideoPacket();
    });
}

QDebug operator <<(QDebug debug, const AkVideoPacket &packet)
{
    debug.nospace() << "AkVideoPacket("
                    << "caps="
                    << packet.caps()
                    << ",bufferSize="
                    << packet.buffer().size()
                    << ",id="
                    << packet.id()
                    << ",pts="
                    << packet.pts()
                    << "("
                    << packet.pts() * packet.timeBase().value()
                    << ")"
                    << ",timeBase="
                    << packet.timeBase()
                    << ",index="
                    << packet.index()
                    << ")";

    return debug.space();
}

uint8_t AkVideoPacketPrivate::rgb_y(int r, int g, int b)
{
    return uint8_t(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
}

uint8_t AkVideoPacketPrivate::rgb_u(int r, int g, int b)
{
    return uint8_t(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
}

uint8_t AkVideoPacketPrivate::rgb_v(int r, int g, int b)
{
    return uint8_t(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
}

uint8_t AkVideoPacketPrivate::yuv_r(int y, int u, int v)
{
    Q_UNUSED(u)
    int r = (298 * (y - 16) + 409 * (v - 128) + 128) >> 8;

    return uint8_t(qBound(0, r, 255));
}

uint8_t AkVideoPacketPrivate::yuv_g(int y, int u, int v)
{
    int g = (298 * (y - 16) - 100 * (u - 128) - 208 * (v - 128) + 128) >> 8;

    return uint8_t(qBound(0, g, 255));
}

uint8_t AkVideoPacketPrivate::yuv_b(int y, int u, int v)
{
    Q_UNUSED(v)
    int b = (298 * (y - 16) + 516 * (u - 128) + 128) >> 8;

    return uint8_t(qBound(0, b, 255));
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_0rgb(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_0rgb);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGBX *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].x = 255;
            dst_line[x].r = src_line[x].r;
            dst_line[x].g = src_line[x].g;
            dst_line[x].b = src_line[x].b;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_rgb24(const AkVideoPacket *src,
                                                   int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGB24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].r = src_line[x].r;
            dst_line[x].g = src_line[x].g;
            dst_line[x].b = src_line[x].b;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_rgb565le(const AkVideoPacket *src,
                                                      int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb565le);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGB16 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].r = src_line[x].r >> 3;
            dst_line[x].g = src_line[x].g >> 2;
            dst_line[x].b = src_line[x].b >> 3;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_rgb555le(const AkVideoPacket *src,
                                                      int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb555le);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGB15 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].x = 1;
            dst_line[x].r = src_line[x].r >> 3;
            dst_line[x].g = src_line[x].g >> 3;
            dst_line[x].b = src_line[x].b >> 3;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_0bgr(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_0bgr);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<XBGR *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].x = 255;
            dst_line[x].r = src_line[x].r;
            dst_line[x].g = src_line[x].g;
            dst_line[x].b = src_line[x].b;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_bgr565le(const AkVideoPacket *src,
                                                      int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_bgr565le);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<BGR16 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].r = src_line[x].r >> 3;
            dst_line[x].g = src_line[x].g >> 2;
            dst_line[x].b = src_line[x].b >> 3;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_bgr555le(const AkVideoPacket *src,
                                                      int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_bgr555le);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<BGR15 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].x = 1;
            dst_line[x].r = src_line[x].r >> 3;
            dst_line[x].g = src_line[x].g >> 3;
            dst_line[x].b = src_line[x].b >> 3;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_uyvy422(const AkVideoPacket *src,
                                                     int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_uyvy422);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<UYVY *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto r0 = src_line[x].r;
            auto g0 = src_line[x].g;
            auto b0 = src_line[x].b;

            x++;

            int r1 = src_line[x].r;
            int g1 = src_line[x].g;
            int b1 = src_line[x].b;

            dst_line[x_yuv].u0 = rgb_u(r0, g0, b0);
            dst_line[x_yuv].y0 = rgb_y(r0, g0, b0);
            dst_line[x_yuv].v0 = rgb_v(r0, g0, b0);
            dst_line[x_yuv].y1 = rgb_y(r1, g1, b1);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_yuyv422(const AkVideoPacket *src,
                                                     int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_yuyv422);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<YUY2 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto r0 = src_line[x].r;
            auto g0 = src_line[x].g;
            auto b0 = src_line[x].b;

            x++;

            auto r1 = src_line[x].r;
            auto g1 = src_line[x].g;
            auto b1 = src_line[x].b;

            dst_line[x_yuv].y0 = rgb_y(r0, g0, b0);
            dst_line[x_yuv].u0 = rgb_u(r0, g0, b0);
            dst_line[x_yuv].y1 = rgb_y(r1, g1, b1);
            dst_line[x_yuv].v0 = rgb_v(r0, g0, b0);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_nv12(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_nv12);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line_y = dst.line(0, y);
        auto dst_line_vu = reinterpret_cast<VU *>(dst.line(1, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto r = src_line[x].r;
            auto g = src_line[x].g;
            auto b = src_line[x].b;

            dst_line_y[y] = rgb_y(r, g, b);
            dst_line_vu[x_yuv].v = rgb_v(r, g, b);
            dst_line_vu[x_yuv].u = rgb_u(r, g, b);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::bgr24_to_nv21(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_nv21);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const BGR24 *>(src->constLine(0, y));
        auto dst_line_y = dst.line(0, y);
        auto dst_line_uv = reinterpret_cast<UV *>(dst.line(1, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto r = src_line[x].r;
            auto g = src_line[x].g;
            auto b = src_line[x].b;

            dst_line_y[y] = rgb_y(r, g, b);
            dst_line_uv[x_yuv].v = rgb_v(r, g, b);
            dst_line_uv[x_yuv].u = rgb_u(r, g, b);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_0rgb(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_0rgb);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGBX *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].x = 255;
            dst_line[x].r = src_line[x].r;
            dst_line[x].g = src_line[x].g;
            dst_line[x].b = src_line[x].b;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_rgb565le(const AkVideoPacket *src,
                                                      int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb565le);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGB16 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].r = src_line[x].r >> 3;
            dst_line[x].g = src_line[x].g >> 2;
            dst_line[x].b = src_line[x].b >> 3;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_rgb555le(const AkVideoPacket *src,
                                                      int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb555le);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGB15 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].x = 1;
            dst_line[x].r = src_line[x].r >> 3;
            dst_line[x].g = src_line[x].g >> 3;
            dst_line[x].b = src_line[x].b >> 3;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_0bgr(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_0bgr);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<XBGR *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].x = 255;
            dst_line[x].r = src_line[x].r;
            dst_line[x].g = src_line[x].g;
            dst_line[x].b = src_line[x].b;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_bgr24(const AkVideoPacket *src,
                                                   int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_bgr24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<BGR24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].r = src_line[x].r;
            dst_line[x].g = src_line[x].g;
            dst_line[x].b = src_line[x].b;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_bgr565le(const AkVideoPacket *src,
                                                      int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_bgr565le);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<BGR16 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].r = src_line[x].r >> 3;
            dst_line[x].g = src_line[x].g >> 2;
            dst_line[x].b = src_line[x].b >> 3;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_bgr555le(const AkVideoPacket *src,
                                                      int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_bgr555le);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<BGR15 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].x = 1;
            dst_line[x].r = src_line[x].r >> 3;
            dst_line[x].g = src_line[x].g >> 3;
            dst_line[x].b = src_line[x].b >> 3;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_uyvy422(const AkVideoPacket *src,
                                                     int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_uyvy422);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<UYVY *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto r0 = src_line[x].r;
            auto g0 = src_line[x].g;
            auto b0 = src_line[x].b;

            x++;

            int r1 = src_line[x].r;
            int g1 = src_line[x].g;
            int b1 = src_line[x].b;

            dst_line[x_yuv].u0 = rgb_u(r0, g0, b0);
            dst_line[x_yuv].y0 = rgb_y(r0, g0, b0);
            dst_line[x_yuv].v0 = rgb_v(r0, g0, b0);
            dst_line[x_yuv].y1 = rgb_y(r1, g1, b1);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_yuyv422(const AkVideoPacket *src,
                                                     int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_yuyv422);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<YUY2 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto r0 = src_line[x].r;
            auto g0 = src_line[x].g;
            auto b0 = src_line[x].b;

            x++;

            auto r1 = src_line[x].r;
            auto g1 = src_line[x].g;
            auto b1 = src_line[x].b;

            dst_line[x_yuv].y0 = rgb_y(r0, g0, b0);
            dst_line[x_yuv].u0 = rgb_u(r0, g0, b0);
            dst_line[x_yuv].y1 = rgb_y(r1, g1, b1);
            dst_line[x_yuv].v0 = rgb_v(r0, g0, b0);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_nv12(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_nv12);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line_y = dst.line(0, y);
        auto dst_line_vu = reinterpret_cast<VU *>(dst.line(1, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto r = src_line[x].r;
            auto g = src_line[x].g;
            auto b = src_line[x].b;

            dst_line_y[y] = rgb_y(r, g, b);
            dst_line_vu[x_yuv].v = rgb_v(r, g, b);
            dst_line_vu[x_yuv].u = rgb_u(r, g, b);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_nv21(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_nv21);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line_y = dst.line(0, y);
        auto dst_line_uv = reinterpret_cast<UV *>(dst.line(1, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto r = src_line[x].r;
            auto g = src_line[x].g;
            auto b = src_line[x].b;

            dst_line_y[y] = rgb_y(r, g, b);
            dst_line_uv[x_yuv].v = rgb_v(r, g, b);
            dst_line_uv[x_yuv].u = rgb_u(r, g, b);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb24_to_yuv420p(const AkVideoPacket *src,
                                                     int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_yuv420p);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const RGB24 *>(src->constLine(0, y));
        auto dst_line_y = reinterpret_cast<quint8 *>(dst.line(0, y));
        auto dst_line_v = reinterpret_cast<quint8 *>(dst.line(1, y));
        auto dst_line_u = reinterpret_cast<quint8 *>(dst.line(2, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto r = src_line[x].r;
            auto g = src_line[x].g;
            auto b = src_line[x].b;

            dst_line_y[x] = rgb_y(r, g, b);
            dst_line_u[x_yuv] = rgb_u(r, g, b);
            dst_line_v[x_yuv] = rgb_v(r, g, b);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgba_to_rgb24(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const XRGB *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGB24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].r = src_line[x].x * src_line[x].r / 255;
            dst_line[x].g = src_line[x].x * src_line[x].g / 255;
            dst_line[x].b = src_line[x].x * src_line[x].b / 255;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgb0_to_rgb24(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const XRGB *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGB24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].r = src_line[x].r;
            dst_line[x].g = src_line[x].g;
            dst_line[x].b = src_line[x].b;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::yuyv422_to_rgb24(const AkVideoPacket *src,
                                                     int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const YUY2 *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGB24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto y0 = src_line[x_yuv].y0;
            auto u0 = src_line[x_yuv].u0;
            auto y1 = src_line[x_yuv].y1;
            auto v0 = src_line[x_yuv].v0;

            dst_line[x].r = yuv_r(y0, u0, v0);
            dst_line[x].g = yuv_g(y0, u0, v0);
            dst_line[x].b = yuv_b(y0, u0, v0);

            x++;

            dst_line[x].r = yuv_r(y1, u0, v0);
            dst_line[x].g = yuv_g(y1, u0, v0);
            dst_line[x].b = yuv_b(y1, u0, v0);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::yuv420p_to_rgb24(const AkVideoPacket *src,
                                                     int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line_y = reinterpret_cast<const quint8 *>(src->constLine(0, y));
        auto src_line_v = reinterpret_cast<const quint8 *>(src->constLine(1, y));
        auto src_line_u = reinterpret_cast<const quint8 *>(src->constLine(2, y));
        auto dst_line = reinterpret_cast<RGB24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto y = src_line_y[x];
            auto u = src_line_u[x_yuv];
            auto v = src_line_v[x_yuv];

            dst_line[x].r = yuv_r(y, u, v);
            dst_line[x].g = yuv_g(y, u, v);
            dst_line[x].b = yuv_b(y, u, v);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::yvu420p_to_rgb24(const AkVideoPacket *src, int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line_y = reinterpret_cast<const quint8 *>(src->constLine(0, y));
        auto src_line_u = reinterpret_cast<const quint8 *>(src->constLine(1, y));
        auto src_line_v = reinterpret_cast<const quint8 *>(src->constLine(2, y));
        auto dst_line = reinterpret_cast<RGB24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto y = src_line_y[x];
            auto u = src_line_u[x_yuv];
            auto v = src_line_v[x_yuv];

            dst_line[x].r = yuv_r(y, u, v);
            dst_line[x].g = yuv_g(y, u, v);
            dst_line[x].b = yuv_b(y, u, v);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::nv12_to_rgb24(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line_y = src->constLine(0, y);
        auto src_line_vu = reinterpret_cast<const VU *>(src->constLine(1, y));
        auto dst_line = reinterpret_cast<RGB24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto y = src_line_y[x];
            auto u = src_line_vu[x_yuv].u;
            auto v = src_line_vu[x_yuv].v;

            dst_line[x].r = yuv_r(y, u, v);
            dst_line[x].g = yuv_g(y, u, v);
            dst_line[x].b = yuv_b(y, u, v);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::nv21_to_rgb24(const AkVideoPacket *src,
                                                  int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line_y = src->constLine(0, y);
        auto src_line_uv = reinterpret_cast<const UV *>(src->constLine(1, y));
        auto dst_line = reinterpret_cast<RGB24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            auto x_yuv = x / 2;

            auto y = src_line_y[x];
            auto u = src_line_uv[x_yuv].u;
            auto v = src_line_uv[x_yuv].v;

            dst_line[x].r = yuv_r(y, u, v);
            dst_line[x].g = yuv_g(y, u, v);
            dst_line[x].b = yuv_b(y, u, v);
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::rgbap_to_rgb24(const AkVideoPacket *src,
                                                   int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line_r = reinterpret_cast<const quint8 *>(src->constLine(0, y));
        auto src_line_g = reinterpret_cast<const quint8 *>(src->constLine(1, y));
        auto src_line_b = reinterpret_cast<const quint8 *>(src->constLine(2, y));
        auto src_line_a = reinterpret_cast<const quint8 *>(src->constLine(3, y));
        auto dst_line = reinterpret_cast<RGB24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].r = src_line_a[x] * src_line_r[x] / 255;
            dst_line[x].g = src_line_a[x] * src_line_g[x] / 255;
            dst_line[x].b = src_line_a[x] * src_line_b[x] / 255;
        }
    }

    return dst;
}

AkVideoPacket AkVideoPacketPrivate::_0bgr_to_rgb24(const AkVideoPacket *src,
                                                   int align)
{
    auto caps = src->caps();
    caps.setFormat(AkVideoCaps::Format_rgb24);
    caps.setAlign(align);
    AkVideoPacket dst(caps);
    dst.copyMetadata(*src);
    auto width = src->caps().width();
    auto height = src->caps().height();

    for (int y = 0; y < height; y++) {
        auto src_line = reinterpret_cast<const XRGB *>(src->constLine(0, y));
        auto dst_line = reinterpret_cast<RGB24 *>(dst.line(0, y));

        for (int x = 0; x < width; x++) {
            dst_line[x].r = src_line[x].x * src_line[x].r / 255;
            dst_line[x].g = src_line[x].x * src_line[x].g / 255;
            dst_line[x].b = src_line[x].x * src_line[x].b / 255;
        }
    }

    return dst;
}

#include "moc_akvideopacket.cpp"
