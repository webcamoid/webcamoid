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
#include "akcolorconvert.h"
#include "akfrac.h"
#include "akpacket.h"
#include "akvideoformatspec.h"

#define MAX_PLANES 4

enum FillType
{
    FillType_Vector,
    FillType_1,
    FillType_3,
};

enum FillDataTypes
{
    FillDataTypes_8,
    FillDataTypes_16,
    FillDataTypes_32,
};

enum AlphaMode
{
    AlphaMode_AO,
    AlphaMode_O,
};

class FillParameters
{
    public:
        AkColorConvert colorConvert;
        FillType fillType {FillType_3};
        FillDataTypes fillDataTypes {FillDataTypes_8};
        AlphaMode alphaMode {AlphaMode_AO};

        int endianess {Q_BYTE_ORDER};

        int width {0};
        int height {0};

        int *dstWidthOffsetX {nullptr};
        int *dstWidthOffsetY {nullptr};
        int *dstWidthOffsetZ {nullptr};
        int *dstWidthOffsetA {nullptr};

        int planeXo {0};
        int planeYo {0};
        int planeZo {0};
        int planeAo {0};

        AkColorComponent compXo;
        AkColorComponent compYo;
        AkColorComponent compZo;
        AkColorComponent compAo;

        size_t xoOffset {0};
        size_t yoOffset {0};
        size_t zoOffset {0};
        size_t aoOffset {0};

        size_t xoShift {0};
        size_t yoShift {0};
        size_t zoShift {0};
        size_t aoShift {0};

        quint64 maskXo {0};
        quint64 maskYo {0};
        quint64 maskZo {0};
        quint64 maskAo {0};

        FillParameters();
        FillParameters(const FillParameters &other);
        ~FillParameters();
        FillParameters &operator =(const FillParameters &other);
        inline void clearBuffers();
        inline void allocateBuffers(const AkVideoCaps &caps);
        void configure(const AkVideoCaps &caps, AkColorConvert &colorConvert);
        void configureFill(const AkVideoCaps &caps);
        void reset();
};

class AkVideoPacketPrivate
{
    public:
        AkVideoCaps m_caps;
        QByteArray m_buffer;
        size_t m_size {0};
        size_t m_nPlanes {0};
        quint8 *m_planes[MAX_PLANES];
        size_t m_planeSize[MAX_PLANES];
        size_t m_planeOffset[MAX_PLANES];
        size_t m_pixelSize[MAX_PLANES];
        size_t m_lineSize[MAX_PLANES];
        size_t m_bytesUsed[MAX_PLANES];
        size_t m_widthDiv[MAX_PLANES];
        size_t m_heightDiv[MAX_PLANES];
        size_t m_align {32};

        void updateParams(const AkVideoFormatSpec &specs);
        inline void updatePlanes();
        template<typename T>
        static inline T alignUp(const T &value, const T &align)
        {
            return (value + align - 1) & ~(align - 1);
        }

        // Endianness conversion functions for color components

        inline quint8 swapBytes(quint8 &&value, int endianness) const
        {
            Q_UNUSED(endianness)

            return value;
        }

        inline quint16 swapBytes(quint16 &&value, int endianness) const
        {
            if (endianness == Q_BYTE_ORDER)
                return value;

            quint16 result;
            auto pv = reinterpret_cast<quint8 *>(&value);
            auto pr = reinterpret_cast<quint8 *>(&result);
            pr[0] = pv[1];
            pr[1] = pv[0];

            return result;
        }

        inline quint32 swapBytes(quint32 &&value, int endianness) const
        {
            if (endianness == Q_BYTE_ORDER)
                return value;

            quint32 result;
            auto pv = reinterpret_cast<quint8 *>(&value);
            auto pr = reinterpret_cast<quint8 *>(&result);
            pr[0] = pv[3];
            pr[1] = pv[2];
            pr[2] = pv[1];
            pr[3] = pv[0];

            return result;
        }

