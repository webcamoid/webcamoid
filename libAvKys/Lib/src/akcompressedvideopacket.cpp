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

#include "akcompressedvideopacket.h"
#include "akcompressedvideocaps.h"
#include "akfrac.h"
#include "akpacket.h"
#include "akcompressedpacket.h"

class AkCompressedVideoPacketPrivate
{
    public:
        AkCompressedVideoCaps m_caps;
        QByteArray m_data;
        AkCompressedVideoPacket::VideoPacketTypeFlag m_flags {AkCompressedVideoPacket::VideoPacketTypeFlag_None};
        AkCompressedVideoPacket::ExtraDataPackets m_extraData;
};

AkCompressedVideoPacket::AkCompressedVideoPacket(QObject *parent):
    AkPacketBase(parent)
{
    this->d = new AkCompressedVideoPacketPrivate();
}

AkCompressedVideoPacket::AkCompressedVideoPacket(const AkCompressedVideoCaps &caps,
                                                 size_t size,
                                                 bool initialized):
    AkPacketBase()
{
    this->d = new AkCompressedVideoPacketPrivate();
    this->d->m_caps = caps;

    if (initialized)
        this->d->m_data = QByteArray(int(size), 0);
    else
        this->d->m_data = QByteArray(int(size), Qt::Uninitialized);
}

AkCompressedVideoPacket::AkCompressedVideoPacket(const AkPacket &other):
    AkPacketBase(other)
{
    this->d = new AkCompressedVideoPacketPrivate();

    if (other.type() == AkPacket::PacketVideoCompressed) {
        auto data = reinterpret_cast<AkCompressedVideoPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_data = data->d->m_data;
        this->d->m_flags = data->d->m_flags;
        this->d->m_extraData = data->d->m_extraData;
    }
}

AkCompressedVideoPacket::AkCompressedVideoPacket(const AkCompressedPacket &other)
{
    this->d = new AkCompressedVideoPacketPrivate();

    if (other.type() == AkCompressedPacket::PacketType_Video) {
        auto data = reinterpret_cast<AkCompressedVideoPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_data = data->d->m_data;
        this->d->m_flags = data->d->m_flags;
        this->d->m_extraData = data->d->m_extraData;
    }
}

AkCompressedVideoPacket::AkCompressedVideoPacket(const AkCompressedVideoPacket &other):
    AkPacketBase(other)
{
    this->d = new AkCompressedVideoPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_data = other.d->m_data;
    this->d->m_flags = other.d->m_flags;
    this->d->m_extraData = other.d->m_extraData;
}

AkCompressedVideoPacket::~AkCompressedVideoPacket()
{
    delete this->d;
}

AkCompressedVideoPacket &AkCompressedVideoPacket::operator =(const AkPacket &other)
{
    if (other.type() == AkPacket::PacketVideoCompressed) {
        auto data = reinterpret_cast<AkCompressedVideoPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_data = data->d->m_data;
        this->d->m_flags = data->d->m_flags;
        this->d->m_extraData = data->d->m_extraData;
    } else {
        this->d->m_caps = AkCompressedVideoCaps();
        this->d->m_data.clear();
        this->d->m_flags = VideoPacketTypeFlag_None;
        this->d->m_extraData.clear();
    }

    this->copyMetadata(other);

    return *this;
}

AkCompressedVideoPacket &AkCompressedVideoPacket::operator =(const AkCompressedPacket &other)
{
    if (other.type() == AkCompressedPacket::PacketType_Video) {
        auto data = reinterpret_cast<AkCompressedVideoPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_data = data->d->m_data;
        this->d->m_flags = data->d->m_flags;
        this->d->m_extraData = data->d->m_extraData;
    } else {
        this->d->m_caps = AkCompressedVideoCaps();
        this->d->m_data.clear();
        this->d->m_flags = VideoPacketTypeFlag_None;
        this->d->m_extraData.clear();
    }

    this->copyMetadata(other);

    return *this;
}

AkCompressedVideoPacket &AkCompressedVideoPacket::operator =(const AkCompressedVideoPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_data = other.d->m_data;
        this->d->m_flags = other.d->m_flags;
        this->d->m_extraData = other.d->m_extraData;
        this->copyMetadata(other);
    }

    return *this;
}

AkCompressedVideoPacket::operator bool() const
{
    return this->d->m_caps && !this->d->m_data.isEmpty();
}

AkCompressedVideoPacket::operator AkPacket() const
{
    AkPacket packet;
    packet.setType(AkPacket::PacketVideoCompressed);
    packet.setPrivateData(new AkCompressedVideoPacket(*this),
                          [] (void *data) -> void * {
                              return new AkCompressedVideoPacket(*reinterpret_cast<AkCompressedVideoPacket *>(data));
                          },
                          [] (void *data) {
                              delete reinterpret_cast<AkCompressedVideoPacket *>(data);
                          });
    packet.copyMetadata(*this);

    return packet;
}

AkCompressedVideoPacket::operator AkCompressedPacket() const
{
    AkCompressedPacket packet;
    packet.setType(AkCompressedPacket::PacketType_Video);
    packet.setPrivateData(new AkCompressedVideoPacket(*this),
                          [] (void *data) -> void * {
                              return new AkCompressedVideoPacket(*reinterpret_cast<AkCompressedVideoPacket *>(data));
                          },
                          [] (void *data) {
                              delete reinterpret_cast<AkCompressedVideoPacket *>(data);
                          });
    packet.copyMetadata(*this);

    return packet;
}

const AkCompressedVideoCaps &AkCompressedVideoPacket::caps() const
{
    return this->d->m_caps;
}

char *AkCompressedVideoPacket::data() const
{
    return this->d->m_data.data();
}

const char *AkCompressedVideoPacket::constData() const
{
    return this->d->m_data.constData();
}

size_t AkCompressedVideoPacket::size() const
{
    return this->d->m_data.size();
}

AkCompressedVideoPacket::VideoPacketTypeFlag AkCompressedVideoPacket::flags() const
{
    return this->d->m_flags;
}

AkCompressedVideoPacket::ExtraDataPackets AkCompressedVideoPacket::extraData() const
{
    return this->d->m_extraData;
}

void AkCompressedVideoPacket::setFlags(VideoPacketTypeFlag flags)
{
    if (this->d->m_flags == flags)
        return;

    this->d->m_flags = flags;
    emit this->flagsChanged(flags);
}

void AkCompressedVideoPacket::setExtraData(const ExtraDataPackets &extraData)
{
    if (this->d->m_extraData == extraData)
        return;

    this->d->m_extraData = extraData;
    emit this->extraDataChanged(extraData);
}

void AkCompressedVideoPacket::resetFlags()
{
    this->setFlags(VideoPacketTypeFlag_None);
}

void AkCompressedVideoPacket::resetExtraData()
{
    this->setExtraData({});
}

void AkCompressedVideoPacket::registerTypes()
{
    qRegisterMetaType<AkCompressedVideoPacket>("AkCompressedVideoPacket");
    qRegisterMetaType<VideoPacketTypeFlag>("AkVideoPacketTypeFlag");
    qmlRegisterSingletonType<AkCompressedVideoPacket>("Ak", 1, 0, "AkCompressedVideoPacket",
                                            [] (QQmlEngine *qmlEngine,
                                                QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkCompressedVideoPacket();
    });
}

QDebug operator <<(QDebug debug, const AkCompressedVideoPacket &packet)
{
    debug.nospace() << "AkCompressedVideoPacket("
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

#include "moc_akcompressedvideopacket.cpp"
