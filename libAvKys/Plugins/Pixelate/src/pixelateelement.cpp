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

#include <QMutex>
#include <QQmlContext>
#include <QSize>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "pixelateelement.h"

class PixelateElementPrivate
{
    public:
        QSize m_blockSize {8, 8};
        QMutex m_mutex;
        AkVideoConverter m_videoConverter;
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
    this->d->m_mutex.lock();
    auto blockSize = this->d->m_blockSize;
    this->d->m_mutex.unlock();

    if (blockSize.isEmpty()) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    auto dcaps = packet.caps();
    dcaps.setWidth(packet.caps().width() / blockSize.width());
    dcaps.setHeight(packet.caps().height() / blockSize.height());
    this->d->m_videoConverter.setOutputCaps(dcaps);
    auto downScaled = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.setOutputCaps(packet.caps());
    auto dst = this->d->m_videoConverter.convert(downScaled);
    this->d->m_videoConverter.end();

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void PixelateElement::setBlockSize(const QSize &blockSize)
{
    if (blockSize == this->d->m_blockSize)
        return;

    this->d->m_mutex.lock();
    this->d->m_blockSize = blockSize;
    this->d->m_mutex.unlock();
    emit this->blockSizeChanged(blockSize);
}

void PixelateElement::resetBlockSize()
{
    this->setBlockSize({8, 8});
}

#include "moc_pixelateelement.cpp"
