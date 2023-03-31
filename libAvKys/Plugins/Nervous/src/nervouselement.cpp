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

#include <QDateTime>
#include <QQmlContext>
#include <QRandomGenerator>
#include <QSize>
#include <QVector>
#include <akpacket.h>
#include <akvideopacket.h>

#include "nervouselement.h"

class NervousElementPrivate
{
    public:
        QVector<AkVideoPacket> m_frames;
        QSize m_frameSize;
        int m_nFrames {32};
        int m_stride {0};
        bool m_simple {false};
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

AkPacket NervousElement::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    QSize frameSize(packet.caps().width(), packet.caps().height());

    if (frameSize != this->d->m_frameSize) {
        this->d->m_frames.clear();
        this->d->m_stride = 0;
        this->d->m_frameSize = frameSize;
    }

    this->d->m_frames << packet;
    int diff = this->d->m_frames.size() - this->d->m_nFrames;

    for (int i = 0; i < diff && !this->d->m_frames.isEmpty(); i++)
        this->d->m_frames.removeFirst();

    if (this->d->m_frames.isEmpty()) {
        emit this->oStream(packet);

        return packet;
    }

    int nFrame = 0;

    if (!this->d->m_simple) {
        static int timer = 0;

        if (timer) {
            nFrame += this->d->m_stride;
            nFrame = qBound(0, nFrame, this->d->m_frames.size() - 1);
            timer--;
        } else {
            nFrame = QRandomGenerator::global()->bounded(this->d->m_frames.size());
            this->d->m_stride = QRandomGenerator::global()->bounded(2, 6);

            if (this->d->m_stride >= 0)
                this->d->m_stride++;

            timer = QRandomGenerator::global()->bounded(2, 8);
        }
    } else if(!this->d->m_frames.isEmpty()) {
        nFrame = QRandomGenerator::global()->bounded(this->d->m_frames.size());
    }

    auto dst = this->d->m_frames[nFrame];
    dst.copyMetadata(packet);

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void NervousElement::setNFrames(int nFrames)
{
    if (this->d->m_nFrames == nFrames)
        return;

    this->d->m_nFrames = nFrames;
    emit this->nFramesChanged(nFrames);
}

void NervousElement::setSimple(bool simple)
{
    if (this->d->m_simple == simple)
        return;

    this->d->m_simple = simple;
    emit this->simpleChanged(simple);
}

void NervousElement::resetNFrames()
{
    this->setNFrames(32);
}

void NervousElement::resetSimple()
{
    this->setSimple(false);
}

#include "moc_nervouselement.cpp"
