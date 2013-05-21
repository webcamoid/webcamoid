/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "qbpacket.h"

QbPacket::QbPacket(QObject *parent): QObject(parent)
{
    this->resetCaps();
    this->resetBuffer();
    this->resetBufferSize();
    this->resetPts();
    this->resetDuration();
    this->resetTimeBase();
    this->resetIndex();
}

QbPacket::QbPacket(QbCaps caps,
                   const QSharedPointer<uchar> &buffer,
                   ulong bufferSize,
                   int64_t pts,
                   int duration,
                   QbFrac timeBase,
                   int index)
{
    this->setCaps(caps);
    bool isValid = this->caps().isValid();
    this->setBuffer(isValid? buffer: QSharedPointer<uchar>());
    this->setBufferSize(isValid? bufferSize: 0);
    this->setPts(isValid? pts: 0);
    this->setDuration(isValid? duration: 0);
    this->setTimeBase(isValid? timeBase: QbFrac());
    this->setIndex(isValid? index: -1);
}

QbPacket::QbPacket(const QbPacket &other):
    QObject(other.parent()),
    m_caps(other.m_caps),
    m_buffer(other.m_buffer),
    m_bufferSize(other.m_bufferSize),
    m_pts(other.m_pts),
    m_duration(other.m_duration),
    m_timeBase(other.m_timeBase),
    m_index(other.m_index)
{
}

QbPacket::~QbPacket()
{
}

QbPacket &QbPacket::operator =(const QbPacket &other)
{
    if (this != &other)
    {
        this->m_caps = other.m_caps;
        this->m_buffer = other.m_buffer;
        this->m_bufferSize = other.m_bufferSize;
        this->m_pts = other.m_pts;
        this->m_duration = other.m_duration;
        this->m_timeBase = other.m_timeBase;
        this->m_index = other.m_index;
    }

    return *this;
}

QString QbPacket::toString() const
{
    QString packetInfo = QString("Caps       : %1\n"
                                 "Buffer Size: %2\n"
                                 "Pts        : %3\n"
                                 "Duration   : %4\n"
                                 "Time Base  : %5\n"
                                 "Index      : %6\n").arg(this->caps().toString())
                                                     .arg(this->bufferSize())
                                                     .arg(this->pts())
                                                     .arg(this->duration())
                                                     .arg(this->timeBase().toString())
                                                     .arg(this->index());

    return packetInfo;
}

QbCaps QbPacket::caps() const
{
    return this->m_caps;
}

QSharedPointer<uchar> QbPacket::buffer() const
{
    return this->m_buffer;
}

ulong QbPacket::bufferSize() const
{
    return this->m_bufferSize;
}

int64_t QbPacket::pts() const
{
    return this->m_pts;
}

int QbPacket::duration() const
{
    return this->m_duration;
}

QbFrac QbPacket::timeBase() const
{
    return this->m_timeBase;
}

int QbPacket::index() const
{
    return this->m_index;
}

void QbPacket::setCaps(QbCaps mimeType)
{
    this->m_caps = mimeType;
}

void QbPacket::setBuffer(const QSharedPointer<uchar> &buffer)
{
    this->m_buffer = buffer;
}

void QbPacket::setBufferSize(ulong bufferSize)
{
    this->m_bufferSize = bufferSize;
}

void QbPacket::setPts(int64_t pts)
{
    this->m_pts = pts;
}

void QbPacket::setDuration(int duration)
{
    this->m_duration = duration;
}

void QbPacket::setTimeBase(QbFrac timeBase)
{
    this->m_timeBase = timeBase;
}

void QbPacket::setIndex(int index)
{
    this->m_index = index;
}

void QbPacket::resetCaps()
{
    this->setCaps(QbCaps());
}

void QbPacket::resetBuffer()
{
    this->setBuffer(QSharedPointer<uchar>());
}

void QbPacket::resetBufferSize()
{
    this->setBufferSize(0);
}

void QbPacket::resetPts()
{
    this->setPts(0);
}

void QbPacket::resetDuration()
{
    this->setDuration(0);
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
    debug.nospace() << packet.toString();

    return debug.space();
}
