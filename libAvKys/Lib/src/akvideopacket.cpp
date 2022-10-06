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
#include "akvideocaps.h"
#include "akfrac.h"
#include "akpacket.h"
#include "akvideoformatspec.h"

class AkVideoPacketPrivate
{
    public:
        AkVideoCaps m_caps;
        QByteArray m_buffer;
        size_t m_size {0};
        size_t m_nPlanes {0};
        quint8 **m_planes {nullptr};
        size_t *m_planeSize {nullptr};
        size_t *m_planeOffset {nullptr};
        size_t *m_lineSize {nullptr};
        size_t *m_bytesUsed {nullptr};
        size_t *m_widthDiv {nullptr};
        size_t *m_heightDiv {nullptr};
        size_t m_align {32};

        ~AkVideoPacketPrivate();
        void allocateBuffers(size_t planes);
        void clearBuffers();
        void updateParams(const AkVideoFormatSpec &specs);
        inline void updatePlanes();
        template<typename T>
        static inline T alignUp(const T &value, const T &align)
        {
            return (value + align - 1) & ~(align - 1);
        }
};

AkVideoPacket::AkVideoPacket(QObject *parent):
    AkPacketBase(parent)
{
    this->d = new AkVideoPacketPrivate;
}

AkVideoPacket::AkVideoPacket(const AkVideoCaps &caps,
                             bool initialized,
                             size_t align):
    AkPacketBase()
{
    this->d = new AkVideoPacketPrivate;
    this->d->m_caps = caps;
    this->d->m_align = align;
    auto specs = AkVideoCaps::formatSpecs(this->d->m_caps.format());
    this->d->m_nPlanes = specs.planes();
    this->d->allocateBuffers(this->d->m_nPlanes);
    this->d->updateParams(specs);

    if (initialized)
        this->d->m_buffer = QByteArray(int(this->d->m_size), 0);
    else
        this->d->m_buffer = QByteArray(int(this->d->m_size), Qt::Uninitialized);

    this->d->updatePlanes();
}

AkVideoPacket::AkVideoPacket(const AkPacket &other):
    AkPacketBase(other)
{
    this->d = new AkVideoPacketPrivate;

    if (other.type() == AkPacket::PacketVideo) {
        auto data = reinterpret_cast<AkVideoPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_buffer = data->d->m_buffer;
        this->d->m_size = data->d->m_size;
        this->d->m_nPlanes = data->d->m_nPlanes;
        this->d->allocateBuffers(this->d->m_nPlanes);

        if (this->d->m_nPlanes > 0) {
            memcpy(this->d->m_planeSize, data->d->m_planeSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_planeOffset, data->d->m_planeOffset, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_lineSize, data->d->m_lineSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_bytesUsed, data->d->m_bytesUsed, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_widthDiv, data->d->m_widthDiv, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_heightDiv, data->d->m_heightDiv, this->d->m_nPlanes * sizeof(size_t));
        }

        this->d->m_align = data->d->m_align;
        this->d->updatePlanes();
    }
}

AkVideoPacket::AkVideoPacket(const AkVideoPacket &other):
    AkPacketBase(other)
{
    this->d = new AkVideoPacketPrivate;
    this->d->m_caps = other.d->m_caps;
    this->d->m_buffer = other.d->m_buffer;
    this->d->m_size = other.d->m_size;
    this->d->m_nPlanes = other.d->m_nPlanes;
    this->d->allocateBuffers(this->d->m_nPlanes);

    if (this->d->m_nPlanes > 0) {
        memcpy(this->d->m_planeSize, other.d->m_planeSize, this->d->m_nPlanes * sizeof(size_t));
        memcpy(this->d->m_planeOffset, other.d->m_planeOffset, this->d->m_nPlanes * sizeof(size_t));
        memcpy(this->d->m_lineSize, other.d->m_lineSize, this->d->m_nPlanes * sizeof(size_t));
        memcpy(this->d->m_bytesUsed, other.d->m_bytesUsed, this->d->m_nPlanes * sizeof(size_t));
        memcpy(this->d->m_widthDiv, other.d->m_widthDiv, this->d->m_nPlanes * sizeof(size_t));
        memcpy(this->d->m_heightDiv, other.d->m_heightDiv, this->d->m_nPlanes * sizeof(size_t));
    }

    this->d->m_align = other.d->m_align;
    this->d->updatePlanes();
}

