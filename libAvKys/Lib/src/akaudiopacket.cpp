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

#include "akaudiopacket.h"
#include "akaudioconverter.h"
#include "akcaps.h"
#include "akfrac.h"
#include "akpacket.h"

class AkAudioPacketPrivate
{
    public:
        AkAudioCaps m_caps;
        QByteArray m_buffer;
        qint64 m_pts {0};
        AkFrac m_timeBase;
        qint64 m_id {-1};
        int m_index {-1};
};

AkAudioPacket::AkAudioPacket(QObject *parent):
    QObject(parent)
{
    this->d = new AkAudioPacketPrivate();
}

AkAudioPacket::AkAudioPacket(const AkAudioCaps &caps)
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = caps;
    this->d->m_buffer = QByteArray(int(caps.frameSize()), Qt::Uninitialized);
}

AkAudioPacket::AkAudioPacket(const AkPacket &other)
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = other.caps();
    this->d->m_buffer = other.buffer();
    this->d->m_pts = other.pts();
    this->d->m_timeBase = other.timeBase();
    this->d->m_index = other.index();
    this->d->m_id = other.id();
}

AkAudioPacket::AkAudioPacket(const AkAudioPacket &other):
    QObject()
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = other.d->m_caps;
    this->d->m_buffer = other.d->m_buffer;
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

AkAudioPacket::~AkAudioPacket()
{
    delete this->d;
}

AkAudioPacket &AkAudioPacket::operator =(const AkPacket &other)
{
    this->d->m_caps = other.caps();
    this->d->m_buffer = other.buffer();
    this->d->m_pts = other.pts();
    this->d->m_timeBase = other.timeBase();
    this->d->m_index = other.index();
    this->d->m_id = other.id();

    return *this;
}

AkAudioPacket &AkAudioPacket::operator =(const AkAudioPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_buffer = other.d->m_buffer;
        this->d->m_pts = other.d->m_pts;
        this->d->m_timeBase = other.d->m_timeBase;
        this->d->m_index = other.d->m_index;
        this->d->m_id = other.d->m_id;
    }

    return *this;
}

AkAudioPacket AkAudioPacket::operator +(const AkAudioPacket &other)
{
    AkAudioConverter converter(this->caps());
    auto tmpPacket = converter.convert(other);

    if (!tmpPacket)
        return *this;

    auto caps = this->caps();
    caps.setSamples(this->caps().samples() + tmpPacket.caps().samples());
    AkAudioPacket packet(caps);
    packet.copyMetadata(*this);

    for (int plane = 0; plane < caps.planes(); plane++) {
        auto start = this->caps().bytesPerPlane();
        memcpy(packet.planeData(plane),
               this->constPlaneData(plane),
               start);
        memcpy(packet.planeData(plane) + start,
               other.constPlaneData(plane),
               other.caps().bytesPerPlane());
    }

    return packet;
}

AkAudioPacket &AkAudioPacket::operator +=(const AkAudioPacket &other)
{
    AkAudioConverter converter(this->caps());
    auto tmpPacket = converter.convert(other);

    if (!tmpPacket)
        return *this;

    auto caps = this->caps();
    caps.setSamples(this->caps().samples() + tmpPacket.caps().samples());
    AkAudioPacket packet(caps);
    packet.copyMetadata(*this);

    for (int plane = 0; plane < caps.planes(); plane++) {
        auto start = this->caps().bytesPerPlane();
        memcpy(packet.planeData(plane),
               this->constPlaneData(plane),
               start);
        memcpy(packet.planeData(plane) + start,
               other.constPlaneData(plane),
               other.caps().bytesPerPlane());
    }

    *this = packet;

    return *this;
}

AkAudioPacket::operator AkPacket() const
{
    AkPacket packet(this->d->m_caps);
    packet.buffer() = this->d->m_buffer;
    packet.pts() = this->d->m_pts;
    packet.timeBase() = this->d->m_timeBase;
    packet.index() = this->d->m_index;
    packet.id() = this->d->m_id;

    return packet;
}

AkAudioPacket::operator bool() const
{
    return this->d->m_caps;
}

AkAudioCaps AkAudioPacket::caps() const
{
    return this->d->m_caps;
}

AkAudioCaps &AkAudioPacket::caps()
{
    return this->d->m_caps;
}

QByteArray AkAudioPacket::buffer() const
{
    return this->d->m_buffer;
}

QByteArray &AkAudioPacket::buffer()
{
    return this->d->m_buffer;
}

qint64 AkAudioPacket::id() const
{
    return this->d->m_id;
}

qint64 &AkAudioPacket::id()
{
    return this->d->m_id;
}

qint64 AkAudioPacket::pts() const
{
    return this->d->m_pts;
}

qint64 &AkAudioPacket::pts()
{
    return this->d->m_pts;
}

AkFrac AkAudioPacket::timeBase() const
{
    return this->d->m_timeBase;
}

AkFrac &AkAudioPacket::timeBase()
{
    return this->d->m_timeBase;
}

int AkAudioPacket::index() const
{
    return this->d->m_index;
}

int &AkAudioPacket::index()
{
    return this->d->m_index;
}

void AkAudioPacket::copyMetadata(const AkAudioPacket &other)
{
    this->d->m_pts = other.d->m_pts;
    this->d->m_timeBase = other.d->m_timeBase;
    this->d->m_index = other.d->m_index;
    this->d->m_id = other.d->m_id;
}

