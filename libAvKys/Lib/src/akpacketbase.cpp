/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#include "akpacketbase.h"
#include "akfrac.h"

class AkPacketBasePrivate
{
    public:
        qint64 m_pts {0};
        AkFrac m_timeBase;
        qint64 m_id {-1};
        int m_index {-1};
};

AkPacketBase::AkPacketBase(QObject *parent):
    QObject(parent)
{
    this->d = new AkPacketBasePrivate();
}

AkPacketBase::AkPacketBase(const AkPacketBase &other):
    QObject()
{
    this->d = new AkPacketBasePrivate();
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_id = other.d->m_id;
    this->d->m_index = other.d->m_index;
}

AkPacketBase::~AkPacketBase()
{
    delete this->d;
}

qint64 AkPacketBase::id() const
{
    return this->d->m_id;
}

qint64 AkPacketBase::pts() const
{
    return this->d->m_pts;
}

AkFrac AkPacketBase::timeBase() const
{
    return this->d->m_timeBase;
}

int AkPacketBase::index() const
{
    return this->d->m_index;
}

void AkPacketBase::copyMetadata(const AkPacketBase &other)
{
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

void AkPacketBase::setId(qint64 id)
{
    if (this->d->m_id == id)
        return;

    this->d->m_id = id;
    emit this->idChanged(id);
}

void AkPacketBase::setPts(qint64 pts)
{
    if (this->d->m_pts == pts)
        return;

    this->d->m_pts = pts;
    emit this->ptsChanged(pts);
}

void AkPacketBase::setTimeBase(const AkFrac &timeBase)
{
    if (this->d->m_timeBase == timeBase)
        return;

    this->d->m_timeBase = timeBase;
    emit this->timeBaseChanged(timeBase);
}

void AkPacketBase::setIndex(int index)
{
    if (this->d->m_index == index)
        return;

    this->d->m_index = index;
    emit this->indexChanged(index);
}

void AkPacketBase::resetId()
{
    this->setId(-1);
}

void AkPacketBase::resetPts()
{
    this->setPts(0);
}

void AkPacketBase::resetTimeBase()
{
    this->setTimeBase({});
}

void AkPacketBase::resetIndex()
{
    this->setIndex(-1);
}

#include "moc_akpacketbase.cpp"
