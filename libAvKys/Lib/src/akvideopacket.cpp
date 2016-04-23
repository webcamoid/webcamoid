/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "akvideopacket.h"

class AkVideoPacketPrivate
{
    public:
        AkVideoCaps m_caps;
};

AkVideoPacket::AkVideoPacket(QObject *parent):
    AkPacket(parent)
{
    this->d = new AkVideoPacketPrivate();
}

AkVideoPacket::AkVideoPacket(const AkVideoCaps &caps,
                             const QByteArray &buffer,
                             qint64 pts,
                             const AkFrac &timeBase,
                             int index,
                             qint64 id)
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = caps;
    this->buffer() = buffer;
    this->pts() = pts;
    this->timeBase() = timeBase;
    this->index() = index;
    this->id() = id;
}

AkVideoPacket::AkVideoPacket(const AkPacket &other)
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = other.caps();
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

AkVideoPacket::AkVideoPacket(const AkVideoPacket &other):
    AkPacket()
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

AkVideoPacket::~AkVideoPacket()
{
    delete this->d;
}

AkVideoPacket &AkVideoPacket::operator =(const AkPacket &other)
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

AkVideoPacket &AkVideoPacket::operator =(const AkVideoPacket &other)
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

AkVideoPacket::operator bool() const
{
    return this->d->m_caps.isValid();
}

AkVideoCaps AkVideoPacket::caps() const
{
    return this->d->m_caps;
}

AkVideoCaps &AkVideoPacket::caps()
{
    return this->d->m_caps;
}

QString AkVideoPacket::toString() const
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

AkPacket AkVideoPacket::toPacket() const
{
    AkPacket packet;
    packet.caps() =  this->d->m_caps.toCaps();
    packet.buffer() = this->buffer();
    packet.pts() = this->pts();
    packet.timeBase() = this->timeBase();
    packet.index() = this->index();
    packet.id() = this->id();

    return packet;
}

void AkVideoPacket::setCaps(const AkVideoCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void AkVideoPacket::resetCaps()
{
    this->setCaps(AkVideoCaps());
}

QDebug operator <<(QDebug debug, const AkVideoPacket &packet)
{
    debug.nospace() << packet.toString().toStdString().c_str();

    return debug.space();
}
