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
#include <QtEndian>

#include "akaudiopacket.h"
#include "akaudioconverter.h"
#include "akfrac.h"
#include "akpacket.h"

class AkAudioPacketPrivate
{
    public:
        AkAudioCaps m_caps;
        quint8 *m_data {nullptr};
        size_t m_dataSize {0};
        size_t m_samples {0};
        size_t m_nPlanes {0};
        quint8 **m_planes {nullptr};
        size_t *m_planeSize {nullptr};
        size_t *m_planeOffset {nullptr};

        ~AkAudioPacketPrivate();
        void allocateBuffers(size_t planes);
        void clearBuffers();
        void updateParams();
        inline void updatePlanes();

        template<typename T>
        inline static T from_(T value) {
            return value;
        }

        template<typename T>
        inline static T fromLE(T value) {
            return qFromLittleEndian(value);
        }

        template<typename T>
        inline static T fromBE(T value) {
            return qFromBigEndian(value);
        }

        template<typename T>
        inline static T to_(T value) {
            return value;
        }

        template<typename T>
        inline static T toLE(T value) {
            return qToLittleEndian(value);
        }

        template<typename T>
        inline static T toBE(T value) {
            return qToBigEndian(value);
        }

        template<typename SampleType, typename TransformFuncType>
        inline qreal volume(TransformFuncType transformFrom) const
        {
            auto amplitude = SampleType(0);

            if (this->m_nPlanes > 0) {
                for (int plane = 0; plane < this->m_nPlanes; ++plane) {
                    auto samples = reinterpret_cast<SampleType *>(this->m_planes[plane]);

                    for (int sample = 0; sample < this->m_samples; ++sample)
                        amplitude = qMax(qAbs(transformFrom(samples[sample])), amplitude);
                }
            } else {
                auto samples = reinterpret_cast<SampleType *>(this->m_data);
                auto totalSamples = this->m_samples * this->m_caps.channels();

                for (int sample = 0; sample < totalSamples; ++sample)
                    amplitude = qMax(qAbs(transformFrom(samples[sample])), amplitude);
            }

            SampleType max;

            if (typeid(SampleType) == typeid(float))
                max = SampleType(1.0f);
            else if (typeid(SampleType) == typeid(qreal))
                max = SampleType(1.0);
            else
                max = std::numeric_limits<SampleType>::max();

            return qreal(amplitude) / max;
        }
};

AkAudioPacket::AkAudioPacket(QObject *parent):
    AkPacketBase(parent)
{
    this->d = new AkAudioPacketPrivate();
}

AkAudioPacket::AkAudioPacket(const AkAudioCaps &caps,
                             size_t samples,
                             bool initialized):
    AkPacketBase()
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = caps;
    this->d->m_samples = samples;
    this->d->m_nPlanes = this->d->m_caps.planar()?
                             this->d->m_caps.channels():
                             1;
    this->d->allocateBuffers(this->d->m_nPlanes);
    this->d->updateParams();

    if (this->d->m_dataSize > 0) {
        this->d->m_data = new quint8 [this->d->m_dataSize];

        if (initialized)
            memset(this->d->m_data, 0, this->d->m_dataSize);
    }

    this->d->updatePlanes();
    this->setDuration(this->d->m_samples);
    this->setTimeBase({1, this->d->m_caps.rate()});
}

AkAudioPacket::AkAudioPacket(size_t size,
                             const AkAudioCaps &caps,
                             bool initialized):
    AkPacketBase()
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = caps;
    this->d->m_samples = 8 * size
                         / (this->d->m_caps.bps() * this->d->m_caps.channels());
    this->d->m_nPlanes = this->d->m_caps.planar()?
                             this->d->m_caps.channels():
                             1;
    this->d->allocateBuffers(this->d->m_nPlanes);
    this->d->updateParams();

    if (size > 0) {
        this->d->m_data = new quint8 [size];

        if (initialized)
            memset(this->d->m_data, 0, size);
    }

    this->d->updatePlanes();
    this->setDuration(this->d->m_samples);
    this->setTimeBase({1, this->d->m_caps.rate()});
}

