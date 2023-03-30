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

class AkCompressedVideoPacketPrivate
{
    public:
        AkCompressedVideoCaps m_caps;
        QByteArray m_data;
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
    }
}

AkCompressedVideoPacket::AkCompressedVideoPacket(const AkCompressedVideoPacket &other):
    AkPacketBase(other)
{
    this->d = new AkCompressedVideoPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_data = other.d->m_data;
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
    } else {
        this->d->m_caps = AkCompressedVideoCaps();
        this->d->m_data.clear();
    }

    this->copyMetadata(other);

    return *this;
}

AkCompressedVideoPacket &AkCompressedVideoPacket::operator =(const AkCompressedVideoPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_data = other.d->m_data;
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

void AkCompressedVideoPacket::registerTypes()
{
    qRegisterMetaType<AkCompressedVideoPacket>("AkCompressedVideoPacket");
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
                    << ",timeBase="
                    << packet.timeBase()
                    << ",index="
                    << packet.index()
                    << ")";

    return debug.space();
}

#include "moc_akcompressedvideopacket.cpp"
