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

#include <QVector>
#include <QImage>
#include <QQmlContext>
#include <akutils.h>
#include <akpacket.h>

#include "nervouselement.h"

class NervousElementPrivate
{
    public:
        QVector<QImage> m_frames;
        QSize m_frameSize;
        int m_nFrames;
        int m_stride;
        bool m_simple;

        NervousElementPrivate():
            m_nFrames(32),
            m_stride(0),
            m_simple(false)
        {
        }
};

NervousElement::NervousElement(): AkElement()
{
    this->d = new NervousElementPrivate;
}

NervousElement::~NervousElement()
{
    delete this->d;
}

int NervousElement::nFrames() const
{
    return this->d->m_nFrames;
}

bool NervousElement::simple() const
{
    return this->d->m_simple;
}

QString NervousElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Nervous/share/qml/main.qml");
}

void NervousElement::controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Nervous", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void NervousElement::setNFrames(int nFrames)
{
    if (this->d->m_nFrames == nFrames)
        return;

    this->d->m_nFrames = nFrames;
    this->nFramesChanged(nFrames);
}

void NervousElement::setSimple(bool simple)
{
    if (this->d->m_simple == simple)
        return;

    this->d->m_simple = simple;
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

    if (src.size() != this->d->m_frameSize) {
        this->d->m_frames.clear();
        this->d->m_stride = 0;
        this->d->m_frameSize = src.size();
    }

    this->d->m_frames << src.copy();
    int diff = this->d->m_frames.size() - this->d->m_nFrames;

    for (int i = 0; i < diff && !this->d->m_frames.isEmpty(); i++)
        this->d->m_frames.removeFirst();

    if (this->d->m_frames.isEmpty())
        akSend(packet)

    int timer = 0;
    int nFrame = 0;

    if (!this->d->m_simple) {
        if (timer) {
            nFrame += this->d->m_stride;
            nFrame = qBound(0, nFrame, this->d->m_frames.size() - 1);
            timer--;
        } else {
            nFrame = qrand() % this->d->m_frames.size();
            this->d->m_stride = qrand() % 5 - 2;

            if (this->d->m_stride >= 0)
                this->d->m_stride++;

            timer = qrand() % 6 + 2;
        }
    } else if(this->d->m_frames.size() > 0)
        nFrame = qrand() % this->d->m_frames.size();

    QImage oFrame = this->d->m_frames[nFrame];

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}

#include "moc_nervouselement.cpp"
