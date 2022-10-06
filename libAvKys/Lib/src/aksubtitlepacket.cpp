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
#include <QQmlEngine>

#include "aksubtitlepacket.h"
#include "akfrac.h"
#include "akpacket.h"
#include "aksubtitlecaps.h"

class AkSubtitlePacketPrivate
{
    public:
        AkSubtitleCaps m_caps;
        QByteArray m_data;
};

AkSubtitlePacket::AkSubtitlePacket(QObject *parent):
    AkPacketBase(parent)
{
    this->d = new AkSubtitlePacketPrivate();
}

AkSubtitlePacket::AkSubtitlePacket(const AkSubtitleCaps &caps,
                                   size_t size,
                                   bool initialized):
    AkPacketBase()
{
    this->d = new AkSubtitlePacketPrivate();
    this->d->m_caps = caps;

    if (initialized)
        this->d->m_data = QByteArray(int(size), 0);
    else
        this->d->m_data = QByteArray(int(size), Qt::Uninitialized);
}

AkSubtitlePacket::AkSubtitlePacket(const AkPacket &other):
    AkPacketBase(other)
{
    this->d = new AkSubtitlePacketPrivate();

    if (other.type() == AkPacket::PacketSubtitle) {
        auto data = reinterpret_cast<AkSubtitlePacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_data = data->d->m_data;
    }
}

AkSubtitlePacket::AkSubtitlePacket(const AkSubtitlePacket &other):
    AkPacketBase(other)
{
    this->d = new AkSubtitlePacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_data = other.d->m_data;
}

AkSubtitlePacket::~AkSubtitlePacket()
{
    delete this->d;
}

AkSubtitlePacket &AkSubtitlePacket::operator =(const AkPacket &other)
{
    if (other.type() == AkPacket::PacketSubtitle) {
        auto data = reinterpret_cast<AkSubtitlePacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_data = data->d->m_data;
    } else {
        this->d->m_caps = AkSubtitleCaps();
        this->d->m_data.clear();
    }

    this->copyMetadata(other);

    return *this;
}

AkSubtitlePacket &AkSubtitlePacket::operator =(const AkSubtitlePacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_data = other.d->m_data;
        this->copyMetadata(other);
    }

    return *this;
}

AkSubtitlePacket::operator bool() const
{
    return this->d->m_caps && !this->d->m_data.isEmpty();
}

AkSubtitlePacket::operator AkPacket() const
{
    AkPacket packet;
    packet.setType(AkPacket::PacketSubtitle);
    packet.setPrivateData(new AkSubtitlePacket(*this),
                          [] (void *data) -> void * {
                              return new AkSubtitlePacket(*reinterpret_cast<AkSubtitlePacket *>(data));
                          },
                          [] (void *data) {
                              delete reinterpret_cast<AkSubtitlePacket *>(data);
                          });
    packet.copyMetadata(*this);

    return packet;
}

const AkSubtitleCaps &AkSubtitlePacket::caps() const
{
    return this->d->m_caps;
}

char *AkSubtitlePacket::data() const
{
    return this->d->m_data.data();
}

const char *AkSubtitlePacket::constData() const
{
    return this->d->m_data.constData();
}

size_t AkSubtitlePacket::size() const
{
    return this->d->m_data.size();
}

void AkSubtitlePacket::registerTypes()
{
    qRegisterMetaType<AkSubtitlePacket>("AkSubtitlePacket");
    qmlRegisterSingletonType<AkSubtitlePacket>("Ak", 1, 0, "AkSubtitlePacket",
                                            [] (QQmlEngine *qmlEngine,
                                                QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkSubtitlePacket();
    });
}

QDebug operator <<(QDebug debug, const AkSubtitlePacket &packet)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "AkSubtitlePacket("
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

    return debug;
}

#include "moc_aksubtitlepacket.cpp"
