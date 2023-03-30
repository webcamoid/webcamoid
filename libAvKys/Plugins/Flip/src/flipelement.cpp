/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#include <QQmlContext>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoformatspec.h>
#include <akvideopacket.h>

#include "flipelement.h"

class FlipElementPrivate
{
    public:
        bool m_horizontalFlip {false};
        bool m_verticalFlip {false};

        inline void copy(quint8 *dst,
                         const quint8 *src,
                         size_t bytes) const
        {
            for (size_t i = 0; i < bytes; ++i)
                dst[i] = src[i];
        }
};

FlipElement::FlipElement(): AkElement()
{
    this->d = new FlipElementPrivate;
}

FlipElement::~FlipElement()
{
    delete this->d;
}

bool FlipElement::horizontalFlip() const
{
    return this->d->m_horizontalFlip;
}

bool FlipElement::verticalFlip() const
{
    return this->d->m_verticalFlip;
}

QString FlipElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Flip/share/qml/main.qml");
}

void FlipElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Flip", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket FlipElement::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet || (!this->d->m_horizontalFlip && !this->d->m_verticalFlip)) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    AkVideoPacket oPacket(packet.caps());
    oPacket.copyMetadata(packet);

    if (this->d->m_horizontalFlip && this->d->m_verticalFlip) {
        auto specs = AkVideoCaps::formatSpecs(packet.caps().format());

        for (int plane = 0; plane < packet.planes(); ++plane) {
            auto pixelSize = specs.plane(plane).pixelSize();
            auto width = packet.caps().width() >> oPacket.widthDiv(plane);

            for (int ys = 0, yd = packet.caps().height() - 1;
                 ys < packet.caps().height();
                 ++ys, --yd) {
                auto srcLine = packet.constLine(plane, ys);
                auto dstLine = oPacket.line(plane, yd);

                for (int xs = 0, xd = width - 1; xs < width; ++xs, --xd)
                    this->d->copy(dstLine + pixelSize * xd,
                                  srcLine + pixelSize * xs,
                                  pixelSize);
            }
        }
    } else if (this->d->m_horizontalFlip) {
        auto specs = AkVideoCaps::formatSpecs(packet.caps().format());

        for (int plane = 0; plane < packet.planes(); ++plane) {
            auto pixelSize = specs.plane(plane).pixelSize();
            auto width = packet.caps().width() >> oPacket.widthDiv(plane);

            for (int y = 0; y < packet.caps().height(); ++y) {
                auto srcLine = packet.constLine(plane, y);
                auto dstLine = oPacket.line(plane, y);

                for (int xs = 0, xd = width - 1; xs < width; ++xs, --xd)
                    this->d->copy(dstLine + pixelSize * xd,
                                  srcLine + pixelSize * xs,
                                  pixelSize);
            }
        }
    } else if (this->d->m_verticalFlip) {
        for (int plane = 0; plane < packet.planes(); ++plane) {
            auto iLineSize = packet.lineSize(plane);
            auto oLineSize = oPacket.lineSize(plane);
            auto lineSize = qMin(iLineSize, oLineSize);

            for (int ys = 0, yd = packet.caps().height() - 1;
                 ys < packet.caps().height();
                 ++ys, --yd) {
                auto srcLine = packet.constLine(plane, ys);
                auto dstLine = oPacket.line(plane, yd);
                memcpy(dstLine, srcLine, lineSize);
            }
        }
    }

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void FlipElement::setHorizontalFlip(bool horizontalFlip)
{
    if (this->d->m_horizontalFlip == horizontalFlip)
        return;

    this->d->m_horizontalFlip = horizontalFlip;
    emit this->horizontalFlipChanged(this->d->m_horizontalFlip);
}

void FlipElement::setVerticalFlip(bool verticalFlip)
{
    if (this->d->m_verticalFlip == verticalFlip)
        return;

    this->d->m_verticalFlip = verticalFlip;
    emit this->verticalFlipChanged(this->d->m_verticalFlip);
}

void FlipElement::resetHorizontalFlip()
{
    this->setHorizontalFlip(false);
}

void FlipElement::resetVerticalFlip()
{
    this->setVerticalFlip(false);
}

#include "moc_flipelement.cpp"