AkVideoPacket::~AkVideoPacket()
{
    delete this->d;
}

AkVideoPacket &AkVideoPacket::operator =(const AkPacket &other)
{
    if (other.type() == AkPacket::PacketVideo) {
        auto data = reinterpret_cast<AkVideoPacket *>(other.privateData());
        this->d->m_caps = data->d->m_caps;
        this->d->m_buffer = data->d->m_buffer;
        this->d->m_size = data->d->m_size;
        this->d->m_nPlanes = data->d->m_nPlanes;
        this->d->allocateBuffers(this->d->m_nPlanes);

        if (this->d->m_nPlanes > 0) {
            memcpy(this->d->m_planeSize, data->d->m_planeSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_planeOffset, data->d->m_planeOffset, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_lineSize, data->d->m_lineSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_bytesUsed, data->d->m_bytesUsed, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_widthDiv, data->d->m_widthDiv, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_heightDiv, data->d->m_heightDiv, this->d->m_nPlanes * sizeof(size_t));
        }

        this->d->m_align = data->d->m_align;
        this->d->updatePlanes();
    } else {
        this->d->m_caps = AkVideoCaps();
        this->d->m_buffer.clear();
        this->d->m_size = 0;
        this->d->m_nPlanes = 0;
        this->d->clearBuffers();
        this->d->m_align = 32;
    }

    this->copyMetadata(other);

    return *this;
}

AkVideoPacket &AkVideoPacket::operator =(const AkVideoPacket &other)
{
    if (this != &other) {
        this->d->m_caps = other.d->m_caps;
        this->d->m_buffer = other.d->m_buffer;
        this->d->m_size = other.d->m_size;
        this->d->m_nPlanes = other.d->m_nPlanes;
        this->d->allocateBuffers(this->d->m_nPlanes);

        if (this->d->m_nPlanes > 0) {
            memcpy(this->d->m_planeSize, other.d->m_planeSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_planeOffset, other.d->m_planeOffset, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_lineSize, other.d->m_lineSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_bytesUsed, other.d->m_bytesUsed, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_widthDiv, other.d->m_widthDiv, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_heightDiv, other.d->m_heightDiv, this->d->m_nPlanes * sizeof(size_t));
        }

        this->copyMetadata(other);
        this->d->m_align = other.d->m_align;
        this->d->updatePlanes();
    }

    return *this;
}

AkVideoPacket::operator bool() const
{
    return this->d->m_caps && !this->d->m_buffer.isEmpty();
}

AkVideoPacket::operator AkPacket() const
{
    AkPacket packet;
    packet.setType(AkPacket::PacketVideo);
    packet.setPrivateData(new AkVideoPacket(*this),
                          [] (void *data) -> void * {
                              return new AkVideoPacket(*reinterpret_cast<AkVideoPacket *>(data));
                          },
                          [] (void *data) {
                              delete reinterpret_cast<AkVideoPacket *>(data);
                          });
    packet.copyMetadata(*this);

    return packet;
}

const AkVideoCaps &AkVideoPacket::caps() const
{
    return this->d->m_caps;
}

size_t AkVideoPacket::size() const
{
    return this->d->m_size;
}

size_t AkVideoPacket::planes() const
{
    return this->d->m_nPlanes;
}

size_t AkVideoPacket::planeSize(int plane) const
{
    return this->d->m_planeSize[plane];
}

size_t AkVideoPacket::lineSize(int plane) const
{
    return this->d->m_lineSize[plane];
}

size_t AkVideoPacket::bytesUsed(int plane) const
{
    return this->d->m_bytesUsed[plane];
}

size_t AkVideoPacket::widthDiv(int plane) const
{
    return this->d->m_widthDiv[plane];
}

size_t AkVideoPacket::heightDiv(int plane) const
{
    return this->d->m_heightDiv[plane];
}

