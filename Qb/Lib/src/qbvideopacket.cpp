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

#include "qbvideopacket.h"

class QbVideoPacketPrivate
{
    public:
        QbVideoCaps m_caps;
};

QbVideoPacket::QbVideoPacket(QObject *parent):
    QbPacket(parent)
{
    this->d = new QbVideoPacketPrivate();
}

QbVideoPacket::QbVideoPacket(const QbVideoCaps &caps,
                             const QByteArray &buffer,
                             qint64 pts,
                             const QbFrac &timeBase,
                             int index,
                             qint64 id)
{
    this->d = new QbVideoPacketPrivate();
    this->d->m_caps = caps;
    this->buffer() = buffer;
    this->pts() = pts;
    this->timeBase() = timeBase;
    this->index() = index;
    this->id() = id;
}

QbVideoPacket::QbVideoPacket(const QbPacket &other)
{
    this->d = new QbVideoPacketPrivate();
    this->d->m_caps = other.caps();
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

QbVideoPacket::QbVideoPacket(const QbVideoPacket &other):
    QbPacket()
{
    this->d = new QbVideoPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

QbVideoPacket::~QbVideoPacket()
{
    delete this->d;
}

QbVideoPacket &QbVideoPacket::operator =(const QbPacket &other)
{
    this->d->m_caps = other.caps();
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();

    return *this;
}

QbVideoPacket &QbVideoPacket::operator =(const QbVideoPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->data() = other.data();
        this->buffer() = other.buffer();
        this->pts() = other.pts();
        this->timeBase() = other.timeBase();
        this->index() = other.index();
        this->id() = other.id();
    }

    return *this;
}

QbVideoPacket::operator bool() const
{
    return this->d->m_caps.isValid();
}

QbVideoCaps QbVideoPacket::caps() const
{
    return this->d->m_caps;
}

QbVideoCaps &QbVideoPacket::caps()
{
    return this->d->m_caps;
}

QString QbVideoPacket::toString() const
{
    QString packetInfo;
    QDebug debug(&packetInfo);

    debug.nospace() << "Caps       : "
                    << this->d->m_caps.toString().toStdString().c_str()
                    << "\n";

    debug.nospace() << "Data       : "
                    << this->data()
                    << "\n";

    debug.nospace() << "Buffer Size: "
                    << this->buffer().size()
                    << "\n";

    debug.nospace() << "Id         : "
                    << this->id()
                    << "\n";

    debug.nospace() << "Pts        : "
                    << this->pts()
                    << " ("
                    << this->pts() * this->timeBase().value()
                    << ")\n";

    debug.nospace() << "Time Base  : "
                    << this->timeBase().toString().toStdString().c_str()
                    << "\n";

    debug.nospace() << "Index      : "
                    << this->index();

    return packetInfo;
}

QbPacket QbVideoPacket::toPacket() const
{
    QbPacket packet;
    packet.caps() =  this->d->m_caps.toCaps();
    packet.buffer() = this->buffer();
    packet.pts() = this->pts();
    packet.timeBase() = this->timeBase();
    packet.index() = this->index();
    packet.id() = this->id();

    return packet;
}

void QbVideoPacket::setCaps(const QbVideoCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void QbVideoPacket::resetCaps()
{
    this->setCaps(QbVideoCaps());
}

QDebug operator <<(QDebug debug, const QbVideoPacket &packet)
{
    debug.nospace() << packet.toString().toStdString().c_str();

    return debug.space();
}
