/* Webcamoid, camera capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#include <QMutex>
#include <qrgb.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "screendev.h"

#define VALUE_SHIFT 8

class ScreenDevPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_rgba, 0, 0, {}}};
};

ScreenDev::ScreenDev(QObject *parent):
    QObject(parent)
{
    this->d = new ScreenDevPrivate;
}

ScreenDev::~ScreenDev()
{
    delete this->d;
}

AkPacket ScreenDev::rotate(const AkPacket &packet, qreal angle)
{
    // Normalize angle to [0, 360)
    int normalizedAngle = int(angle) % 360;

    if (normalizedAngle < 0)
        normalizedAngle += 360;

    // 0°: return original packet without any processing
    if (normalizedAngle == 0)
        return packet;

    // Convert to RGBA format (required for pixel manipulation)
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return packet;

    int srcW = src.caps().width();
    int srcH = src.caps().height();
    auto caps = src.caps();
    AkVideoPacket dst;

    if (normalizedAngle == 180) {
        // 180°: same dimensions, reverse both axes
        dst = AkVideoPacket(caps);
        dst.copyMetadata(src);

        for (int y = 0; y < srcH; ++y) {
            auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
            auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, srcH - 1 - y));
            std::reverse_copy(srcLine, srcLine + srcW, dstLine);
        }
    } else {
        // 90° or 270°: swap width and height
        caps.setWidth(srcH);
        caps.setHeight(srcW);
        dst = AkVideoPacket(caps);
        dst.copyMetadata(src);

        if (normalizedAngle == 90) {
            // 90° clockwise: dst(x, y) = src(srcH - 1 - x, y)
            QVector<const QRgb *> srcLines(srcH);

            for (int row = 0; row < srcH; ++row)
                srcLines[row] = reinterpret_cast<const QRgb *>(src.constLine(0, row));

            for (int y = 0; y < srcW; ++y) {
                auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

                for (int x = 0; x < srcH; ++x)
                    dstLine[x] = srcLines[srcH - 1 - x][y];
            }
        } else { // 270°
            // 270° clockwise = 90° counter-clockwise
            // dst(x, y) = src(x, srcW - 1 - y)
            for (int y = 0; y < srcW; ++y) {
                auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));
                auto srcLine = reinterpret_cast<const QRgb *>(src.constLine(0, srcW - 1 - y));
                memcpy(dstLine, srcLine, srcH * sizeof(QRgb));
            }
        }
    }

    return dst;
}

#include "moc_screendev.cpp"
