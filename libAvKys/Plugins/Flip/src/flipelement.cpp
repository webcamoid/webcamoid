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

#include <QImage>
#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "flipelement.h"

class FlipElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};
        bool m_horizontalFlip {false};
        bool m_verticalFlip {false};
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
    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return {};

    auto oFrame =
            src.mirrored(this->d->m_horizontalFlip, this->d->m_verticalFlip);
    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

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
