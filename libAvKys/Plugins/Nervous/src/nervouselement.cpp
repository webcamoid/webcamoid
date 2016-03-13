/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "nervouselement.h"

NervousElement::NervousElement(): AkElement()
{
    this->m_nFrames = 32;
    this->m_simple = false;
    this->m_stride = 0;
}

QObject *NervousElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Nervous/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Nervous", (QObject *) this);
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

int NervousElement::nFrames() const
{
    return this->m_nFrames;
}

bool NervousElement::simple() const
{
    return this->m_simple;
}

void NervousElement::setNFrames(int nFrames)
{
    if (this->m_nFrames == nFrames)
        return;

    this->m_nFrames = nFrames;
    this->nFramesChanged(nFrames);
}

void NervousElement::setSimple(bool simple)
{
    if (this->m_simple == simple)
        return;

    this->m_simple = simple;
    this->simpleChanged(simple);
}

void NervousElement::resetNFrames()
{
    this->setNFrames(32);
}

void NervousElement::resetSimple()
{
    this->setSimple(false);
}

AkPacket NervousElement::iStream(const AkPacket &packet)
{
    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    if (src.size() != this->m_frameSize) {
        this->m_frames.clear();
        this->m_stride = 0;
        this->m_frameSize = src.size();
    }

    this->m_frames << src.copy();
    int diff = this->m_frames.size() - this->m_nFrames;

    for (int i = 0; i < diff && !this->m_frames.isEmpty(); i++)
        this->m_frames.takeFirst();

    if (this->m_frames.isEmpty())
        akSend(packet)

    int timer = 0;
    int nFrame = 0;

    if (!this->m_simple) {
        if (timer) {
            nFrame += this->m_stride;
            nFrame = qBound(0, nFrame, this->m_frames.size() - 1);
            timer--;
        } else {
            nFrame = qrand() % this->m_frames.size();
            this->m_stride = qrand() % 5 - 2;

            if (this->m_stride >= 0)
                this->m_stride++;

            timer = qrand() % 6 + 2;
        }
    } else if(this->m_frames.size() > 0)
        nFrame = qrand() % this->m_frames.size();

    QImage oFrame = this->m_frames[nFrame];

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