AkAudioPacket::AkAudioPacket(const AkPacket &other):
    AkPacketBase(other)
{
    this->d = new AkAudioPacketPrivate();

    if (other.type() == AkPacket::PacketAudio) {
        auto data = reinterpret_cast<AkAudioPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;

        if (data->d->m_data && data->d->m_dataSize > 0) {
            this->d->m_data = new quint8 [data->d->m_dataSize];
            memcpy(this->d->m_data, data->d->m_data, data->d->m_dataSize);
        }

        this->d->m_dataSize = data->d->m_dataSize;
        this->d->m_samples = data->d->m_samples;
        this->d->m_nPlanes = data->d->m_nPlanes;
        this->d->allocateBuffers(this->d->m_nPlanes);

        if (this->d->m_nPlanes > 0) {
            memcpy(this->d->m_planeSize, data->d->m_planeSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_planeOffset, data->d->m_planeOffset, this->d->m_nPlanes * sizeof(size_t));
        }

        this->d->updatePlanes();
        this->setDuration(this->d->m_samples);
        this->setTimeBase({1, this->d->m_caps.rate()});
    }
}

AkAudioPacket::AkAudioPacket(const AkAudioPacket &other):
    AkPacketBase(other)
{
    this->d = new AkAudioPacketPrivate();
    this->d->m_caps = other.d->m_caps;

    if (other.d->m_data && other.d->m_dataSize > 0) {
        this->d->m_data = new quint8 [other.d->m_dataSize];
        memcpy(this->d->m_data, other.d->m_data, other.d->m_dataSize);
    }

    this->d->m_dataSize = other.d->m_dataSize;
    this->d->m_samples = other.d->m_samples;
    this->d->m_nPlanes = other.d->m_nPlanes;
    this->d->allocateBuffers(this->d->m_nPlanes);

    if (this->d->m_nPlanes > 0) {
        memcpy(this->d->m_planeSize, other.d->m_planeSize, this->d->m_nPlanes * sizeof(size_t));
        memcpy(this->d->m_planeOffset, other.d->m_planeOffset, this->d->m_nPlanes * sizeof(size_t));
    }

    this->d->updatePlanes();
}

AkAudioPacket::~AkAudioPacket()
{
    if (this->d->m_data)
        delete [] this->d->m_data;

    delete this->d;
}

AkAudioPacket &AkAudioPacket::operator =(const AkPacket &other)
{
    if (other.type() == AkPacket::PacketAudio) {
        auto data = reinterpret_cast<AkAudioPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;

        if (this->d->m_data) {
            delete [] this->d->m_data;
            this->d->m_data = nullptr;
        }

        if (data->d->m_data && data->d->m_dataSize > 0) {
            this->d->m_data = new quint8 [data->d->m_dataSize];
            memcpy(this->d->m_data, data->d->m_data, data->d->m_dataSize);
        }

        this->d->m_dataSize = data->d->m_dataSize;
        this->d->m_samples = data->d->m_samples;
        this->d->m_nPlanes = data->d->m_nPlanes;
        this->d->allocateBuffers(this->d->m_nPlanes);

        if (this->d->m_nPlanes > 0) {
            memcpy(this->d->m_planeSize, data->d->m_planeSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_planeOffset, data->d->m_planeOffset, this->d->m_nPlanes * sizeof(size_t));
        }

        this->d->updatePlanes();
        this->setDuration(this->d->m_samples);
        this->setTimeBase({1, this->d->m_caps.rate()});
    } else {
        this->d->m_caps = AkAudioCaps();

        if (this->d->m_data) {
            delete [] this->d->m_data;
            this->d->m_data = nullptr;
        }

        this->d->m_dataSize = 0;
        this->d->m_samples = 0;
        this->d->m_nPlanes = 0;
        this->d->clearBuffers();
        this->setDuration(0);
        this->setTimeBase({});
    }

    this->copyMetadata(other);

    return *this;
}

AkAudioPacket &AkAudioPacket::operator =(const AkAudioPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;

        if (this->d->m_data) {
            delete [] this->d->m_data;
            this->d->m_data = nullptr;
        }

        if (other.d->m_data && other.d->m_dataSize > 0) {
            this->d->m_data = new quint8 [other.d->m_dataSize];
            memcpy(this->d->m_data, other.d->m_data, other.d->m_dataSize);
        }

        this->d->m_dataSize = other.d->m_dataSize;
        this->d->m_samples = other.d->m_samples;
        this->d->m_nPlanes = other.d->m_nPlanes;
        this->d->allocateBuffers(this->d->m_nPlanes);

        if (this->d->m_nPlanes > 0) {
            memcpy(this->d->m_planeSize, other.d->m_planeSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_planeOffset, other.d->m_planeOffset, this->d->m_nPlanes * sizeof(size_t));
        }

        this->copyMetadata(other);
        this->d->updatePlanes();
        this->setDuration(this->d->m_samples);
        this->setTimeBase({1, this->d->m_caps.rate()});
    }

    return *this;
}

