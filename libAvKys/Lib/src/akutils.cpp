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

#include "akutils.h"

typedef QMap<QImage::Format, AkVideoCaps::PixelFormat> ImageToPixelFormatMap;

inline ImageToPixelFormatMap initImageToPixelFormatMap()
{
    ImageToPixelFormatMap imageToFormat;
    imageToFormat[QImage::Format_Mono] = AkVideoCaps::Format_monob;
    imageToFormat[QImage::Format_RGB32] = AkVideoCaps::Format_0rgb;
    imageToFormat[QImage::Format_ARGB32] = AkVideoCaps::Format_argb;
    imageToFormat[QImage::Format_RGB16] = AkVideoCaps::Format_rgb565le;
    imageToFormat[QImage::Format_RGB555] = AkVideoCaps::Format_rgb555le;
    imageToFormat[QImage::Format_RGB888] = AkVideoCaps::Format_rgb24;
    imageToFormat[QImage::Format_RGB444] = AkVideoCaps::Format_rgb444le;
    imageToFormat[QImage::Format_Grayscale8] = AkVideoCaps::Format_gray;

    return imageToFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImageToPixelFormatMap, imageToFormat, (initImageToPixelFormatMap()))

AkPacket AkUtils::imageToPacket(const QImage &image, const AkPacket &defaultPacket)
{
    if (!imageToFormat->contains(image.format()))
        return AkPacket();

    QByteArray oBuffer(image.byteCount(), 0);
    memcpy(oBuffer.data(), image.constBits(), size_t(image.byteCount()));

    AkVideoCaps caps(defaultPacket.caps());
    caps.format() = imageToFormat->value(image.format());
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

    if (!imageToFormat->values().contains(caps.format()))
        return QImage();

    QImage image(caps.width(),
                 caps.height(),
                 imageToFormat->key(caps.format()));
    memcpy(image.bits(), packet.buffer().constData(), size_t(packet.buffer().size()));

    if (caps.format() == AkVideoCaps::Format_gray)
        for (int i = 0; i < 256; i++)
            image.setColor(i, QRgb(i));

    return image;
}

AkPacket AkUtils::roundSizeTo(const AkPacket &packet, int n)
{
    int frameWidth = packet.property("width").toInt();
    int frameHeight = packet.property("height").toInt();

    int width = n * qRound(frameWidth / qreal(n));
    int height = n * qRound(frameHeight / qreal(n));

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
    if (!imageToFormat->values().contains(format))
        return AkVideoPacket();

    if (packet.caps().format() == format
        && (size.isEmpty() || packet.caps().size() == size))
        return packet;

    QImage frame = AkUtils::packetToImage(packet.toPacket());

    if (frame.isNull())
        return packet;

    QImage convertedFrame;

    if (size.isEmpty())
        convertedFrame = frame.convertToFormat(imageToFormat->key(format));
    else
        convertedFrame = frame.convertToFormat(imageToFormat->key(format)).scaled(size);

    return AkUtils::imageToPacket(convertedFrame, packet.toPacket());
}
