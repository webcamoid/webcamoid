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

#include "akpacket.h"

class AkPacketPrivate
{
    public:
        AkCaps m_caps;
        QVariant m_data;
        QByteArray m_buffer;
        qint64 m_pts;
        AkFrac m_timeBase;
        int m_index;
        qint64 m_id;
};

AkPacket::AkPacket(QObject *parent):
    QObject(parent)
{
    this->d = new AkPacketPrivate();
    this->d->m_pts = 0;
    this->d->m_index = -1;
    this->d->m_id = -1;
}

AkPacket::AkPacket(const AkCaps &caps,
                   const QByteArray &buffer,
                   qint64 pts,
                   const AkFrac &timeBase,
                   int index,
                   qint64 id)
{
    this->d = new AkPacketPrivate();
    this->d->m_caps = caps;
    bool isValid = this->d->m_caps.isValid();
    this->d->m_buffer = isValid? buffer: QByteArray();
    this->d->m_pts = isValid? pts: 0;
    this->d->m_timeBase = isValid? timeBase: AkFrac();
    this->d->m_index = isValid? index: -1;
    this->d->m_id = isValid? id: -1;
}

AkPacket::AkPacket(const AkPacket &other):
    QObject()
{
    this->d = new AkPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_data = other.d->m_data;
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
        this->d->m_data = other.d->m_data;
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
    return this->d->m_caps.isValid();
}

QString AkPacket::toString() const
{
    QString packetInfo;
    QDebug debug(&packetInfo);

    debug.nospace() << "Caps       : "
                    << this->d->m_caps.toString().toStdString().c_str()
                    << "\n";

    debug.nospace() << "Data       : "
                    << this->d->m_data
                    << "\n";

    debug.nospace() << "Buffer Size: "
                    << this->d->m_buffer.size()
                    << "\n";

    debug.nospace() << "Id         : "
                    << this->d->m_id
                    << "\n";

    debug.nospace() << "Pts        : "
                    << this->d->m_pts
                    << " ("
                    << this->d->m_pts * this->d->m_timeBase.value()
                    << ")\n";

    debug.nospace() << "Time Base  : "
                    << this->d->m_timeBase.toString().toStdString().c_str()
                    << "\n";

    debug.nospace() << "Index      : "
                    << this->d->m_index;

    return packetInfo;
}

AkCaps AkPacket::caps() const
{
    return this->d->m_caps;
}

AkCaps &AkPacket::caps()
{
    return this->d->m_caps;
}

QVariant AkPacket::data() const
{
    return this->d->m_data;
}

QVariant &AkPacket::data()
{
    return this->d->m_data;
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

void AkPacket::setCaps(const AkCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void AkPacket::setData(const QVariant &data)
{
    if (this->d->m_data == data)
        return;

    this->d->m_data = data;
    emit this->dataChanged(data);
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

void AkPacket::resetData()
{
    this->setData(QVariant());
}

void AkPacket::resetBuffer()
{
    this->setBuffer(QByteArray());
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
    this->setTimeBase(AkFrac());
}

void AkPacket::resetIndex()
{
    this->setIndex(-1);
}

QDebug operator <<(QDebug debug, const AkPacket &packet)
{
    debug.nospace() << packet.toString().toStdString().c_str();

    return debug.space();
}
