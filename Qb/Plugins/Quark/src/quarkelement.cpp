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

#include "quarkelement.h"

QuarkElement::QuarkElement(): QbElement()
{
    this->m_nFrames = 16;
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

int QuarkElement::nFrames() const
{
    return this->m_nFrames;
}

void QuarkElement::setNFrames(int nFrames)
{
    if (this->m_nFrames == nFrames)
        return;

    this->m_nFrames = nFrames;
    emit this->nFramesChanged(nFrames);
}

void QuarkElement::resetNFrames()
{
    this->setNFrames(16);
}

QbPacket QuarkElement::iStream(const QbPacket &packet)
{
    QImage src = QbUtils::packetToImage(packet);

    if (src.isNull())
        return QbPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);
    int videoArea = src.width() * src.height();
    QImage oFrame(src.size(), src.format());

    if (src.size() != this->m_frameSize) {
        this->m_frames.clear();
        this->m_frameSize = src.size();
    }

    int nFrames = this->m_nFrames > 0? this->m_nFrames: 1;
    this->m_frames << src.copy();
    int diff = this->m_frames.size() - nFrames;

    for (int i = 0; i < diff; i++)
        this->m_frames.takeFirst();

    QRgb *destBits = (QRgb *) oFrame.bits();

    for (int i = 0; i < videoArea; i++) {
        int frame = qrand() % this->m_frames.size();
        const QRgb *image = (const QRgb *) this->m_frames[frame].constBits();
        destBits[i] = image[i];
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, packet);
    qbSend(oPacket)
}
