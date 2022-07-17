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
#include <QImage>
#include <QQmlEngine>

#include "akvideopacket.h"
#include "akpacket.h"
#include "akcaps.h"
#include "akfrac.h"
#include "akvideoformatspec.h"

class AkVideoPacketPrivate
{
    public:
        AkVideoCaps m_caps;
        QByteArray m_buffer;
        quint8 *m_data {nullptr};
        size_t *m_planeOffset {nullptr};
        size_t *m_planeSize {nullptr};
        qint64 m_pts {0};
        AkFrac m_timeBase;
        qint64 m_id {-1};
        int m_index {-1};
        int m_height {0};

        inline size_t lineOffset(int plane, int y) const;
        inline void clearCapsCache();
        inline void allocateCapsCache();
};

AkVideoPacket::AkVideoPacket(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoPacketPrivate();
    this->d->allocateCapsCache();
    this->d->m_data = reinterpret_cast<quint8 *>(this->d->m_buffer.data());
}

AkVideoPacket::AkVideoPacket(const AkVideoCaps &caps, bool initialized)
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = caps;

    if (initialized)
        this->d->m_buffer = QByteArray(int(caps.pictureSize()), 0);
    else
        this->d->m_buffer = QByteArray(int(caps.pictureSize()), Qt::Uninitialized);

    this->d->allocateCapsCache();
    this->d->m_data = reinterpret_cast<quint8 *>(this->d->m_buffer.data());
}

AkVideoPacket::AkVideoPacket(const AkPacket &other)
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = other.caps();
    this->d->m_buffer = other.buffer();
    this->d->m_pts = other.pts();
    this->d->m_timeBase = other.timeBase();
    this->d->m_index = other.index();
    this->d->m_id = other.id();
    this->d->allocateCapsCache();
    this->d->m_data = reinterpret_cast<quint8 *>(this->d->m_buffer.data());
}

AkVideoPacket::AkVideoPacket(const AkVideoPacket &other):
    QObject()
{
    this->d = new AkVideoPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_buffer = other.d->m_buffer;
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
    this->d->allocateCapsCache();
    this->d->m_data = reinterpret_cast<quint8 *>(this->d->m_buffer.data());
}

AkVideoPacket::~AkVideoPacket()
{
    this->d->clearCapsCache();
    delete this->d;
}

AkVideoPacket &AkVideoPacket::operator =(const AkPacket &other)
{
    this->d->m_caps = other.caps();
    this->d->m_buffer = other.buffer();
    this->d->m_pts = other.pts();
    this->d->m_timeBase = other.timeBase();
    this->d->m_index = other.index();
    this->d->m_id = other.id();
    this->d->allocateCapsCache();
    this->d->m_data = reinterpret_cast<quint8 *>(this->d->m_buffer.data());

    return *this;
}

AkVideoPacket &AkVideoPacket::operator =(const AkVideoPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_buffer = other.d->m_buffer;
        this->d->m_pts = other.d->m_pts;
        this->d->m_timeBase = other.d->m_timeBase;
        this->d->m_index = other.d->m_index;
        this->d->m_id = other.d->m_id;
        this->d->allocateCapsCache();
        this->d->m_data = reinterpret_cast<quint8 *>(this->d->m_buffer.data());
    }

    return *this;
}

AkVideoPacket::operator AkPacket() const
{
    AkPacket packet(this->d->m_caps);
    packet.buffer() = this->d->m_buffer;
    packet.pts() = this->d->m_pts;
    packet.timeBase() = this->d->m_timeBase;
    packet.index() = this->d->m_index;
    packet.id() = this->d->m_id;

    return packet;
}

AkVideoPacket::operator bool() const
{
    return this->d->m_caps && !this->d->m_buffer.isEmpty();
}

AkVideoCaps AkVideoPacket::caps() const
{
    return this->d->m_caps;
}

AkVideoCaps &AkVideoPacket::caps()
{
    return this->d->m_caps;
}

QByteArray AkVideoPacket::buffer() const
{
    return this->d->m_buffer;
}

QByteArray &AkVideoPacket::buffer()
{
    return this->d->m_buffer;
}

qint64 AkVideoPacket::id() const
{
    return this->d->m_id;
}

qint64 &AkVideoPacket::id()
{
    return this->d->m_id;
}

qint64 AkVideoPacket::pts() const
{
    return this->d->m_pts;
}

