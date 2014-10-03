/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "qbutils.h"

QbPacket QbUtils::imageToPacket(const QImage &image, const QbPacket &defaultPacket)
{
    QMap<QImage::Format, QString> imageToFormat;

    imageToFormat[QImage::Format_Mono] = "monob";
    imageToFormat[QImage::Format_Indexed8] = "gray";
    imageToFormat[QImage::Format_RGB32] = "bgr0";
    imageToFormat[QImage::Format_ARGB32] = "bgra";
    imageToFormat[QImage::Format_RGB16] = "rgb565le";
    imageToFormat[QImage::Format_RGB555] = "rgb555le";
    imageToFormat[QImage::Format_RGB888] = "bgr24";
    imageToFormat[QImage::Format_RGB444] = "rgb444le";

    if (!imageToFormat.contains(image.format()))
        return QbPacket();

    QbBufferPtr oBuffer(new char[image.byteCount()]);
    memcpy(oBuffer.data(), image.constBits(), image.byteCount());

    QbCaps caps(defaultPacket.caps());
    caps.setMimeType("video/x-raw");
    caps.setProperty("format", imageToFormat[image.format()]);
    caps.setProperty("width", image.width());
    caps.setProperty("height", image.height());

    QbPacket packet = defaultPacket;
    packet.setCaps(caps);
    packet.setBuffer(oBuffer);
    packet.setBufferSize(image.byteCount());

    return packet;
}

QImage QbUtils::packetToImage(const QbPacket &packet)
{
    if (packet.caps().mimeType() != "video/x-raw")
        return QImage();

    QMap<QString, QImage::Format> formatToImage;

    formatToImage["monob"] = QImage::Format_Mono;
    formatToImage["gray"] = QImage::Format_Indexed8;
    formatToImage["bgr0"] = QImage::Format_RGB32;
    formatToImage["bgra"] = QImage::Format_ARGB32;
    formatToImage["rgb565le"] = QImage::Format_RGB16;
    formatToImage["rgb555le"] = QImage::Format_RGB555;
    formatToImage["bgr24"] = QImage::Format_RGB888;
    formatToImage["rgb444le"] = QImage::Format_RGB444;

    QString format = packet.caps().property("format").toString();

    if (!formatToImage.contains(format))
        return QImage();

    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage image((const uchar *) packet.buffer().data(),
                 width,
                 height,
                 formatToImage[format]);

    if (format == "gray")
        for (int i = 0; i < 256; i++)
            image.setColor(i, i);

    return image;
}
