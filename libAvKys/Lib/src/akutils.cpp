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

#include <QVariant>
#include <QMap>
#include <QImage>

#include "akutils.h"
#include "akcaps.h"
#include "akvideocaps.h"
#include "akpacket.h"
#include "akvideopacket.h"

typedef QMap<QImage::Format, AkVideoCaps::PixelFormat> ImageToPixelFormatMap;

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

AkPacket AkUtils::imageToPacket(const QImage &image, const AkPacket &defaultPacket)
{
    if (!AkImageToFormat->contains(image.format()))
        return AkPacket();

    size_t imageSize = size_t(image.bytesPerLine()) * size_t(image.height());
    QByteArray oBuffer(int(imageSize), 0);
    memcpy(oBuffer.data(), image.constBits(), imageSize);

    AkVideoCaps caps(defaultPacket.caps());
    caps.format() = AkImageToFormat->value(image.format());
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = image.width();
    caps.height() = image.height();

    AkPacket packet = defaultPacket;
    packet.setCaps(caps.toCaps());
    packet.setBuffer(oBuffer);

    return packet;
}

QImage AkUtils::packetToImage(const AkPacket &packet)
{
    AkVideoCaps caps(packet.caps());

    if (!caps)
        return QImage();

    if (!AkImageToFormat->values().contains(caps.format()))
        return QImage();

    QImage image(caps.width(),
                 caps.height(),
                 AkImageToFormat->key(caps.format()));
    memcpy(image.bits(), packet.buffer().constData(), size_t(packet.buffer().size()));

    if (caps.format() == AkVideoCaps::Format_gray)
        for (int i = 0; i < 256; i++)
            image.setColor(i, QRgb(i));

    return image;
}

AkPacket AkUtils::roundSizeTo(const AkPacket &packet, int align)
{
    int frameWidth = packet.caps().property("width").toInt();
    int frameHeight = packet.caps().property("height").toInt();

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
    int width = (frameWidth + (align >> 1)) & ~(align - 1);

    /* Find the nearest width:
     *
     * round(height * owidth / width)
     */
    int height = (2 * frameHeight * width + frameWidth) / (2 * frameWidth);

    if (frameWidth == width
        && frameHeight == height)
        return packet;

    QImage frame = AkUtils::packetToImage(packet);

    if (frame.isNull())
        return packet;

    return AkUtils::imageToPacket(frame.scaled(width, height), packet);
}

AkVideoPacket AkUtils::convertVideo(const AkVideoPacket &packet,
                                    AkVideoCaps::PixelFormat format,
                                    const QSize &size)
{
    if (!AkImageToFormat->values().contains(format))
        return AkVideoPacket();

    if (packet.caps().format() == format
        && (size.isEmpty() || packet.caps().size() == size))
        return packet;

    QImage frame = AkUtils::packetToImage(packet.toPacket());

    if (frame.isNull())
        return packet;

    QImage convertedFrame;

    if (size.isEmpty())
        convertedFrame = frame.convertToFormat(AkImageToFormat->key(format));
    else
        convertedFrame = frame.convertToFormat(AkImageToFormat->key(format)).scaled(size);

    return AkUtils::imageToPacket(convertedFrame, packet.toPacket());
}
