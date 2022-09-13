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

#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "pixelateelement.h"

class PixelateElementPrivate
{
    public:
        QSize m_blockSize;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};

        PixelateElementPrivate():
            m_blockSize(QSize(8, 8))
        {
        }
};

PixelateElement::PixelateElement(): AkElement()
{
    this->d = new PixelateElementPrivate;
}

PixelateElement::~PixelateElement()
{
    delete this->d;
}

QSize PixelateElement::blockSize() const
{
    return this->d->m_blockSize;
}

QString PixelateElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Pixelate/share/qml/main.qml");
}

void PixelateElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Pixelate", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket PixelateElement::iVideoStream(const AkVideoPacket &packet)
{
    auto blockSize = this->d->m_blockSize;

    if (blockSize.isEmpty()) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    auto oFrame = this->d->m_videoConverter.convertToImage(packet);

    if (oFrame.isNull())
        return AkPacket();

    qreal sw = 1.0 / blockSize.width();
    qreal sh = 1.0 / blockSize.height();

    oFrame = oFrame.scaled(int(sw * oFrame.width()),
                           int(sh * oFrame.height()),
                           Qt::IgnoreAspectRatio,
                           Qt::FastTransformation)
                   .scaled(oFrame.width(),
                           oFrame.height(),
                           Qt::IgnoreAspectRatio,
                           Qt::FastTransformation);

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void PixelateElement::setBlockSize(const QSize &blockSize)
{
    if (blockSize == this->d->m_blockSize)
        return;

    this->d->m_blockSize = blockSize;
    emit this->blockSizeChanged(blockSize);
}

void PixelateElement::resetBlockSize()
{
    this->setBlockSize(QSize(8, 8));
}

#include "moc_pixelateelement.cpp"