qint64 &AkVideoPacket::pts()
{
    return this->d->m_pts;
}

AkFrac AkVideoPacket::timeBase() const
{
    return this->d->m_timeBase;
}

AkFrac &AkVideoPacket::timeBase()
{
    return this->d->m_timeBase;
}

int AkVideoPacket::index() const
{
    return this->d->m_index;
}

int &AkVideoPacket::index()
{
    return this->d->m_index;
}

void AkVideoPacket::copyMetadata(const AkVideoPacket &other)
{
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

const quint8 *AkVideoPacket::constLine(int plane, int y) const
{
    return this->d->m_data + this->d->lineOffset(plane, y);
}

quint8 *AkVideoPacket::line(int plane, int y)
{
    return this->d->m_data + this->d->lineOffset(plane, y);
}

void AkVideoPacket::setCaps(const AkVideoCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    this->d->allocateCapsCache();
    emit this->capsChanged(caps);
}

void AkVideoPacket::setBuffer(const QByteArray &buffer)
{
    if (this->d->m_buffer == buffer)
        return;

    this->d->m_buffer = buffer;
    this->d->m_data = reinterpret_cast<quint8 *>(this->d->m_buffer.data());
    emit this->bufferChanged(buffer);
}

void AkVideoPacket::setId(qint64 id)
{
    if (this->d->m_id == id)
        return;

    this->d->m_id = id;
    emit this->idChanged(id);
}

void AkVideoPacket::setPts(qint64 pts)
{
    if (this->d->m_pts == pts)
        return;

    this->d->m_pts = pts;
    emit this->ptsChanged(pts);
}

void AkVideoPacket::setTimeBase(const AkFrac &timeBase)
{
    if (this->d->m_timeBase == timeBase)
        return;

    this->d->m_timeBase = timeBase;
    emit this->timeBaseChanged(timeBase);
}

void AkVideoPacket::setIndex(int index)
{
    if (this->d->m_index == index)
        return;

    this->d->m_index = index;
    emit this->indexChanged(index);
}

void AkVideoPacket::resetCaps()
{
    this->setCaps(AkVideoCaps());
}

void AkVideoPacket::resetBuffer()
{
    this->setBuffer({});
}

void AkVideoPacket::resetId()
{
    this->setId(-1);
}

void AkVideoPacket::resetPts()
{
    this->setPts(0);
}

void AkVideoPacket::resetTimeBase()
{
    this->setTimeBase({});
}

void AkVideoPacket::resetIndex()
{
    this->setIndex(-1);
}

void AkVideoPacket::registerTypes()
{
    qRegisterMetaType<AkVideoPacket>("AkVideoPacket");
    qmlRegisterSingletonType<AkVideoPacket>("Ak", 1, 0, "AkVideoPacket",
                                            [] (QQmlEngine *qmlEngine,
                                                QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkVideoPacket();
    });
}

QDebug operator <<(QDebug debug, const AkVideoPacket &packet)
{
    debug.nospace() << "AkVideoPacket("
                    << "caps="
                    << packet.caps()
                    << ",bufferSize="
                    << packet.buffer().size()
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

size_t AkVideoPacketPrivate::lineOffset(int plane, int y) const
{
    return this->m_planeOffset[plane]
            + size_t(y) * this->m_planeSize[plane] / this->m_height;
}

void AkVideoPacketPrivate::clearCapsCache()
{
    if (this->m_planeOffset) {
        delete [] this->m_planeOffset;
        this->m_planeOffset = nullptr;
    }

    if (this->m_planeSize) {
        delete [] this->m_planeSize;
        this->m_planeSize = nullptr;
    }
}

void AkVideoPacketPrivate::allocateCapsCache()
{
    this->clearCapsCache();
    this->m_height = this->m_caps.height();
    auto specs = AkVideoCaps::formatSpecs(this->m_caps.format());

    if (specs.planes().size() > 0) {
        this->m_planeOffset = new size_t [specs.planes().size()];
        this->m_planeSize = new size_t [specs.planes().size()];
        int i = 0;

        for (auto &plane: specs.planes()) {
            this->m_planeOffset[i] = this->m_caps.planeOffset(i);
            this->m_planeSize[i] = this->m_caps.planeSize(i);
            i++;
        }
    }
}

#include "moc_akvideopacket.cpp"
