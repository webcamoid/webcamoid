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

#include "pixelateelement.h"

PixelateElement::PixelateElement(): AkElement()
{
    this->m_blockSize = QSize(8, 8);
}

QObject *PixelateElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Pixelate/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Pixelate", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QSize PixelateElement::blockSize() const
{
    return this->m_blockSize;
}

void PixelateElement::setBlockSize(const QSize &blockSize)
{
    if (blockSize == this->m_blockSize)
        return;

    this->m_blockSize = blockSize;
    emit this->blockSizeChanged(blockSize);
}

void PixelateElement::resetBlockSize()
{
    this->setBlockSize(QSize(8, 8));
}

AkPacket PixelateElement::iStream(const AkPacket &packet)
{
    QSize blockSize = this->m_blockSize;

    if (blockSize.isEmpty())
        akSend(packet)

    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame = src.convertToFormat(QImage::Format_ARGB32);

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

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
