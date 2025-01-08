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
#include <QQmlEngine>

#include "akcompressedaudiopacket.h"
#include "akcompressedaudiocaps.h"
#include "akfrac.h"
#include "akpacket.h"
#include "akcompressedpacket.h"

class AkCompressedAudioPacketPrivate
{
    public:
        AkCompressedAudioCaps m_caps;
        QByteArray m_data;
        AkCompressedAudioPacket::AudioPacketTypeFlag m_flags {AkCompressedAudioPacket::AudioPacketTypeFlag_None};
};

AkCompressedAudioPacket::AkCompressedAudioPacket(QObject *parent):
    AkPacketBase(parent)
{
    this->d = new AkCompressedAudioPacketPrivate();
}

AkCompressedAudioPacket::AkCompressedAudioPacket(const AkCompressedAudioCaps &caps,
                                                 size_t size,
                                                 bool initialized):
    AkPacketBase()
{
    this->d = new AkCompressedAudioPacketPrivate();
    this->d->m_caps = caps;

    if (initialized)
        this->d->m_data = QByteArray(int(size), 0);
    else
        this->d->m_data = QByteArray(int(size), Qt::Uninitialized);
}

AkCompressedAudioPacket::AkCompressedAudioPacket(const AkPacket &other):
    AkPacketBase(other)
{
    this->d = new AkCompressedAudioPacketPrivate();

    if (other.type() == AkPacket::PacketAudioCompressed) {
        auto data = reinterpret_cast<AkCompressedAudioPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_data = data->d->m_data;
        this->d->m_flags = data->d->m_flags;
    }
}

AkCompressedAudioPacket::AkCompressedAudioPacket(const AkCompressedPacket &other)
{
    this->d = new AkCompressedAudioPacketPrivate();

    if (other.type() == AkCompressedPacket::PacketType_Audio) {
        auto data = reinterpret_cast<AkCompressedAudioPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_data = data->d->m_data;
        this->d->m_flags = data->d->m_flags;
    }
}

AkCompressedAudioPacket::AkCompressedAudioPacket(const AkCompressedAudioPacket &other):
    AkPacketBase(other)
{
    this->d = new AkCompressedAudioPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_data = other.d->m_data;
    this->d->m_flags = other.d->m_flags;
}

AkCompressedAudioPacket::~AkCompressedAudioPacket()
{
    delete this->d;
}

AkCompressedAudioPacket &AkCompressedAudioPacket::operator =(const AkPacket &other)
{
    if (other.type() == AkPacket::PacketAudioCompressed) {
        auto data = reinterpret_cast<AkCompressedAudioPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_data = data->d->m_data;
        this->d->m_flags = data->d->m_flags;
    } else {
        this->d->m_caps = AkCompressedAudioCaps();
        this->d->m_data.clear();
        this->d->m_flags = AudioPacketTypeFlag_None;
    }

    this->copyMetadata(other);

    return *this;
}

AkCompressedAudioPacket &AkCompressedAudioPacket::operator =(const AkCompressedPacket &other)
{
    if (other.type() == AkCompressedPacket::PacketType_Audio) {
        auto data = reinterpret_cast<AkCompressedAudioPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_data = data->d->m_data;
        this->d->m_flags = data->d->m_flags;
    } else {
        this->d->m_caps = AkCompressedAudioCaps();
        this->d->m_data.clear();
        this->d->m_flags = AudioPacketTypeFlag_None;
    }

    this->copyMetadata(other);

    return *this;
}

AkCompressedAudioPacket &AkCompressedAudioPacket::operator =(const AkCompressedAudioPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_data = other.d->m_data;
        this->d->m_flags = other.d->m_flags;
        this->copyMetadata(other);
    }

    return *this;
}

AkCompressedAudioPacket::operator bool() const
{
    return this->d->m_caps && !this->d->m_data.isEmpty();
}

AkCompressedAudioPacket::operator AkPacket() const
{
    AkPacket packet;
    packet.setType(AkPacket::PacketAudioCompressed);
    packet.setPrivateData(new AkCompressedAudioPacket(*this),
                          [] (void *data) -> void * {
                              return new AkCompressedAudioPacket(*reinterpret_cast<AkCompressedAudioPacket *>(data));
                          },
                          [] (void *data) {
                              delete reinterpret_cast<AkCompressedAudioPacket *>(data);
                          });
    packet.copyMetadata(*this);

    return packet;
}

AkCompressedAudioPacket::operator AkCompressedPacket() const
{
    AkCompressedPacket packet;
    packet.setType(AkCompressedPacket::PacketType_Audio);
    packet.setPrivateData(new AkCompressedAudioPacket(*this),
                          [] (void *data) -> void * {
                              return new AkCompressedAudioPacket(*reinterpret_cast<AkCompressedAudioPacket *>(data));
                          },
                          [] (void *data) {
                              delete reinterpret_cast<AkCompressedAudioPacket *>(data);
                          });
    packet.copyMetadata(*this);

    return packet;
}

const AkCompressedAudioCaps &AkCompressedAudioPacket::caps() const
{
    return this->d->m_caps;
}

char *AkCompressedAudioPacket::data() const
{
    return this->d->m_data.data();
}

const char *AkCompressedAudioPacket::constData() const
{
    return this->d->m_data.constData();
}

size_t AkCompressedAudioPacket::size() const
{
    return this->d->m_data.size();
}

AkCompressedAudioPacket::AudioPacketTypeFlag AkCompressedAudioPacket::flags() const
{
    return this->d->m_flags;
}

void AkCompressedAudioPacket::setFlags(AudioPacketTypeFlag flags)
{
    if (this->d->m_flags == flags)
        return;

    this->d->m_flags = flags;
    emit this->flagsChanged(flags);
}

void AkCompressedAudioPacket::resetFlags()
{
    this->setFlags(AudioPacketTypeFlag_None);
}

void AkCompressedAudioPacket::registerTypes()
{
    qRegisterMetaType<AkCompressedAudioPacket>("AkCompressedAudioPacket");
    qRegisterMetaType<AudioPacketTypeFlag>("AkAudioPacketTypeFlag");
    qmlRegisterSingletonType<AkCompressedAudioPacket>("Ak", 1, 0, "AkCompressedAudioPacket",
                                            [] (QQmlEngine *qmlEngine,
                                                QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkCompressedAudioPacket();
    });
}

QDebug operator <<(QDebug debug, const AkCompressedAudioPacket &packet)
{
    debug.nospace() << "AkCompressedAudioPacket("
                    << "caps="
                    << packet.caps()
                    << ",dataSize="
                    << packet.size()
                    << ",id="
                    << packet.id()
                    << ",pts="
                    << packet.pts()
                    << "("
                    << packet.pts() * packet.timeBase().value()
                    << ")"
                    << ",duration="
                    << packet.duration()
                    << "("
                    << packet.duration() * packet.timeBase().value()
                    << ")"
                    << ",timeBase="
                    << packet.timeBase()
                    << ",index="
                    << packet.index()
                    << ",flags="
                    << packet.flags()
                    << ",extraData="
                    << packet.extraData()
                    << ")";

    return debug.space();
}

#include "moc_akcompressedaudiopacket.cpp"