AkAudioPacket AkAudioPacket::operator +(const AkAudioPacket &other)
{
    AkAudioConverter converter(this->d->m_caps);
    auto tmpPacket = converter.convert(other);

    if (!tmpPacket)
        return *this;

    AkAudioPacket packet(this->d->m_caps,
                         this->d->m_samples + tmpPacket.d->m_samples);
    packet.copyMetadata(*this);
    packet.setDuration(packet.samples());

    for (int plane = 0; plane < this->d->m_nPlanes; ++plane) {
        auto start = this->d->m_planeSize[plane];
        memcpy(packet.d->m_planes[plane],
               this->d->m_planes[plane],
               start);
        memcpy(packet.d->m_planes[plane] + start,
               tmpPacket.d->m_planes[plane],
               tmpPacket.d->m_planeSize[plane]);
    }

    return packet;
}

AkAudioPacket &AkAudioPacket::operator +=(const AkAudioPacket &other)
{
    AkAudioConverter converter(this->d->m_caps);
    auto tmpPacket = converter.convert(other);

    if (!tmpPacket)
        return *this;

    AkAudioPacket packet(this->d->m_caps,
                         this->d->m_samples + tmpPacket.d->m_samples);
    packet.copyMetadata(*this);
    packet.setDuration(packet.samples());

    for (int plane = 0; plane < this->d->m_nPlanes; ++plane) {
        auto start = this->d->m_planeSize[plane];
        memcpy(packet.d->m_planes[plane],
               this->d->m_planes[plane],
               start);
        memcpy(packet.d->m_planes[plane] + start,
               tmpPacket.d->m_planes[plane],
               tmpPacket.d->m_planeSize[plane]);
    }

    *this = packet;

    return *this;
}

AkAudioPacket::operator bool() const
{
    return this->d->m_caps && this->d->m_data;
}

AkAudioPacket::operator AkPacket() const
{
    AkPacket packet;
    packet.setType(AkPacket::PacketAudio);
    packet.setPrivateData(new AkAudioPacket(*this),
                          [] (void *data) -> void * {
                              return new AkAudioPacket(*reinterpret_cast<AkAudioPacket *>(data));
                          },
                          [] (void *data) {
                              delete reinterpret_cast<AkAudioPacket *>(data);
                          });
    packet.copyMetadata(*this);

    return packet;
}

const AkAudioCaps &AkAudioPacket::caps() const
{
    return this->d->m_caps;
}

size_t AkAudioPacket::size() const
{
    return this->d->m_dataSize;
}

size_t AkAudioPacket::samples() const
{
    return this->d->m_samples;
}

size_t AkAudioPacket::planes() const
{
    return this->d->m_nPlanes;
}

size_t AkAudioPacket::planeSize(int plane) const
{
    return this->d->m_planeSize[plane];
}

const char *AkAudioPacket::constData() const
{
    return reinterpret_cast<char *>(this->d->m_data);
}

char *AkAudioPacket::data()
{
    return reinterpret_cast<char *>(this->d->m_data);
}

const quint8 *AkAudioPacket::constPlane(int plane) const
{
    return this->d->m_planes[plane];
}

quint8 *AkAudioPacket::plane(int plane)
{
    return this->d->m_planes[plane];
}

const quint8 *AkAudioPacket::constSample(int channel, int i) const
{
    auto bps = this->d->m_caps.bps();

    if (this->d->m_caps.planar())
        return this->d->m_planes[channel] + i * bps / 8;

    auto channels = this->d->m_caps.channels();

    return this->d->m_planes[0] + (i * channels + channel) * bps / 8;
}

quint8 *AkAudioPacket::sample(int channel, int i)
{
    auto bps = this->d->m_caps.bps();

    if (this->d->m_caps.planar())
        return this->d->m_planes[channel] + i * bps / 8;

    auto channels = this->d->m_caps.channels();

    return this->d->m_planes[0] + (i * channels + channel) * bps / 8;
}

void AkAudioPacket::setSample(int channel, int i, const quint8 *sample)
{
    memcpy(this->sample(channel, i), sample, size_t(this->d->m_caps.bps()) / 8);
}

