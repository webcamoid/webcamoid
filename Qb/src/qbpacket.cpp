/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "qbpacket.h"

QbPacket::QbPacket(QObject *parent): QObject(parent)
{
    this->resetCaps();
    this->resetBuffer();
    this->resetBufferSize();
    this->resetId();
    this->resetPts();
    this->resetTimeBase();
    this->resetIndex();
}

QbPacket::QbPacket(const QbCaps &caps,
                   const QbBufferPtr &buffer,
                   ulong bufferSize,
                   qint64 pts,
                   const QbFrac &timeBase,
                   int index,
                   qint64 id)
{
    this->setCaps(caps);
    bool isValid = this->caps().isValid();
    this->setBuffer(isValid? buffer: QbBufferPtr());
    this->setBufferSize(isValid? bufferSize: 0);
    this->setPts(isValid? pts: 0);
    this->setTimeBase(isValid? timeBase: QbFrac());
    this->setIndex(isValid? index: -1);
    this->setId(isValid? id: -1);
}

QbPacket::QbPacket(const QbPacket &other):
    QObject(other.parent()),
    m_caps(other.m_caps),
    m_data(other.m_data),
    m_buffer(other.m_buffer),
    m_bufferSize(other.m_bufferSize),
    m_pts(other.m_pts),
    m_timeBase(other.m_timeBase),
    m_index(other.m_index),
    m_id(other.m_id)
{
}

QbPacket::~QbPacket()
{
}

QbPacket &QbPacket::operator =(const QbPacket &other)
{
    if (this != &other) {
        this->m_caps = other.m_caps;
        this->m_data = other.m_data;
        this->m_buffer = other.m_buffer;
        this->m_bufferSize = other.m_bufferSize;
        this->m_pts = other.m_pts;
        this->m_timeBase = other.m_timeBase;
        this->m_index = other.m_index;
        this->m_id = other.m_id;
    }

    return *this;
}

QbPacket::operator bool() const
{
    return this->m_caps.isValid();
}

QString QbPacket::toString() const
{
    QString packetInfo;
    QDebug debug(&packetInfo);

    debug.nospace() << "Caps       : "
                    << this->caps().toString().toStdString().c_str()
                    << "\n";

    debug.nospace() << "Data       : "
                    << this->data()
                    << "\n";

    debug.nospace() << "Buffer Size: "
                    << this->bufferSize()
                    << "\n";

    debug.nospace() << "Id         : "
                    << this->id()
                    << "\n";

    debug.nospace() << "Pts        : "
                    << this->pts() * this->timeBase().value()
                    << "\n";

    debug.nospace() << "Time Base  : "
                    << this->timeBase().toString().toStdString().c_str()
                    << "\n";

    debug.nospace() << "Index      : "
                    << this->index();

    return packetInfo;
}

QbCaps QbPacket::caps() const
{
    return this->m_caps;
}

QbCaps &QbPacket::caps()
{
    return this->m_caps;
}

QVariant QbPacket::data() const
{
    return this->m_data;
}

QVariant &QbPacket::data()
{
    return this->m_data;
}

QbBufferPtr QbPacket::buffer() const
{
    return this->m_buffer;
}

QbBufferPtr &QbPacket::buffer()
{
    return this->m_buffer;
}

ulong QbPacket::bufferSize() const
{
    return this->m_bufferSize;
}

ulong &QbPacket::bufferSize()
{
    return this->m_bufferSize;
}

qint64 QbPacket::id() const
{
    return this->m_id;
}

qint64 &QbPacket::id()
{
    return this->m_id;
}

qint64 QbPacket::pts() const
{
    return this->m_pts;
}

qint64 &QbPacket::pts()
{
    return this->m_pts;
}

QbFrac QbPacket::timeBase() const
{
    return this->m_timeBase;
}

QbFrac &QbPacket::timeBase()
{
    return this->m_timeBase;
}

int QbPacket::index() const
{
    return this->m_index;
}

int &QbPacket::index()
{
    return this->m_index;
}

void QbPacket::setCaps(const QbCaps &mimeType)
{
    this->m_caps = mimeType;
    emit this->capsChanged();
}

void QbPacket::setData(const QVariant &data)
{
    this->m_data = data;
    emit this->dataChanged();
}

void QbPacket::setBuffer(const QbBufferPtr &buffer)
{
    this->m_buffer = buffer;
    emit this->bufferChanged();
}

void QbPacket::setBufferSize(ulong bufferSize)
{
    this->m_bufferSize = bufferSize;
    emit this->bufferSizeChanged();
}

void QbPacket::setId(qint64 id)
{
    this->m_id = id;
    emit this->idChanged();
}

void QbPacket::setPts(qint64 pts)
{
    this->m_pts = pts;
    emit this->ptsChanged();
}

void QbPacket::setTimeBase(const QbFrac &timeBase)
{
    this->m_timeBase = timeBase;
    emit this->timeBaseChanged();
}

void QbPacket::setIndex(int index)
{
    this->m_index = index;
    emit this->indexChanged();
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