const quint8 *AkAudioPacket::constPlaneData(int plane) const
{
    return reinterpret_cast<const quint8 *>(this->d->m_buffer.constData())
            + this->d->m_caps.planeOffset(plane);
}

quint8 *AkAudioPacket::planeData(int plane)
{
    return reinterpret_cast<quint8 *>(this->d->m_buffer.data())
            + this->d->m_caps.planeOffset(plane);
}

const quint8 *AkAudioPacket::constSample(int channel, int i) const
{
    auto bps = this->d->m_caps.bps();

    if (this->d->m_caps.planar())
        return this->constPlaneData(channel) + i * bps / 8;

    auto channels = this->d->m_caps.channels();

    return this->constPlaneData(0) + (i * channels + channel) * bps / 8;
}

quint8 *AkAudioPacket::sample(int channel, int i)
{
    auto bps = this->d->m_caps.bps();

    if (this->d->m_caps.planar())
        return this->planeData(channel) + i * bps / 8;

    auto channels = this->d->m_caps.channels();

    return this->planeData(0) + (i * channels + channel) * bps / 8;
}

void AkAudioPacket::setSample(int channel, int i, const quint8 *sample)
{
    memcpy(this->sample(channel, i), sample, size_t(this->d->m_caps.bps()) / 8);
}

AkAudioPacket AkAudioPacket::realign(int align) const
{
    auto caps = this->d->m_caps;
    caps.realign(align);

    if (caps == this->d->m_caps)
        return *this;

    auto iPlaneSize = caps.planeSize();
    auto oPlaneSize = this->d->m_caps.planeSize();
    AkAudioPacket dst(caps);
    dst.copyMetadata(*this);

    for (int plane = 0; plane < caps.planes(); plane++) {
        auto planeSize = qMin(iPlaneSize[plane], oPlaneSize[plane]);
        auto src_line = this->constPlaneData(plane);
        auto dst_line = dst.planeData(plane);
        memcpy(dst_line, src_line, planeSize);
    }

    return dst;
}

AkAudioPacket AkAudioPacket::pop(int samples)
{
    auto caps = this->d->m_caps;
    samples = qMin(caps.samples(), samples);

    if (samples < 1)
        return {};

    caps.setSamples(samples);
    AkAudioPacket dst(caps);
    dst.copyMetadata(*this);

    caps.setSamples(this->d->m_caps.samples() - samples);
    AkAudioPacket tmpPacket(caps);
    tmpPacket.copyMetadata(*this);
    auto pts = this->d->m_pts
               + samples
               * this->d->m_timeBase.invert().value()
               / this->d->m_caps.rate();
    tmpPacket.setPts(qRound64(pts));

    for (int plane = 0; plane < dst.caps().planes(); plane++) {
        auto src_line = this->constPlaneData(plane);
        auto dst_line = dst.planeData(plane);
        auto dataSize = dst.caps().planeSize()[plane];
        memcpy(dst_line, src_line, dataSize);

        src_line = this->constPlaneData(plane) + dataSize;
        dst_line = tmpPacket.planeData(plane);
        dataSize = tmpPacket.caps().planeSize()[plane];

        if (dataSize > 0)
            memcpy(dst_line, src_line, dataSize);
    }

    *this = tmpPacket;

    return dst;
}

void AkAudioPacket::setCaps(const AkAudioCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void AkAudioPacket::setBuffer(const QByteArray &buffer)
{
    if (this->d->m_buffer == buffer)
        return;

    this->d->m_buffer = buffer;
    emit this->bufferChanged(buffer);
}

void AkAudioPacket::setId(qint64 id)
{
    if (this->d->m_id == id)
        return;

    this->d->m_id = id;
    emit this->idChanged(id);
}

void AkAudioPacket::setPts(qint64 pts)
{
    if (this->d->m_pts == pts)
        return;

    this->d->m_pts = pts;
    emit this->ptsChanged(pts);
}

void AkAudioPacket::setTimeBase(const AkFrac &timeBase)
{
    if (this->d->m_timeBase == timeBase)
        return;

    this->d->m_timeBase = timeBase;
    emit this->timeBaseChanged(timeBase);
}

void AkAudioPacket::setIndex(int index)
{
    if (this->d->m_index == index)
        return;

    this->d->m_index = index;
    emit this->indexChanged(index);
}

void AkAudioPacket::resetCaps()
{
    this->setCaps({});
}

void AkAudioPacket::resetBuffer()
{
    this->setBuffer({});
}

void AkAudioPacket::resetId()
{
    this->setId(-1);
}

void AkAudioPacket::resetPts()
{
    this->setPts(0);
}

void AkAudioPacket::resetTimeBase()
{
    this->setTimeBase({});
}

void AkAudioPacket::resetIndex()
{
    this->setIndex(-1);
}

void AkAudioPacket::registerTypes()
{
    qRegisterMetaType<AkAudioPacket>("AkAudioPacket");
    qmlRegisterSingletonType<AkAudioPacket>("Ak", 1, 0, "AkAudioPacket",
                                            [] (QQmlEngine *qmlEngine,
                                                QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkAudioPacket();
    });
}

QDebug operator <<(QDebug debug, const AkAudioPacket &packet)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "AkAudioPacket("
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

    return debug;
}

#include "moc_akaudiopacket.cpp"
