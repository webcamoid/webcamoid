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

#include "akaudiopacket.h"

class AkAudioPacketPrivate
{
    public:
        AkAudioCaps m_caps;
};

AkAudioPacket::AkAudioPacket(QObject *parent):
    AkPacket(parent)
{
    this->d = new AkAudioPacketPrivate();
}

AkAudioPacket::AkAudioPacket(const AkAudioCaps &caps,
                             const QByteArray &buffer,
                             qint64 pts,
                             const AkFrac &timeBase,
                             int index,
                             qint64 id)
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = caps;
    this->buffer() = buffer;
    this->pts() = pts;
    this->timeBase() = timeBase;
    this->index() = index;
    this->id() = id;
}

AkAudioPacket::AkAudioPacket(const AkPacket &other)
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = other.caps();
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

AkAudioPacket::AkAudioPacket(const AkAudioPacket &other):
    AkPacket()
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->data() = other.data();
    this->buffer() = other.buffer();
    this->pts() = other.pts();
    this->timeBase() = other.timeBase();
    this->index() = other.index();
    this->id() = other.id();
}

AkAudioPacket::~AkAudioPacket()
{
    delete this->d;
}

AkAudioPacket &AkAudioPacket::operator =(const AkPacket &other)
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

AkAudioPacket &AkAudioPacket::operator =(const AkAudioPacket &other)
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

AkAudioPacket::operator bool() const
{
    return this->d->m_caps.isValid();
}

AkAudioCaps AkAudioPacket::caps() const
{
    return this->d->m_caps;
}

AkAudioCaps &AkAudioPacket::caps()
{
    return this->d->m_caps;
}

QString AkAudioPacket::toString() const
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

AkPacket AkAudioPacket::toPacket() const
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

void AkAudioPacket::setCaps(const AkAudioCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void AkAudioPacket::resetCaps()
{
    this->setCaps(AkAudioCaps());
}

QDebug operator <<(QDebug debug, const AkAudioPacket &packet)
{
    debug.nospace() << packet.toString().toStdString().c_str();

    return debug.space();
}
