/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#include "scrollelement.h"

ScrollElement::ScrollElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgr0");

    this->m_offset = 0;
    this->resetSpeed();
}

QObject *ScrollElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Scroll/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Scroll", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

qreal ScrollElement::speed() const
{
    return this->m_speed;
}

void ScrollElement::addNoise(QImage &dest)
{
    quint32 *destBits = (quint32 *) dest.bits();

    for (int y = this->m_offset, dy = 0; dy < 3 && y < dest.height(); y++, dy++) {
        int i = y * dest.width();

        for (int x = 0; x < dest.width(); x++, i++) {
            if (dy == 2 && qrand() >> 31)
                continue;

            destBits[i] = (qrand() >> 31) ? 0xffffff : 0;
        }
    }
}

void ScrollElement::setSpeed(qreal speed)
{
    if (speed != this->m_speed) {
        this->m_speed = speed;
        emit this->speedChanged();
    }
}

void ScrollElement::resetSpeed()
{
    this->setSpeed(30);
}

QbPacket ScrollElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame = QImage(src.size(), src.format());

    quint32 *srcBits = (quint32 *) src.bits();
    quint32 *destBits = (quint32 *) oFrame.bits();

    if (src.size() != this->m_curSize) {
        this->m_offset = 0;
        this->m_curSize = src.size();
    }

    int offset = this->m_offset;

    memcpy(destBits,
           srcBits + (src.height() - offset) * src.width(),
           sizeof(qint32) * offset * src.width());

    memcpy(destBits + offset * src.width(),
           srcBits,
           sizeof(qint32) * (src.height() - offset) * src.width());

    this->addNoise(oFrame);

    this->m_offset += this->m_speed;

    if ((int) this->m_offset >= src.height())
        this->m_offset = 0;

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