const char *AkVideoPacket::constData() const
{
    return reinterpret_cast<char *>(this->d->m_planes[0]);
}

char *AkVideoPacket::data()
{
    return reinterpret_cast<char *>(this->d->m_planes[0]);
}

const quint8 *AkVideoPacket::constPlane(int plane) const
{
    return this->d->m_planes[plane];
}

quint8 *AkVideoPacket::plane(int plane)
{
    return this->d->m_planes[plane];
}

const quint8 *AkVideoPacket::constLine(int plane, int y) const
{
    return this->d->m_planes[plane]
            + size_t(y >> this->d->m_heightDiv[plane])
            * this->d->m_lineSize[plane];
}

quint8 *AkVideoPacket::line(int plane, int y)
{
    return this->d->m_planes[plane]
            + size_t(y >> this->d->m_heightDiv[plane])
            * this->d->m_lineSize[plane];
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

AkVideoPacketPrivate::~AkVideoPacketPrivate()
{
    this->clearBuffers();
}

void AkVideoPacketPrivate::allocateBuffers(size_t planes)
{
    this->clearBuffers();

    if (planes > 0) {
        this->m_planes = new quint8 *[planes];
        this->m_planeSize = new size_t[planes];
        this->m_planeOffset = new size_t[planes];
        this->m_lineSize = new size_t[planes];
        this->m_bytesUsed = new size_t[planes];
        this->m_widthDiv = new size_t[planes];
        this->m_heightDiv = new size_t[planes];

        memset(this->m_planes, 0, planes * sizeof(quint8 *));
        memset(this->m_planeSize, 0, planes * sizeof(size_t));
        memset(this->m_planeOffset, 0, planes * sizeof(size_t));
        memset(this->m_lineSize, 0, planes * sizeof(size_t));
        memset(this->m_bytesUsed, 0, planes * sizeof(size_t));
        memset(this->m_widthDiv, 0, planes * sizeof(size_t));
        memset(this->m_heightDiv, 0, planes * sizeof(size_t));
    }
}

void AkVideoPacketPrivate::clearBuffers()
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

    if (this->m_lineSize) {
        delete [] this->m_lineSize;
        this->m_lineSize = nullptr;
    }

    if (this->m_bytesUsed) {
        delete [] this->m_bytesUsed;
        this->m_bytesUsed = nullptr;
    }

    if (this->m_widthDiv) {
        delete [] this->m_widthDiv;
        this->m_widthDiv = nullptr;
    }

    if (this->m_heightDiv) {
        delete [] this->m_heightDiv;
        this->m_heightDiv = nullptr;
    }
}

void AkVideoPacketPrivate::updateParams(const AkVideoFormatSpec &specs)
{
    if (this->m_align < 1)
        this->m_align = 32;

    this->m_size = 0;
    size_t offset = 0;
    int i = 0;

    for (size_t j = 0; j < specs.planes(); ++j) {
        auto &plane = specs.plane(j);
        size_t bytesUsed = plane.bitsSize() * this->m_caps.width() / 8;
        size_t lineSize =
                AkVideoPacketPrivate::alignUp(bytesUsed, size_t(this->m_align));

        if (this->m_lineSize)
            this->m_lineSize[i] = lineSize;

        if (this->m_bytesUsed)
            this->m_bytesUsed[i] = bytesUsed;

        size_t planeSize = (lineSize * this->m_caps.height()) >> plane.heightDiv();

        if (this->m_planeSize)
            this->m_planeSize[i] = planeSize;

        if (this->m_planeOffset)
            this->m_planeOffset[i] = this->m_size;

        this->m_size += planeSize;

        if (this->m_widthDiv)
            this->m_widthDiv[i] = plane.widthDiv();

        if (this->m_heightDiv)
            this->m_heightDiv[i] = plane.heightDiv();

        i++;
    }
}

void AkVideoPacketPrivate::updatePlanes()
{
    for (int i = 0; i < this->m_nPlanes; ++i)
        this->m_planes[i] = reinterpret_cast<quint8 *>(this->m_buffer.data())
                            + this->m_planeOffset[i];
}

#include "moc_akvideopacket.cpp"