        inline quint64 swapBytes(quint64 &&value, int endianness) const
        {
            if (endianness == Q_BYTE_ORDER)
                return value;

            quint64 result;
            auto pv = reinterpret_cast<quint8 *>(&value);
            auto pr = reinterpret_cast<quint8 *>(&result);
            pr[0] = pv[7];
            pr[1] = pv[6];
            pr[2] = pv[5];
            pr[3] = pv[4];
            pr[4] = pv[3];
            pr[5] = pv[2];
            pr[6] = pv[1];
            pr[7] = pv[0];

            return result;
        }

        /* Fill functions */

        template <typename DataType>
        void fill3(const FillParameters &fc, QRgb color) const
        {
            auto xi = qRed(color);
            auto yi = qGreen(color);
            auto zi = qBlue(color);
            auto ai = qAlpha(color);

            qint64 xo_ = 0;
            qint64 yo_ = 0;
            qint64 zo_ = 0;
            fc.colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);
            fc.colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

            auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
            auto line_y = this->m_planes[fc.planeYo] + fc.yoOffset;
            auto line_z = this->m_planes[fc.planeZo] + fc.zoOffset;

            auto width = qMax<size_t>(8 * this->m_pixelSize[0] / this->m_caps.bpp(), 1);

            for (int x = 0; x < width; ++x) {
                int &xd_x = fc.dstWidthOffsetX[x];
                int &xd_y = fc.dstWidthOffsetY[x];
                int &xd_z = fc.dstWidthOffsetZ[x];

                auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                auto yo = reinterpret_cast<DataType *>(line_y + xd_y);
                auto zo = reinterpret_cast<DataType *>(line_z + xd_z);

                *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                *yo = (*yo & DataType(fc.maskYo)) | (DataType(yo_) << fc.yoShift);
                *zo = (*zo & DataType(fc.maskZo)) | (DataType(zo_) << fc.zoShift);

                auto xot = this->swapBytes(DataType(*xo), fc.endianess);
                auto yot = this->swapBytes(DataType(*yo), fc.endianess);
                auto zot = this->swapBytes(DataType(*zo), fc.endianess);

                *xo = xot;
                *yo = yot;
                *zo = zot;
            }
        }

        template <typename DataType>
        void fill3A(const FillParameters &fc, QRgb color) const
        {
            auto xi = qRed(color);
            auto yi = qGreen(color);
            auto zi = qBlue(color);
            auto ai = qAlpha(color);

            qint64 xo_ = 0;
            qint64 yo_ = 0;
            qint64 zo_ = 0;
            fc.colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

            auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
            auto line_y = this->m_planes[fc.planeYo] + fc.yoOffset;
            auto line_z = this->m_planes[fc.planeZo] + fc.zoOffset;
            auto line_a = this->m_planes[fc.planeAo] + fc.aoOffset;

            auto width = qMax<size_t>(8 * this->m_pixelSize[0] / this->m_caps.bpp(), 1);

            for (int x = 0; x < width; ++x) {
                int &xd_x = fc.dstWidthOffsetX[x];
                int &xd_y = fc.dstWidthOffsetY[x];
                int &xd_z = fc.dstWidthOffsetZ[x];
                int &xd_a = fc.dstWidthOffsetA[x];

                auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                auto yo = reinterpret_cast<DataType *>(line_y + xd_y);
                auto zo = reinterpret_cast<DataType *>(line_z + xd_z);
                auto ao = reinterpret_cast<DataType *>(line_a + xd_a);

                *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                *yo = (*yo & DataType(fc.maskYo)) | (DataType(yo_) << fc.yoShift);
                *zo = (*zo & DataType(fc.maskZo)) | (DataType(zo_) << fc.zoShift);
                *ao = (*ao & DataType(fc.maskAo)) | (DataType(ai) << fc.aoShift);

                auto xot = this->swapBytes(DataType(*xo), fc.endianess);
                auto yot = this->swapBytes(DataType(*yo), fc.endianess);
                auto zot = this->swapBytes(DataType(*zo), fc.endianess);
                auto aot = this->swapBytes(DataType(*ao), fc.endianess);

                *xo = xot;
                *yo = yot;
                *zo = zot;
                *ao = aot;
            }
        }

        template <typename DataType>
        void fill1(const FillParameters &fc, QRgb color) const
        {
            auto xi = qRed(color);
            auto yi = qGreen(color);
            auto zi = qBlue(color);
            auto ai = qAlpha(color);

            qint64 xo_ = 0;
            fc.colorConvert.applyPoint(xi, yi, zi, &xo_);
            fc.colorConvert.applyAlpha(ai, &xo_);

            auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
            auto width = qMax<size_t>(8 * this->m_pixelSize[0] / this->m_caps.bpp(), 1);

            for (int x = 0; x < width; ++x) {
                int &xd_x = fc.dstWidthOffsetX[x];
                auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                *xo = this->swapBytes(DataType(*xo), fc.endianess);
            }
        }

        template <typename DataType>
        void fill1A(const FillParameters &fc, QRgb color) const
        {
            auto xi = qRed(color);
            auto yi = qGreen(color);
            auto zi = qBlue(color);
            auto ai = qAlpha(color);

            qint64 xo_ = 0;
            fc.colorConvert.applyPoint(xi, yi, zi, &xo_);

            auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
            auto line_a = this->m_planes[fc.planeAo] + fc.aoOffset;

            auto width = qMax<size_t>(8 * this->m_pixelSize[0] / this->m_caps.bpp(), 1);

            for (int x = 0; x < width; ++x) {
                int &xd_x = fc.dstWidthOffsetX[x];
                int &xd_a = fc.dstWidthOffsetA[x];

                auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                auto ao = reinterpret_cast<DataType *>(line_a + xd_a);

                *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                *ao = (*ao & DataType(fc.maskAo)) | (DataType(ai) << fc.aoShift);

                auto xot = this->swapBytes(DataType(*xo), fc.endianess);
                auto aot = this->swapBytes(DataType(*ao), fc.endianess);

                *xo = xot;
                *ao = aot;
            }
        }

        // Vectorized fill functions

        template <typename DataType>
        void fillV3(const FillParameters &fc, QRgb color) const
        {
            auto xi = qRed(color);
            auto yi = qGreen(color);
            auto zi = qBlue(color);
            auto ai = qAlpha(color);

            qint64 xo_ = 0;
            qint64 yo_ = 0;
            qint64 zo_ = 0;
            fc.colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);
            fc.colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

            auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
            auto line_y = this->m_planes[fc.planeYo] + fc.yoOffset;
            auto line_z = this->m_planes[fc.planeZo] + fc.zoOffset;

            auto width = qMax<size_t>(8 * this->m_pixelSize[0] / this->m_caps.bpp(), 1);

            for (int x = 0; x < width; ++x) {
                int &xd_x = fc.dstWidthOffsetX[x];
                int &xd_y = fc.dstWidthOffsetY[x];
                int &xd_z = fc.dstWidthOffsetZ[x];

                auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                auto yo = reinterpret_cast<DataType *>(line_y + xd_y);
                auto zo = reinterpret_cast<DataType *>(line_z + xd_z);

                *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                *yo = (*yo & DataType(fc.maskYo)) | (DataType(yo_) << fc.yoShift);
                *zo = (*zo & DataType(fc.maskZo)) | (DataType(zo_) << fc.zoShift);

                auto xot = this->swapBytes(DataType(*xo), fc.endianess);
                auto yot = this->swapBytes(DataType(*yo), fc.endianess);
                auto zot = this->swapBytes(DataType(*zo), fc.endianess);

                *xo = xot;
                *yo = yot;
                *zo = zot;
            }
        }

        template <typename DataType>
        void fillV3A(const FillParameters &fc, QRgb color) const
        {
            auto xi = qRed(color);
            auto yi = qGreen(color);
            auto zi = qBlue(color);
            auto ai = qAlpha(color);

            qint64 xo_ = 0;
            qint64 yo_ = 0;
            qint64 zo_ = 0;
            fc.colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

            auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
            auto line_y = this->m_planes[fc.planeYo] + fc.yoOffset;
            auto line_z = this->m_planes[fc.planeZo] + fc.zoOffset;
            auto line_a = this->m_planes[fc.planeAo] + fc.aoOffset;

            auto width = qMax<size_t>(8 * this->m_pixelSize[0] / this->m_caps.bpp(), 1);

            for (int x = 0; x < width; ++x) {
                int &xd_x = fc.dstWidthOffsetX[x];
                int &xd_y = fc.dstWidthOffsetY[x];
                int &xd_z = fc.dstWidthOffsetZ[x];
                int &xd_a = fc.dstWidthOffsetA[x];

                auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                auto yo = reinterpret_cast<DataType *>(line_y + xd_y);
                auto zo = reinterpret_cast<DataType *>(line_z + xd_z);
                auto ao = reinterpret_cast<DataType *>(line_a + xd_a);

                *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                *yo = (*yo & DataType(fc.maskYo)) | (DataType(yo_) << fc.yoShift);
                *zo = (*zo & DataType(fc.maskZo)) | (DataType(zo_) << fc.zoShift);
                *ao = (*ao & DataType(fc.maskAo)) | (DataType(ai) << fc.aoShift);

                auto xot = this->swapBytes(DataType(*xo), fc.endianess);
                auto yot = this->swapBytes(DataType(*yo), fc.endianess);
                auto zot = this->swapBytes(DataType(*zo), fc.endianess);
                auto aot = this->swapBytes(DataType(*ao), fc.endianess);

                *xo = xot;
                *yo = yot;
                *zo = zot;
                *ao = aot;
            }
        }