AkAudioPacket AkAudioPacket::pop(int samples)
{
    samples = qMin<size_t>(this->d->m_samples, samples);

    if (samples < 1)
        return {};

    AkAudioPacket dst(this->d->m_caps, samples);
    dst.copyMetadata(*this);
    dst.setDuration(dst.samples());

    AkAudioPacket tmpPacket(this->d->m_caps, this->d->m_samples - samples);
    tmpPacket.copyMetadata(*this);
    tmpPacket.setPts(this->pts() + samples);
    tmpPacket.setDuration(tmpPacket.samples());

    for (int plane = 0; plane < dst.d->m_nPlanes; ++plane) {
        auto src_line = this->d->m_planes[plane];
        auto dst_line = dst.d->m_planes[plane];
        auto dataSize = dst.d->m_planeSize[plane];
        memcpy(dst_line, src_line, dataSize);

        src_line = this->d->m_planes[plane] + dataSize;
        dst_line = tmpPacket.d->m_planes[plane];
        dataSize = tmpPacket.d->m_planeSize[plane];

        if (dataSize > 0)
            memcpy(dst_line, src_line, dataSize);
    }

    *this = tmpPacket;

    return dst;
}

AkAudioPacket AkAudioPacket::pop()
{
    return this->pop(this->d->m_samples);
}

#define HANDLE_CASE_VOLUME(format, sampleType, endian) \
        case AkAudioCaps::SampleFormat_##format: \
            return this->d->volume<sampleType>(AkAudioPacketPrivate::from##endian<sampleType>);

qreal AkAudioPacket::volume() const
{
    switch (this->d->m_caps.format()) {
    HANDLE_CASE_VOLUME(s8   , qint8  ,  _)
    HANDLE_CASE_VOLUME(u8   , quint8 ,  _)
    HANDLE_CASE_VOLUME(s16le, qint16 , LE)
    HANDLE_CASE_VOLUME(s16be, qint16 , BE)
    HANDLE_CASE_VOLUME(u16le, quint16, LE)
    HANDLE_CASE_VOLUME(u16be, quint16, BE)
    HANDLE_CASE_VOLUME(s32le, qint32 , LE)
    HANDLE_CASE_VOLUME(s32be, qint32 , BE)
    HANDLE_CASE_VOLUME(u32le, quint32, LE)
    HANDLE_CASE_VOLUME(u32be, quint32, BE)
    HANDLE_CASE_VOLUME(s64le, qint64 , LE)
    HANDLE_CASE_VOLUME(s64be, qint64 , BE)
    HANDLE_CASE_VOLUME(u64le, quint64, LE)
    HANDLE_CASE_VOLUME(u64be, quint64, BE)
    HANDLE_CASE_VOLUME(fltle, float  , LE)
    HANDLE_CASE_VOLUME(fltbe, float  , BE)
    HANDLE_CASE_VOLUME(dblle, qreal  , LE)
    HANDLE_CASE_VOLUME(dblbe, qreal  , BE)
    default:
        break;
    }

    return 0.0;
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
                    << ",samples="
                    << packet.samples()
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
                    << "s"
                    << ",timeBase="
                    << packet.timeBase()
                    << ",index="
                    << packet.index()
                    << ")";

    return debug;
}

AkAudioPacketPrivate::~AkAudioPacketPrivate()
{
    this->clearBuffers();
}

void AkAudioPacketPrivate::allocateBuffers(size_t planes)
{
    this->clearBuffers();

    if (planes > 0) {
        this->m_planes = new quint8 *[planes];
        this->m_planeSize = new size_t[planes];
        this->m_planeOffset = new size_t[planes];

        memset(this->m_planes, 0, planes * sizeof(quint8 *));
        memset(this->m_planeSize, 0, planes * sizeof(size_t));
        memset(this->m_planeOffset, 0, planes * sizeof(size_t));
    }
}

void AkAudioPacketPrivate::clearBuffers()
{
    if (this->m_planes) {
        delete [] this->m_planes;
        this->m_planes = nullptr;
    }

    if (this->m_planeSize) {
        delete [] this->m_planeSize;
        this->m_planeSize = nullptr;
    }

    if (this->m_planeOffset) {
        delete [] this->m_planeOffset;
        this->m_planeOffset = nullptr;
    }
}

void AkAudioPacketPrivate::updateParams()
{
    this->m_dataSize = 0;
    this->allocateBuffers(this->m_nPlanes);
    size_t lineSize = this->m_caps.planar()?
                          size_t(this->m_caps.bps() * this->m_samples / 8):
                          size_t(this->m_caps.bps()
                                 * this->m_caps.channels()
                                 * this->m_samples
                                 / 8);

    for (int i = 0; i < this->m_nPlanes; ++i) {
        this->m_planeSize[i] = lineSize;
        this->m_planeOffset[i] = this->m_dataSize;
        this->m_dataSize += lineSize;
    }
}

void AkAudioPacketPrivate::updatePlanes()
{
    for (int i = 0; i < this->m_nPlanes; ++i)
        this->m_planes[i] = this->m_data + this->m_planeOffset[i];
}

#include "moc_akaudiopacket.cpp"
