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

#include "imageprovider.h"

ImageProvider::ImageProvider():
    QQuickImageProvider(Image)
{
}

QImage ImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id)

    QImage frame = QbUtils::packetToImage(this->m_packet);

    if (frame.isNull()) {
        if (requestedSize.isValid())
            *size = requestedSize;
        else
            *size = QSize(1, 1);

        QImage image(*size, QImage::Format_ARGB32);
        image.fill(qRgba(0, 0, 0, 0));

        return image;
    }

    *size = frame.size();

    if (requestedSize.isValid()) {
        *size = requestedSize;

        return frame.scaled(requestedSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    }

    return frame;
}

void ImageProvider::setFrame(const QbPacket &packet)
{
    this->m_packet = packet;
}