#define FILL_FUNC(components) \
        template <typename DataType> \
            inline void fillFrame##components(const FillParameters &fc, \
                                              QRgb color) const \
        { \
                switch (fc.alphaMode) { \
                case AlphaMode_AO: \
                    this->fill##components##A<DataType>(fc, color); \
                    break; \
                case AlphaMode_O: \
                    this->fill##components<DataType>(fc, color); \
                    break; \
            }; \
        }

#define FILLV_FUNC(components) \
        template <typename DataType> \
            inline void fillVFrame##components(const FillParameters &fc, \
                                              QRgb color) const \
        { \
                switch (fc.alphaMode) { \
                case AlphaMode_AO: \
                    this->fillV##components##A<DataType>(fc, color); \
                    break; \
                case AlphaMode_O: \
                    this->fillV##components<DataType>(fc, color); \
                    break; \
            }; \
        }

        FILL_FUNC(1)
        FILL_FUNC(3)
        FILLV_FUNC(3)

        template <typename DataType>
        inline void fill(const FillParameters &fc, QRgb color)
        {
            switch (fc.fillType) {
            case FillType_Vector:
                this->fillVFrame3<DataType>(fc, color);
                break;
            case FillType_3:
                this->fillFrame3<DataType>(fc, color);
                break;
            case FillType_1:
                this->fillFrame1<DataType>(fc, color);
                break;
            }
        }

        inline void fill(QRgb color);
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

        if (this->d->m_nPlanes > 0) {
            const size_t dataSize = MAX_PLANES * sizeof(size_t);
            memcpy(this->d->m_planeSize, data->d->m_planeSize, dataSize);
            memcpy(this->d->m_planeOffset, data->d->m_planeOffset, dataSize);
            memcpy(this->d->m_pixelSize, data->d->m_pixelSize, dataSize);
            memcpy(this->d->m_lineSize, data->d->m_lineSize, dataSize);
            memcpy(this->d->m_bytesUsed, data->d->m_bytesUsed, dataSize);
            memcpy(this->d->m_widthDiv, data->d->m_widthDiv, dataSize);
            memcpy(this->d->m_heightDiv, data->d->m_heightDiv, dataSize);
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

    if (this->d->m_nPlanes > 0) {
        const size_t dataSize = MAX_PLANES * sizeof(size_t);
        memcpy(this->d->m_planeSize, other.d->m_planeSize, dataSize);
        memcpy(this->d->m_planeOffset, other.d->m_planeOffset, dataSize);
        memcpy(this->d->m_pixelSize, other.d->m_pixelSize, dataSize);
        memcpy(this->d->m_lineSize, other.d->m_lineSize, dataSize);
        memcpy(this->d->m_bytesUsed, other.d->m_bytesUsed, dataSize);
        memcpy(this->d->m_widthDiv, other.d->m_widthDiv, dataSize);
        memcpy(this->d->m_heightDiv, other.d->m_heightDiv, dataSize);
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

        if (this->d->m_nPlanes > 0) {
            const size_t dataSize = MAX_PLANES * sizeof(size_t);
            memcpy(this->d->m_planeSize, data->d->m_planeSize, dataSize);
            memcpy(this->d->m_planeOffset, data->d->m_planeOffset, dataSize);
            memcpy(this->d->m_pixelSize, data->d->m_pixelSize, dataSize);
            memcpy(this->d->m_lineSize, data->d->m_lineSize, dataSize);
            memcpy(this->d->m_bytesUsed, data->d->m_bytesUsed, dataSize);
            memcpy(this->d->m_widthDiv, data->d->m_widthDiv, dataSize);
            memcpy(this->d->m_heightDiv, data->d->m_heightDiv, dataSize);
        }

        this->d->m_align = data->d->m_align;
        this->d->updatePlanes();
    } else {
        this->d->m_caps = AkVideoCaps();
        this->d->m_buffer.clear();
        this->d->m_size = 0;
        this->d->m_nPlanes = 0;
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

        if (this->d->m_nPlanes > 0) {
            memcpy(this->d->m_planeSize, other.d->m_planeSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_planeOffset, other.d->m_planeOffset, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_pixelSize, other.d->m_pixelSize, this->d->m_nPlanes * sizeof(size_t));
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

size_t AkVideoPacket::pixelSize(int plane) const
{
    return this->d->m_pixelSize[plane];
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

AkVideoPacket AkVideoPacket::copy(int x, int y, int width, int height) const
{
    auto ocaps = this->d->m_caps;
    ocaps.setWidth(width);
    ocaps.setHeight(height);
    AkVideoPacket dst(ocaps, true);
    dst.copyMetadata(*this);

    auto maxX = qMin(x + width, this->d->m_caps.width());
    auto maxY = qMin(y + height, this->d->m_caps.height());
    auto copyWidth = qMax(maxX - x, 0);

    if (copyWidth < 1)
        return dst;

    auto diffY = maxY - y;

    for (int plane = 0; plane < this->d->m_nPlanes; plane++) {
        size_t offset = x
                        * this->d->m_bytesUsed[plane]
                        / this->d->m_caps.width();
        size_t copyBytes = copyWidth
                           * this->d->m_bytesUsed[plane]
                           / this->d->m_caps.width();
        auto srcLineOffset = this->d->m_lineSize[plane];
        auto dstLineOffset = dst.d->m_lineSize[plane];
        auto srcLine = this->constLine(plane, y) + offset;
        auto dstLine = dst.d->m_planes[plane];
        auto maxY = diffY >> this->d->m_heightDiv[plane];

        for (int y = 0; y < maxY; y++) {
            memcpy(dstLine, srcLine, copyBytes);
            srcLine += srcLineOffset;
            dstLine += dstLineOffset;
        }
    }

    return dst;
}

void AkVideoPacket::fillRgb(QRgb color)
{
    return this->d->fill(color);
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

void AkVideoPacketPrivate::updateParams(const AkVideoFormatSpec &specs)
{
    if (this->m_align < 1)
        this->m_align = 32;

    this->m_size = 0;
    int i = 0;

    for (size_t j = 0; j < specs.planes(); ++j) {
        auto &plane = specs.plane(j);
        size_t bytesUsed = plane.bitsSize() * this->m_caps.width() / 8;
        size_t lineSize =
                AkVideoPacketPrivate::alignUp(bytesUsed, size_t(this->m_align));

        this->m_pixelSize[i] = plane.pixelSize();
        this->m_lineSize[i] = lineSize;
        this->m_bytesUsed[i] = bytesUsed;

        size_t planeSize = (lineSize * this->m_caps.height()) >> plane.heightDiv();

        this->m_planeSize[i] = planeSize;
        this->m_planeOffset[i] = this->m_size;

        this->m_size += planeSize;

        this->m_widthDiv[i] = plane.widthDiv();
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

#define DEFINE_FILL_FUNC(size) \
case FillDataTypes_##size: \
    this->fill<quint##size>(fc, color); \
    break;

void AkVideoPacketPrivate::fill(QRgb color)
{
    FillParameters fc;
    fc.configure(this->m_caps, fc.colorConvert);
    fc.configureFill(this->m_caps);

    switch (fc.fillDataTypes) {
        DEFINE_FILL_FUNC(8)
        DEFINE_FILL_FUNC(16)
        DEFINE_FILL_FUNC(32)
    }

    for (size_t plane = 0; plane < this->m_nPlanes; plane++) {
        auto &lineSize = this->m_lineSize[plane];
        auto &pixelSize = this->m_pixelSize[plane];
        auto line0 = this->m_planes[plane];
        auto line = line0 + pixelSize;
        auto width = lineSize / pixelSize;
        auto height = fc.height >> this->m_heightDiv[plane];

        for (int x = 1; x < width; ++x) {
            memcpy(line, line0, pixelSize);
            line += pixelSize;
        }

        line = line0 + lineSize;

        for (int y = 1; y < height; ++y) {
            memcpy(line, line0, lineSize);
            line += lineSize;
        }
    }
}

FillParameters::FillParameters()
{
}

FillParameters::FillParameters(const FillParameters &other):
    colorConvert(other.colorConvert),
    fillType(other.fillType),
    fillDataTypes(other.fillDataTypes),
    alphaMode(other.alphaMode),
    endianess(other.endianess),
    width(other.width),
    height(other.height),
    planeXo(other.planeXo),
    planeYo(other.planeYo),
    planeZo(other.planeZo),
    planeAo(other.planeAo),
    compXo(other.compXo),
    compYo(other.compYo),
    compZo(other.compZo),
    compAo(other.compAo),
    xoOffset(other.xoOffset),
    yoOffset(other.yoOffset),
    zoOffset(other.zoOffset),
    aoOffset(other.aoOffset),
    xoShift(other.xoShift),
    yoShift(other.yoShift),
    zoShift(other.zoShift),
    aoShift(other.aoShift),
    maskXo(other.maskXo),
    maskYo(other.maskYo),
    maskZo(other.maskZo),
    maskAo(other.maskAo)
{
    if (this->width > 0) {
        size_t oWidthDataSize = sizeof(int) * this->width;

        if (other.dstWidthOffsetX) {
            this->dstWidthOffsetX = new int [this->width];
            memcpy(this->dstWidthOffsetX, other.dstWidthOffsetX, oWidthDataSize);
        }

        if (other.dstWidthOffsetY) {
            this->dstWidthOffsetY = new int [this->width];
            memcpy(this->dstWidthOffsetY, other.dstWidthOffsetY, oWidthDataSize);
        }

        if (other.dstWidthOffsetZ) {
            this->dstWidthOffsetZ = new int [this->width];
            memcpy(this->dstWidthOffsetZ, other.dstWidthOffsetZ, oWidthDataSize);
        }

        if (other.dstWidthOffsetA) {
            this->dstWidthOffsetA = new int [this->width];
            memcpy(this->dstWidthOffsetA, other.dstWidthOffsetA, oWidthDataSize);
        }
    }
}

FillParameters::~FillParameters()
{
    this->clearBuffers();
}

FillParameters &FillParameters::operator =(const FillParameters &other)
{
    if (this != &other) {
        this->colorConvert = other.colorConvert;
        this->fillType = other.fillType;
        this->fillDataTypes = other.fillDataTypes;
        this->alphaMode = other.alphaMode;
        this->endianess = other.endianess;
        this->width = other.width;
        this->height = other.height;
        this->planeXo = other.planeXo;
        this->planeYo = other.planeYo;
        this->planeZo = other.planeZo;
        this->planeAo = other.planeAo;
        this->compXo = other.compXo;
        this->compYo = other.compYo;
        this->compZo = other.compZo;
        this->compAo = other.compAo;
        this->xoOffset = other.xoOffset;
        this->yoOffset = other.yoOffset;
        this->zoOffset = other.zoOffset;
        this->aoOffset = other.aoOffset;
        this->xoShift = other.xoShift;
        this->yoShift = other.yoShift;
        this->zoShift = other.zoShift;
        this->aoShift = other.aoShift;
        this->maskXo = other.maskXo;
        this->maskYo = other.maskYo;
        this->maskZo = other.maskZo;
        this->maskAo = other.maskAo;

        if (this->width > 0) {
            size_t oWidthDataSize = sizeof(int) * this->width;

            if (other.dstWidthOffsetX) {
                this->dstWidthOffsetX = new int [this->width];
                memcpy(this->dstWidthOffsetX, other.dstWidthOffsetX, oWidthDataSize);
            }

            if (other.dstWidthOffsetY) {
                this->dstWidthOffsetY = new int [this->width];
                memcpy(this->dstWidthOffsetY, other.dstWidthOffsetY, oWidthDataSize);
            }

            if (other.dstWidthOffsetZ) {
                this->dstWidthOffsetZ = new int [this->width];
                memcpy(this->dstWidthOffsetZ, other.dstWidthOffsetZ, oWidthDataSize);
            }

            if (other.dstWidthOffsetA) {
                this->dstWidthOffsetA = new int [this->width];
                memcpy(this->dstWidthOffsetA, other.dstWidthOffsetA, oWidthDataSize);
            }
        }
    }

    return *this;
}

void FillParameters::clearBuffers()
{
    if (this->dstWidthOffsetX) {
        delete [] this->dstWidthOffsetX;
        this->dstWidthOffsetX = nullptr;
    }

    if (this->dstWidthOffsetY) {
        delete [] this->dstWidthOffsetY;
        this->dstWidthOffsetY = nullptr;
    }

    if (this->dstWidthOffsetZ) {
        delete [] this->dstWidthOffsetZ;
        this->dstWidthOffsetZ = nullptr;
    }

    if (this->dstWidthOffsetA) {
        delete [] this->dstWidthOffsetA;
        this->dstWidthOffsetA = nullptr;
    }
}

void FillParameters::allocateBuffers(const AkVideoCaps &caps)
{
    this->clearBuffers();

    if (caps.width() > 0) {
        this->dstWidthOffsetX = new int [caps.width()];
        this->dstWidthOffsetY = new int [caps.width()];
        this->dstWidthOffsetZ = new int [caps.width()];
        this->dstWidthOffsetA = new int [caps.width()];
    }
}

#define DEFINE_FILL_TYPES(size) \
    if (ospecs.byteLength() == (size / 8)) \
        this->fillDataTypes = FillDataTypes_##size;

void FillParameters::configure(const AkVideoCaps &caps,
                               AkColorConvert &colorConvert)
{
    auto ispecs = AkVideoCaps::formatSpecs(AkVideoCaps::Format_argbpack);
    auto ospecs = AkVideoCaps::formatSpecs(caps.format());

    DEFINE_FILL_TYPES(8);
    DEFINE_FILL_TYPES(16);
    DEFINE_FILL_TYPES(32);

    auto components = ospecs.mainComponents();

    switch (components) {
    case 3:
        this->fillType =
            ospecs.type() == AkVideoFormatSpec::VFT_RGB?
                                FillType_Vector:
                                FillType_3;

        break;

    case 1:
        this->fillType = FillType_1;

        break;

    default:
        break;
    }

    this->endianess = ospecs.endianness();
    colorConvert.loadMatrix(ispecs, ospecs);

    switch (ospecs.type()) {
    case AkVideoFormatSpec::VFT_RGB:
        this->planeXo = ospecs.componentPlane(AkColorComponent::CT_R);
        this->planeYo = ospecs.componentPlane(AkColorComponent::CT_G);
        this->planeZo = ospecs.componentPlane(AkColorComponent::CT_B);

        this->compXo = ospecs.component(AkColorComponent::CT_R);
        this->compYo = ospecs.component(AkColorComponent::CT_G);
        this->compZo = ospecs.component(AkColorComponent::CT_B);

        break;

    case AkVideoFormatSpec::VFT_YUV:
        this->planeXo = ospecs.componentPlane(AkColorComponent::CT_Y);
        this->planeYo = ospecs.componentPlane(AkColorComponent::CT_U);
        this->planeZo = ospecs.componentPlane(AkColorComponent::CT_V);

        this->compXo = ospecs.component(AkColorComponent::CT_Y);
        this->compYo = ospecs.component(AkColorComponent::CT_U);
        this->compZo = ospecs.component(AkColorComponent::CT_V);

        break;

    case AkVideoFormatSpec::VFT_Gray:
        this->planeXo = ospecs.componentPlane(AkColorComponent::CT_Y);
        this->compXo = ospecs.component(AkColorComponent::CT_Y);

        break;

    default:
        break;
    }

    this->planeAo = ospecs.componentPlane(AkColorComponent::CT_A);
    this->compAo = ospecs.component(AkColorComponent::CT_A);

    this->xoOffset = this->compXo.offset();
    this->yoOffset = this->compYo.offset();
    this->zoOffset = this->compZo.offset();
    this->aoOffset = this->compAo.offset();

    this->xoShift = this->compXo.shift();
    this->yoShift = this->compYo.shift();
    this->zoShift = this->compZo.shift();
    this->aoShift = this->compAo.shift();

    this->maskXo = ~(this->compXo.max<quint64>() << this->compXo.shift());
    this->maskYo = ~(this->compYo.max<quint64>() << this->compYo.shift());
    this->maskZo = ~(this->compZo.max<quint64>() << this->compZo.shift());
    this->maskAo = ~(this->compAo.max<quint64>() << this->compAo.shift());

    this->alphaMode = ospecs.contains(AkColorComponent::CT_A)?
                          AlphaMode_AO:
                          AlphaMode_O;
}

void FillParameters::configureFill(const AkVideoCaps &caps)
{
    this->allocateBuffers(caps);

    for (int x = 0; x < caps.width(); ++x) {
        this->dstWidthOffsetX[x] = (x >> this->compXo.widthDiv()) * this->compXo.step();
        this->dstWidthOffsetY[x] = (x >> this->compYo.widthDiv()) * this->compYo.step();
        this->dstWidthOffsetZ[x] = (x >> this->compZo.widthDiv()) * this->compZo.step();
        this->dstWidthOffsetA[x] = (x >> this->compAo.widthDiv()) * this->compAo.step();
    }

    this->width = caps.width();
    this->height = caps.height();
}

void FillParameters::reset()
{
    this->fillType = FillType_3;
    this->fillDataTypes = FillDataTypes_8;
    this->alphaMode = AlphaMode_AO;

    this->endianess = Q_BYTE_ORDER;

    this->clearBuffers();

    this->width = 0;
    this->height = 0;

    this->planeXo = 0;
    this->planeYo = 0;
    this->planeZo = 0;
    this->planeAo = 0;

    this->compXo = {};
    this->compYo = {};
    this->compZo = {};
    this->compAo = {};

    this->xoOffset = 0;
    this->yoOffset = 0;
    this->zoOffset = 0;
    this->aoOffset = 0;

    this->xoShift = 0;
    this->yoShift = 0;
    this->zoShift = 0;
    this->aoShift = 0;

    this->maskXo = 0;
    this->maskYo = 0;
    this->maskZo = 0;
    this->maskAo = 0;
}

#include "moc_akvideopacket.cpp"
