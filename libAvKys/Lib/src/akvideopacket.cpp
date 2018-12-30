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
#include <QImage>
#include <QVariant>

#include "akvideopacket.h"
#include "akcaps.h"
#include "akvideocaps.h"

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
};

AkVideoPacket::AkVideoPacket(QObject *parent):
    AkPacket(parent)
{
    this->d = new AkVideoPacketPrivate();
}

AkVideoPacket::AkVideoPacket(const AkVideoCaps &caps,
                             const QByteArray &buffer,
                             qint64 pts,
                             const AkFrac &timeBase,
                             int index,
                             qint64 id)
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = caps;
    this->buffer() = buffer;
    this->pts() = pts;
    this->timeBase() = timeBase;
    this->index() = index;
    this->id() = id;
}

AkVideoPacket::AkVideoPacket(const AkPacket &other)
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = other.caps();
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

AkVideoPacket::AkVideoPacket(const AkVideoPacket &other):
    AkPacket()
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

AkVideoPacket::~AkVideoPacket()
{
    delete this->d;
}

AkVideoPacket &AkVideoPacket::operator =(const AkPacket &other)
{
    this->d->m_caps = other.caps();
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();

    return *this;
}

AkVideoPacket &AkVideoPacket::operator =(const AkVideoPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->data() = other.data();
        this->buffer() = other.buffer();
        this->pts() = other.pts();
        this->timeBase() = other.timeBase();
        this->index() = other.index();
        this->id() = other.id();
    }

    return *this;
}

AkVideoPacket::operator bool() const
{
    return this->d->m_caps.isValid();
}

AkVideoCaps AkVideoPacket::caps() const
{
    return this->d->m_caps;
}

AkVideoCaps &AkVideoPacket::caps()
{
    return this->d->m_caps;
}

QString AkVideoPacket::toString() const
{
    QString packetInfo;
    QDebug debug(&packetInfo);

    debug.nospace() << "Caps       : "
                    << this->d->m_caps.toString().toStdString().c_str()
                    << "\n"
                    << "Data       : "
                    << this->data()
                    << "\n"
                    << "Buffer Size: "
                    << this->buffer().size()
                    << "\n"
                    << "Id         : "
                    << this->id()
                    << "\n"
                    << "Pts        : "
                    << this->pts()
                    << " ("
                    << this->pts() * this->timeBase().value()
                    << ")\n"
                    << "Time Base  : "
                    << this->timeBase().toString().toStdString().c_str()
                    << "\n"
                    << "Index      : "
                    << this->index();

    return packetInfo;
}

AkPacket AkVideoPacket::toPacket() const
{
    AkPacket packet;
    packet.caps() =  this->d->m_caps.toCaps();
    packet.buffer() = this->buffer();
    packet.pts() = this->pts();
    packet.timeBase() = this->timeBase();
    packet.index() = this->index();
    packet.id() = this->id();

    return packet;
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
    auto size = qMin(size_t(this->buffer().size()),
                     size_t(image.sizeInBytes()));

    if (size > 0)
        memcpy(image.bits(), this->buffer().constData(), size);

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

    size_t imageSize = size_t(image.bytesPerLine()) * size_t(image.height());
    QByteArray oBuffer(int(imageSize), 0);
    memcpy(oBuffer.data(), image.constBits(), imageSize);

    AkVideoPacket packet = defaultPacket;
    packet.caps().format() = AkImageToFormat->value(image.format());
    packet.caps().bpp() = AkVideoCaps::bitsPerPixel(packet.caps().format());
    packet.caps().width() = image.width();
    packet.caps().height() = image.height();
    packet.setBuffer(oBuffer);

    return packet;
}

AkVideoPacket AkVideoPacket::roundSizeTo(int align) const
{
    /* Explanation:
     *
     * When 'align' is a power of 2, the left most bit will be 1 (the pivot),
     * while all other bits be 0, if destination width is multiple of 'align'
     * all bits after pivot position will be 0, then we create a mask
     * substracting 1 to the align, so all bits after pivot position in the
     * mask will 1.
     * Then we negate all bits in the mask so all bits from pivot to the left
     * will be 1, and then we use that mask to get a width multiple of align.
     * This give us the lower (floor) width nearest to the original 'width' and
     * multiple of align. To get the rounded nearest value we add align / 2 to
     * 'width'.
     * This is the equivalent of:
     *
     * align * round(width / align)
     */
    int width = (this->d->m_caps.width() + (align >> 1)) & ~(align - 1);

    /* Find the nearest width:
     *
     * round(height * owidth / width)
     */
    int height = (2 * this->d->m_caps.height() * width
                  + this->d->m_caps.width())
                 / (2 * this->d->m_caps.width());

    if (this->d->m_caps.width() == width
        && this->d->m_caps.height() == height)
        return *this;

    auto frame = this->toImage();

    if (frame.isNull())
        return *this;

    return AkVideoPacket::fromImage(frame.scaled(width, height), *this);
}

AkVideoPacket AkVideoPacket::convert(AkVideoCaps::PixelFormat format,
                                     const QSize &size) const
{
    if (!AkImageToFormat->values().contains(format))
        return AkVideoPacket();

    if (this->d->m_caps.format() == format
        && (size.isEmpty() || this->d->m_caps.size() == size))
        return *this;

    auto frame = this->toImage();

    if (frame.isNull())
        return *this;

    QImage convertedFrame;

    if (size.isEmpty())
        convertedFrame = frame.convertToFormat(AkImageToFormat->key(format));
    else
        convertedFrame = frame.convertToFormat(AkImageToFormat->key(format)).scaled(size);

    return AkVideoPacket::fromImage(convertedFrame, *this);
}

void AkVideoPacket::setCaps(const AkVideoCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void AkVideoPacket::resetCaps()
{
    this->setCaps(AkVideoCaps());
}

QDebug operator <<(QDebug debug, const AkVideoPacket &packet)
{
    debug.nospace() << packet.toString().toStdString().c_str();

    return debug.space();
}

#include "moc_akvideopacket.cpp"
