/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "quarkelement.h"

QuarkElement::QuarkElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetPlanes();
    this->m_plane = 0;
}

QObject *QuarkElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Quark/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Quark", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
    context->setParent(item);

    return item;
}

int QuarkElement::planes() const
{
    return this->m_planes;
}

void QuarkElement::setPlanes(int planes)
{
    if (planes != this->m_planes) {
        this->m_planes = planes;
        emit this->planesChanged();
    }
}

void QuarkElement::resetPlanes()
{
    this->setPlanes(16);
}

QbPacket QuarkElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    int videoArea = src.width() * src.height();

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    static int planes = -1;

    if (packet.caps() != this->m_caps
        || this->m_planes != planes) {
        planes = this->m_planes;
        this->m_buffer = QImage(src.width(), src.height() * planes, QImage::Format_RGB32);
        this->m_planeTable.resize(planes);

        for(int i = 0; i < planes; i++)
            this->m_planeTable[i] = (QRgb *) this->m_buffer.bits() + videoArea * i;

        this->m_plane = planes - 1;
        this->m_caps = packet.caps();
    }

    if (planes < 1)
        qbSend(packet)

    memcpy(this->m_planeTable[this->m_plane], srcBits, src.bytesPerLine() * src.height());

    for (int i = 0; i < videoArea; i++) {
        int cf = (qrand() >> 24) & (planes - 1);
        destBits[i] = this->m_planeTable[cf][i];
    }

    this->m_plane--;

    if (this->m_plane < 0)
        this->m_plane = planes - 1;

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
