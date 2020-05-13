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

#include <QDebug>
#include <QVariant>
#include <QQmlEngine>

#include "akpacket.h"
#include "akcaps.h"
#include "akfrac.h"

class AkPacketPrivate
{
    public:
        AkCaps m_caps;
        QByteArray m_buffer;
        qint64 m_pts {0};
        AkFrac m_timeBase;
        qint64 m_id {-1};
        int m_index {-1};
};

AkPacket::AkPacket(QObject *parent):
    QObject(parent)
{
    this->d = new AkPacketPrivate();
}

AkPacket::AkPacket(const AkCaps &caps)
{
    this->d = new AkPacketPrivate();
    this->d->m_caps = caps;
}

AkPacket::AkPacket(const AkPacket &other):
    QObject()
{
    this->d = new AkPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_buffer = other.d->m_buffer;
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

AkPacket::~AkPacket()
{
    delete this->d;
}

AkPacket &AkPacket::operator =(const AkPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_buffer = other.d->m_buffer;
        this->d->m_pts = other.d->m_pts;
        this->d->m_timeBase = other.d->m_timeBase;
        this->d->m_index = other.d->m_index;
        this->d->m_id = other.d->m_id;
    }

    return *this;
}

AkPacket::operator bool() const
{
    return this->d->m_caps && !this->d->m_buffer.isEmpty();
}

AkCaps AkPacket::caps() const
{
    return this->d->m_caps;
}

AkCaps &AkPacket::caps()
{
    return this->d->m_caps;
}

QByteArray AkPacket::buffer() const
{
    return this->d->m_buffer;
}

QByteArray &AkPacket::buffer()
{
    return this->d->m_buffer;
}

qint64 AkPacket::id() const
{
    return this->d->m_id;
}

qint64 &AkPacket::id()
{
    return this->d->m_id;
}

qint64 AkPacket::pts() const
{
    return this->d->m_pts;
}

qint64 &AkPacket::pts()
{
    return this->d->m_pts;
}

AkFrac AkPacket::timeBase() const
{
    return this->d->m_timeBase;
}

AkFrac &AkPacket::timeBase()
{
    return this->d->m_timeBase;
}

int AkPacket::index() const
{
    return this->d->m_index;
}

int &AkPacket::index()
{
    return this->d->m_index;
}

void AkPacket::copyMetadata(const AkPacket &other)
{
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

void AkPacket::setCaps(const AkCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void AkPacket::setBuffer(const QByteArray &buffer)
{
    if (this->d->m_buffer == buffer)
        return;

    this->d->m_buffer = buffer;
    emit this->bufferChanged(buffer);
}

void AkPacket::setId(qint64 id)
{
    if (this->d->m_id == id)
        return;

    this->d->m_id = id;
    emit this->idChanged(id);
}

void AkPacket::setPts(qint64 pts)
{
    if (this->d->m_pts == pts)
        return;

    this->d->m_pts = pts;
    emit this->ptsChanged(pts);
}

void AkPacket::setTimeBase(const AkFrac &timeBase)
{
    if (this->d->m_timeBase == timeBase)
        return;

    this->d->m_timeBase = timeBase;
    emit this->timeBaseChanged(timeBase);
}

void AkPacket::setIndex(int index)
{
    if (this->d->m_index == index)
        return;

    this->d->m_index = index;
    emit this->indexChanged(index);
}

void AkPacket::resetCaps()
{
    this->setCaps(AkCaps());
}

void AkPacket::resetBuffer()
{
    this->setBuffer({});
}

void AkPacket::resetId()
{
    this->setId(-1);
}

void AkPacket::resetPts()
{
    this->setPts(0);
}

void AkPacket::resetTimeBase()
{
    this->setTimeBase({});
}

void AkPacket::resetIndex()
{
    this->setIndex(-1);
}

void AkPacket::registerTypes()
{
    qRegisterMetaType<AkPacket>("AkPacket");
    qmlRegisterSingletonType<AkPacket>("Ak", 1, 0, "AkPacket",
                                       [] (QQmlEngine *qmlEngine,
                                           QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkPacket();
    });
}

QDebug operator <<(QDebug debug, const AkPacket &packet)
{
    debug.nospace() << "AkPacket("
                    << "caps="
                    << packet.caps()
                    << ",bufferSize="
                    << packet.buffer().size()
                    << ",id="
                    << packet.id()
                    << ",pts="
                    << packet.pts()
                    << "("
                    << packet.pts() * packet.timeBase().value()
                    << ")"
                    << ",timeBase="
                    << packet.timeBase()
                    << ",index="
                    << packet.index()
                    << ")";

    return debug.space();
}

#include "moc_akpacket.cpp"
