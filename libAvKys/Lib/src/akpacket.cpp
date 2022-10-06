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
#include <QVariant>
#include <QQmlEngine>

#include "akpacket.h"
#include "akaudiocaps.h"
#include "akaudiopacket.h"
#include "akcaps.h"
#include "akcompressedvideocaps.h"
#include "akcompressedvideopacket.h"
#include "aksubtitlecaps.h"
#include "aksubtitlepacket.h"
#include "akvideocaps.h"
#include "akvideopacket.h"

class AkPacketPrivate
{
    public:
        AkPacket::PacketType m_type {AkPacket::PacketUnknown};
        void *m_privateData {nullptr};
        AkPacket::DataCopy m_copyFunc {nullptr};
        AkPacket::DataDeleter m_deleterFunc {nullptr};
};

AkPacket::AkPacket(QObject *parent):
    AkPacketBase(parent)
{
    this->d = new AkPacketPrivate();
}

AkPacket::AkPacket(const AkPacket &other):
    AkPacketBase(other)
{
    this->d = new AkPacketPrivate();
    this->d->m_type = other.d->m_type;

    if (other.d->m_privateData && other.d->m_copyFunc)
        this->d->m_privateData = other.d->m_copyFunc(other.d->m_privateData);

    this->d->m_copyFunc = other.d->m_copyFunc;
    this->d->m_deleterFunc = other.d->m_deleterFunc;
}

AkPacket::~AkPacket()
{
    if (this->d->m_privateData && this->d->m_copyFunc)
        this->d->m_deleterFunc(this->d->m_privateData);

    delete this->d;
}

AkPacket &AkPacket::operator =(const AkPacket &other)
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

AkPacket::operator bool() const
{
    if (!this->d->m_privateData)
        return false;

    switch (this->d->m_type) {
    case AkPacket::PacketAudio:
        return *reinterpret_cast<AkAudioPacket *>(this->d->m_privateData);
    case AkPacket::PacketSubtitle:
        return *reinterpret_cast<AkSubtitlePacket *>(this->d->m_privateData);
    case AkPacket::PacketVideo:
        return *reinterpret_cast<AkVideoPacket *>(this->d->m_privateData);
    case AkPacket::PacketVideoCompressed:
        return *reinterpret_cast<AkCompressedVideoPacket *>(this->d->m_privateData);
    default:
        break;
    }

    return false;
}

AkPacket::PacketType AkPacket::type() const
{
    return this->d->m_type;
}

AkCaps AkPacket::caps() const
{
    switch (this->d->m_type) {
    case AkPacket::PacketAudio:
        return reinterpret_cast<AkAudioPacket *>(this->d->m_privateData)->caps();
    case AkPacket::PacketSubtitle:
        return reinterpret_cast<AkSubtitlePacket *>(this->d->m_privateData)->caps();
    case AkPacket::PacketVideo:
        return reinterpret_cast<AkVideoPacket *>(this->d->m_privateData)->caps();
    case AkPacket::PacketVideoCompressed:
        return reinterpret_cast<AkCompressedVideoPacket *>(this->d->m_privateData)->caps();
    default:
        break;
    }

    return {};
}

char *AkPacket::data() const
{
    switch (this->d->m_type) {
    case AkPacket::PacketAudio:
        return reinterpret_cast<AkAudioPacket *>(this->d->m_privateData)->data();
    case AkPacket::PacketSubtitle:
        return reinterpret_cast<AkSubtitlePacket *>(this->d->m_privateData)->data();
    case AkPacket::PacketVideo:
        return reinterpret_cast<AkVideoPacket *>(this->d->m_privateData)->data();
    case AkPacket::PacketVideoCompressed:
        return reinterpret_cast<AkCompressedVideoPacket *>(this->d->m_privateData)->data();
    default:
        break;
    }

    return nullptr;
}

const char *AkPacket::constData() const
{
    switch (this->d->m_type) {
    case AkPacket::PacketAudio:
        return reinterpret_cast<AkAudioPacket *>(this->d->m_privateData)->constData();
    case AkPacket::PacketSubtitle:
        return reinterpret_cast<AkSubtitlePacket *>(this->d->m_privateData)->constData();
    case AkPacket::PacketVideo:
        return reinterpret_cast<AkVideoPacket *>(this->d->m_privateData)->constData();
    case AkPacket::PacketVideoCompressed:
        return reinterpret_cast<AkCompressedVideoPacket *>(this->d->m_privateData)->constData();
    default:
        break;
    }

    return nullptr;
}

size_t AkPacket::size() const
{
    switch (this->d->m_type) {
    case AkPacket::PacketAudio:
        return reinterpret_cast<AkAudioPacket *>(this->d->m_privateData)->size();
    case AkPacket::PacketSubtitle:
        return reinterpret_cast<AkSubtitlePacket *>(this->d->m_privateData)->size();
    case AkPacket::PacketVideo:
        return reinterpret_cast<AkVideoPacket *>(this->d->m_privateData)->size();
    case AkPacket::PacketVideoCompressed:
        return reinterpret_cast<AkCompressedVideoPacket *>(this->d->m_privateData)->size();
    default:
        break;
    }

    return 0;
}

void *AkPacket::privateData() const
{
    return this->d->m_privateData;
}

void AkPacket::setPrivateData(void *data, DataCopy copyFunc, DataDeleter deleterFunc)
{
    this->d->m_privateData = data;
    this->d->m_copyFunc = copyFunc;
    this->d->m_deleterFunc = deleterFunc;
}

void AkPacket::setType(AkPacket::PacketType type)
{
    this->d->m_type = type;
}

void AkPacket::registerTypes()
{
    qRegisterMetaType<AkPacket>("AkPacket");
    qmlRegisterSingletonType<AkPacket>("Ak", 1, 0, "AkPacket",
                                       [] (QQmlEngine *qmlEngine,
                                           QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkPacket();
    });
}

QDebug operator <<(QDebug debug, const AkPacket &packet)
{
    debug.nospace() << "AkPacket(";

    switch (packet.type()) {
    case AkPacket::PacketAudio:
        debug.nospace() << *reinterpret_cast<AkAudioPacket *>(packet.d->m_privateData);
        break;
    case AkPacket::PacketSubtitle:
        debug.nospace() << *reinterpret_cast<AkSubtitlePacket *>(packet.d->m_privateData);
        break;
    case AkPacket::PacketVideo:
        debug.nospace() << *reinterpret_cast<AkVideoPacket *>(packet.d->m_privateData);
        break;
    case AkPacket::PacketVideoCompressed:
        debug.nospace() << *reinterpret_cast<AkCompressedVideoPacket *>(packet.d->m_privateData);
        break;
    default:
        break;
    }

    debug.nospace() << ")";

    return debug.space();
}

#include "moc_akpacket.cpp"
