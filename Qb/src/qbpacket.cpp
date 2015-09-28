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

#include "qbpacket.h"

class QbPacketPrivate
{
    public:
        QbCaps m_caps;
        QVariant m_data;
        QbBufferPtr m_buffer;
        ulong m_bufferSize;
        qint64 m_pts;
        QbFrac m_timeBase;
        int m_index;
        qint64 m_id;
};

QbPacket::QbPacket(QObject *parent):
    QObject(parent)
{
    this->d = new QbPacketPrivate();
    this->d->m_bufferSize = 0;
    this->d->m_pts = 0;
    this->d->m_index = -1;
    this->d->m_id = -1;
}

QbPacket::QbPacket(const QbCaps &caps,
                   const QbBufferPtr &buffer,
                   ulong bufferSize,
                   qint64 pts,
                   const QbFrac &timeBase,
                   int index,
                   qint64 id)
{
    this->d = new QbPacketPrivate();
    this->d->m_caps = caps;
    bool isValid = this->d->m_caps.isValid();
    this->d->m_buffer = isValid? buffer: QbBufferPtr();
    this->d->m_bufferSize = isValid? bufferSize: 0;
    this->d->m_pts = isValid? pts: 0;
    this->d->m_timeBase = isValid? timeBase: QbFrac();
    this->d->m_index = isValid? index: -1;
    this->d->m_id = isValid? id: -1;
}

QbPacket::QbPacket(const QbPacket &other):
    QObject()
{
    this->d = new QbPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_data = other.d->m_data;
    this->d->m_buffer = other.d->m_buffer;
    this->d->m_bufferSize = other.d->m_bufferSize;
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

QbPacket::~QbPacket()
{
    delete this->d;
}

QbPacket &QbPacket::operator =(const QbPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_data = other.d->m_data;
        this->d->m_buffer = other.d->m_buffer;
        this->d->m_bufferSize = other.d->m_bufferSize;
        this->d->m_pts = other.d->m_pts;
        this->d->m_timeBase = other.d->m_timeBase;
        this->d->m_index = other.d->m_index;
        this->d->m_id = other.d->m_id;
    }

    return *this;
}

QbPacket::operator bool() const
{
    return this->d->m_caps.isValid();
}

QString QbPacket::toString() const
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
                    << this->d->m_bufferSize
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

QbCaps QbPacket::caps() const
{
    return this->d->m_caps;
}

QbCaps &QbPacket::caps()
{
    return this->d->m_caps;
}

QVariant QbPacket::data() const
{
    return this->d->m_data;
}

QVariant &QbPacket::data()
{
    return this->d->m_data;
}

QbBufferPtr QbPacket::buffer() const
{
    return this->d->m_buffer;
}

QbBufferPtr &QbPacket::buffer()
{
    return this->d->m_buffer;
}

ulong QbPacket::bufferSize() const
{
    return this->d->m_bufferSize;
}

ulong &QbPacket::bufferSize()
{
    return this->d->m_bufferSize;
}

qint64 QbPacket::id() const
{
    return this->d->m_id;
}

qint64 &QbPacket::id()
{
    return this->d->m_id;
}

qint64 QbPacket::pts() const
{
    return this->d->m_pts;
}

qint64 &QbPacket::pts()
{
    return this->d->m_pts;
}

QbFrac QbPacket::timeBase() const
{
    return this->d->m_timeBase;
}

QbFrac &QbPacket::timeBase()
{
    return this->d->m_timeBase;
}

int QbPacket::index() const
{
    return this->d->m_index;
}

int &QbPacket::index()
{
    return this->d->m_index;
}

void QbPacket::setCaps(const QbCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void QbPacket::setData(const QVariant &data)
{
    if (this->d->m_data == data)
        return;

    this->d->m_data = data;
    emit this->dataChanged(data);
}

void QbPacket::setBuffer(const QbBufferPtr &buffer)
{
    if (this->d->m_buffer == buffer)
        return;

    this->d->m_buffer = buffer;
    emit this->bufferChanged(buffer);
}

void QbPacket::setBufferSize(ulong bufferSize)
{
    if (this->d->m_bufferSize == bufferSize)
        return;

    this->d->m_bufferSize = bufferSize;
    emit this->bufferSizeChanged(bufferSize);
}

void QbPacket::setId(qint64 id)
{
    if (this->d->m_id == id)
        return;

    this->d->m_id = id;
    emit this->idChanged(id);
}

void QbPacket::setPts(qint64 pts)
{
    if (this->d->m_pts == pts)
        return;

    this->d->m_pts = pts;
    emit this->ptsChanged(pts);
}

void QbPacket::setTimeBase(const QbFrac &timeBase)
{
    if (this->d->m_timeBase == timeBase)
        return;

    this->d->m_timeBase = timeBase;
    emit this->timeBaseChanged(timeBase);
}

void QbPacket::setIndex(int index)
{
    if (this->d->m_index == index)
        return;

    this->d->m_index = index;
    emit this->indexChanged(index);
}

void QbPacket::resetCaps()
{
    this->setCaps(QbCaps());
}

void QbPacket::resetData()
{
    this->setData(QVariant());
}

void QbPacket::resetBuffer()
{
    this->setBuffer(QbBufferPtr());
}

void QbPacket::resetBufferSize()
{
    this->setBufferSize(0);
}

void QbPacket::resetId()
{
    this->setId(-1);
}

void QbPacket::resetPts()
{
    this->setPts(0);
}

void QbPacket::resetTimeBase()
{
    this->setTimeBase(QbFrac());
}

void QbPacket::resetIndex()
{
    this->setIndex(-1);
}

QDebug operator <<(QDebug debug, const QbPacket &packet)
{
    debug.nospace() << packet.toString().toStdString().c_str();

    return debug.space();
}
