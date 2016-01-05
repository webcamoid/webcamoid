/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "akutils.h"

typedef QMap<QImage::Format, QString> ImageStrMap;

inline ImageStrMap initImageStrMap()
{
    QMap<QImage::Format, QString> imageToFormat;
    imageToFormat[QImage::Format_Mono] = "monob";
    imageToFormat[QImage::Format_RGB32] = "bgr0";
    imageToFormat[QImage::Format_ARGB32] = "bgra";
    imageToFormat[QImage::Format_RGB16] = "rgb565le";
    imageToFormat[QImage::Format_RGB555] = "rgb555le";
    imageToFormat[QImage::Format_RGB888] = "bgr24";
    imageToFormat[QImage::Format_RGB444] = "rgb444le";
    imageToFormat[QImage::Format_Grayscale8] = "gray";

    return imageToFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImageStrMap, imageToFormat, (initImageStrMap()))

AkPacket AkUtils::imageToPacket(const QImage &image, const AkPacket &defaultPacket)
{
    if (!imageToFormat->contains(image.format()))
        return AkPacket();

    QByteArray oBuffer(image.byteCount(), Qt::Uninitialized);
    memcpy(oBuffer.data(), image.constBits(), image.byteCount());

    AkCaps caps(defaultPacket.caps());
    caps.setMimeType("video/x-raw");
    caps.setProperty("format", imageToFormat->value(image.format()));
    caps.setProperty("width", image.width());
    caps.setProperty("height", image.height());

    AkPacket packet = defaultPacket;
    packet.setCaps(caps);
    packet.setBuffer(oBuffer);

    return packet;
}

QImage AkUtils::packetToImage(const AkPacket &packet)
{
    if (packet.caps().mimeType() != "video/x-raw")
        return QImage();

    QString format = packet.caps().property("format").toString();

    if (!imageToFormat->values().contains(format))
        return QImage();

    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage image(width,
                 height,
                 imageToFormat->key(format));
    memcpy(image.bits(), packet.buffer().constData(), packet.buffer().size());

    if (format == "gray")
        for (int i = 0; i < 256; i++)
            image.setColor(i, i);

    return image;
}
