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
    this->resetData();
    this->resetDataSize();
    this->resetDts();
    this->resetPts();
    this->resetDuration();
    this->resetIndex();
}

QbPacket::QbPacket(QbCaps caps, const void *data,
                   ulong dataSize,
                   int64_t dts,
                   int64_t pts,
                   int duration,
                   int index)
{
    this->setCaps(caps);
    bool isValid = this->caps().isValid();
    this->setData(isValid? data: NULL);
    this->setDataSize(isValid? dataSize: 0);
    this->setDts(isValid? dts: 0);
    this->setPts(isValid? pts: 0);
    this->setDuration(isValid? duration: 0);
    this->setIndex(isValid? index: -1);
}

QbPacket::QbPacket(const QbPacket &other):
    QObject(NULL),
    m_caps(other.m_caps),
    m_data(other.m_data),
    m_dataSize(other.m_dataSize),
    m_dts(other.m_pts),
    m_pts(other.m_pts),
    m_duration(other.m_duration),
    m_index(other.m_index)
{
}

QbPacket &QbPacket::operator =(const QbPacket &other)
{
    if (this != &other)
    {
        this->m_caps = other.m_caps;
        this->m_data = other.m_data;
        this->m_dataSize = other.m_dataSize;
        this->m_dts = other.m_dts;
        this->m_pts = other.m_pts;
        this->m_duration = other.m_duration;
        this->m_index = other.m_index;
    }

    return *this;
}

QbCaps QbPacket::caps() const
{
    return this->m_caps;
}

const void *QbPacket::data() const
{
    return this->m_data;
}

ulong QbPacket::dataSize() const
{
    return this->m_dataSize;
}

int64_t QbPacket::dts() const
{
    return this->m_dts;
}

int64_t QbPacket::pts() const
{
    return this->m_pts;
}

int QbPacket::duration() const
{
    return this->m_duration;
}

int QbPacket::index() const
{
    return this->m_index;
}

void QbPacket::setCaps(QbCaps mimeType)
{
    this->m_caps = mimeType;
}

void QbPacket::setData(const void *data)
{
    this->m_data = data;
}

void QbPacket::setDataSize(ulong dataSize)
{
    this->m_dataSize = dataSize;
}

void QbPacket::setDts(int64_t dts)
{
    this->m_dts = dts;
}

void QbPacket::setPts(int64_t pts)
{
    this->m_pts = pts;
}

void QbPacket::setDuration(int duration)
{
    this->m_duration = duration;
}

void QbPacket::setIndex(int index)
{
    this->m_index = index;
}

void QbPacket::resetCaps()
{
    this->setCaps(QbCaps());
}

void QbPacket::resetData()
{
    this->setData(NULL);
}

void QbPacket::resetDataSize()
{
    this->setDataSize(0);
}

void QbPacket::resetDts()
{
    this->setDts(0);
}

void QbPacket::resetPts()
{
    this->setPts(0);
}

void QbPacket::resetDuration()
{
    this->setDuration(0);
}

void QbPacket::resetIndex()
{
    this->setIndex(-1);
}
