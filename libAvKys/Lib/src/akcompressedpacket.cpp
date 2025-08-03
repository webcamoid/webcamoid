/* Webcamoid, camera capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#include "akcompressedpacket.h"
#include "akcompressedcaps.h"
#include "akcompressedaudiocaps.h"
#include "akcompressedaudiopacket.h"
#include "akcompressedvideocaps.h"
#include "akcompressedvideopacket.h"

class AkCompressedPacketPrivate
{
    public:
        AkCompressedPacket::PacketType m_type {AkCompressedPacket::PacketType_Unknown};
        void *m_privateData {nullptr};
        AkCompressedPacket::DataCopy m_copyFunc {nullptr};
        AkCompressedPacket::DataDeleter m_deleterFunc {nullptr};
};

AkCompressedPacket::AkCompressedPacket(QObject *parent):
    AkPacketBase(parent)
{
    this->d = new AkCompressedPacketPrivate();
}

AkCompressedPacket::AkCompressedPacket(const AkCompressedPacket &other):
    AkPacketBase(other)
{
    this->d = new AkCompressedPacketPrivate();
    this->d->m_type = other.d->m_type;

    if (other.d->m_privateData && other.d->m_copyFunc)
        this->d->m_privateData = other.d->m_copyFunc(other.d->m_privateData);

    this->d->m_copyFunc = other.d->m_copyFunc;
    this->d->m_deleterFunc = other.d->m_deleterFunc;
}

AkCompressedPacket::~AkCompressedPacket()
{
    if (this->d->m_privateData && this->d->m_copyFunc)
        this->d->m_deleterFunc(this->d->m_privateData);

    delete this->d;
}

AkCompressedPacket &AkCompressedPacket::operator =(const AkCompressedPacket &other)
{
    if (this != &other) {
        this->d->m_type = other.d->m_type;

        if (this->d->m_privateData && this->d->m_copyFunc) {
            this->d->m_deleterFunc(this->d->m_privateData);
            this->d->m_privateData = nullptr;
        }

        if (other.d->m_privateData && other.d->m_copyFunc)
            this->d->m_privateData = other.d->m_copyFunc(other.d->m_privateData);

        this->d->m_copyFunc = other.d->m_copyFunc;
        this->d->m_deleterFunc = other.d->m_deleterFunc;
        this->copyMetadata(other);
    }

    return *this;
}

AkCompressedPacket::operator bool() const
{
    if (!this->d->m_privateData)
        return false;

    switch (this->d->m_type) {
    case AkCompressedPacket::PacketType_Audio:
        return *reinterpret_cast<AkCompressedAudioPacket *>(this->d->m_privateData);
    case AkCompressedPacket::PacketType_Video:
        return *reinterpret_cast<AkCompressedVideoPacket *>(this->d->m_privateData);
    default:
        break;
    }

    return false;
}

AkCompressedPacket::PacketType AkCompressedPacket::type() const
{
    return this->d->m_type;
}

AkCompressedCaps AkCompressedPacket::caps() const
{
    switch (this->d->m_type) {
    case AkCompressedPacket::PacketType_Audio:
        return reinterpret_cast<AkCompressedAudioPacket *>(this->d->m_privateData)->caps();
    case AkCompressedPacket::PacketType_Video:
        return reinterpret_cast<AkCompressedVideoPacket *>(this->d->m_privateData)->caps();
    default:
        break;
    }

    return {};
}

char *AkCompressedPacket::data() const
{
    switch (this->d->m_type) {
    case AkCompressedPacket::PacketType_Audio:
        return reinterpret_cast<AkCompressedAudioPacket *>(this->d->m_privateData)->data();
    case AkCompressedPacket::PacketType_Video:
        return reinterpret_cast<AkCompressedVideoPacket *>(this->d->m_privateData)->data();
    default:
        break;
    }

    return nullptr;
}

const char *AkCompressedPacket::constData() const
{
    switch (this->d->m_type) {
    case AkCompressedPacket::PacketType_Audio:
        return reinterpret_cast<AkCompressedAudioPacket *>(this->d->m_privateData)->constData();
    case AkCompressedPacket::PacketType_Video:
        return reinterpret_cast<AkCompressedVideoPacket *>(this->d->m_privateData)->constData();
    default:
        break;
    }

    return nullptr;
}

size_t AkCompressedPacket::size() const
{
    switch (this->d->m_type) {
    case AkCompressedPacket::PacketType_Audio:
        return reinterpret_cast<AkCompressedAudioPacket *>(this->d->m_privateData)->size();
    case AkCompressedPacket::PacketType_Video:
        return reinterpret_cast<AkCompressedVideoPacket *>(this->d->m_privateData)->size();
    default:
        break;
    }

    return 0;
}

void *AkCompressedPacket::privateData() const
{
    return this->d->m_privateData;
}

void AkCompressedPacket::setPrivateData(void *data, DataCopy copyFunc, DataDeleter deleterFunc)
{
    this->d->m_privateData = data;
    this->d->m_copyFunc = copyFunc;
    this->d->m_deleterFunc = deleterFunc;
}

void AkCompressedPacket::setType(AkCompressedPacket::PacketType type)
{
    this->d->m_type = type;
}

void AkCompressedPacket::registerTypes()
{
    qRegisterMetaType<AkCompressedPacket>("AkCompressedPacket");
    qmlRegisterSingletonType<AkCompressedPacket>("Ak", 1, 0, "AkCompressedPacket",
                                                 [] (QQmlEngine *qmlEngine,
                                                     QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkCompressedPacket();
    });
}

QDebug operator <<(QDebug debug, const AkCompressedPacket &packet)
{
    debug.nospace() << "AkCompressedPacket(";

    switch (packet.type()) {
    case AkCompressedPacket::PacketType_Audio:
        debug.nospace() << *reinterpret_cast<AkCompressedAudioPacket *>(packet.d->m_privateData);
        break;
    case AkCompressedPacket::PacketType_Video:
        debug.nospace() << *reinterpret_cast<AkCompressedVideoPacket *>(packet.d->m_privateData);
        break;
    default:
        break;
    }

    debug.nospace() << ")";

    return debug.space();
}

#include "moc_akcompressedpacket.cpp"
