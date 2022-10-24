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

#include <QQmlEngine>

#include "akvideomixer.h"
#include "akvideocaps.h"
#include "akvideoformatspec.h"
#include "akvideopacket.h"

enum DrawType
{
    DrawType_1_component,
    DrawType_3_components,
};

enum DrawDataTypes
{
    DrawDataTypes_8,
    DrawDataTypes_16,
    DrawDataTypes_32,
};

class DrawParameters
{
    public:
        AkVideoCaps inputCaps;
        AkVideoCaps outputCaps;
        int x {0};
        int y {0};
        DrawType drawType {DrawType_1_component};
        DrawDataTypes drawDataTypes {DrawDataTypes_8};
        bool hasAlpha {false};
        bool optimizedFor8bits {false};

        int iX {0};
        int iY {0};
        int iWidth {0};
        int iHeight {0};

        int oX {0};
        int oY {0};
        int oWidth {0};
        int oHeight {0};

        int endianness {Q_BYTE_ORDER};

        int *srcWidthOffsetX {nullptr};
        int *srcWidthOffsetY {nullptr};
        int *srcWidthOffsetZ {nullptr};
        int *srcWidthOffsetA {nullptr};
        int *srcHeight {nullptr};

        int planeXi {0};
        int planeYi {0};
        int planeZi {0};
        int planeAi {0};

        AkColorComponent compXi;
        AkColorComponent compYi;
        AkColorComponent compZi;
        AkColorComponent compAi;

        size_t xiOffset {0};
        size_t yiOffset {0};
        size_t ziOffset {0};
        size_t aiOffset {0};

        size_t xiShift {0};
        size_t yiShift {0};
        size_t ziShift {0};
        size_t aiShift {0};

        quint64 maxXi {0};
        quint64 maxYi {0};
        quint64 maxZi {0};
        quint64 maxAi {0};
        quint64 maxAi2 {0};

        quint64 maskXo {0};
        quint64 maskYo {0};
        quint64 maskZo {0};
        quint64 maskAo {0};

        size_t lengthAi {0};
        size_t alphaShift {0};
        qint64 *aiMultTable {nullptr};
        qint64 *aoMultTable {nullptr};
        qint64 *alphaDivTable {nullptr};

        ~DrawParameters();
        inline void allocateBuffers(const AkVideoCaps &caps);
        inline void clearBuffers();
        void configure(int x, int y,
                       const AkVideoCaps &icaps,
                       const AkVideoCaps &ocaps);
        void reset();
};

class AkVideoMixerPrivate
{
    public:
        AkVideoPacket *m_baseFrame {nullptr};
        DrawParameters *m_dp {nullptr};
        size_t m_dpSize {0};
        int m_cacheIndex {0};

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

        template <typename DataType>
        void draw8bits3A(const DrawParameters &dp,
                         const AkVideoPacket &src,
                         AkVideoPacket &dst) const
        {
            for (int y = dp.oY; y < dp.oHeight; ++y) {
                auto &ys = dp.srcHeight[y];

                auto src_line_x = src.constLine(dp.planeXi, ys) + dp.xiOffset;
                auto src_line_y = src.constLine(dp.planeYi, ys) + dp.yiOffset;
                auto src_line_z = src.constLine(dp.planeZi, ys) + dp.ziOffset;
                auto src_line_a = src.constLine(dp.planeAi, ys) + dp.aiOffset;

                auto dst_line_x = dst.line(dp.planeXi, y) + dp.xiOffset;
                auto dst_line_y = dst.line(dp.planeYi, y) + dp.yiOffset;
                auto dst_line_z = dst.line(dp.planeZi, y) + dp.ziOffset;
                auto dst_line_a = dst.line(dp.planeAi, y) + dp.aiOffset;

                for (int x = dp.oX; x < dp.oWidth; ++x) {
                    int &xs_x = dp.srcWidthOffsetX[x];
                    int &xs_y = dp.srcWidthOffsetY[x];
                    int &xs_z = dp.srcWidthOffsetZ[x];
                    int &xs_a = dp.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const DataType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const DataType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const DataType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const DataType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(DataType(xi), dp.endianness) >> dp.xiShift) & dp.maxXi;
                    yi = (this->swapBytes(DataType(yi), dp.endianness) >> dp.yiShift) & dp.maxYi;
                    zi = (this->swapBytes(DataType(zi), dp.endianness) >> dp.ziShift) & dp.maxZi;
                    ai = (this->swapBytes(DataType(ai), dp.endianness) >> dp.aiShift) & dp.maxAi;

                    auto xop = reinterpret_cast<DataType *>(dst_line_x + xs_x);
                    auto yop = reinterpret_cast<DataType *>(dst_line_y + xs_y);
                    auto zop = reinterpret_cast<DataType *>(dst_line_z + xs_z);
                    auto aop = reinterpret_cast<DataType *>(dst_line_a + xs_a);

                    auto xo = (this->swapBytes(DataType(*xop), dp.endianness) >> dp.xiShift) & dp.maxXi;
                    auto yo = (this->swapBytes(DataType(*yop), dp.endianness) >> dp.yiShift) & dp.maxYi;
                    auto zo = (this->swapBytes(DataType(*zop), dp.endianness) >> dp.ziShift) & dp.maxZi;
                    auto ao = (this->swapBytes(DataType(*aop), dp.endianness) >> dp.aiShift) & dp.maxAi;

                    auto alphaMask = (size_t(ai) << dp.lengthAi) | size_t(ao);
                    qint64 xt = (qint64(xi) * dp.aiMultTable[alphaMask] + qint64(xo) * dp.aoMultTable[alphaMask]) >> dp.alphaShift;
                    qint64 yt = (qint64(yi) * dp.aiMultTable[alphaMask] + qint64(yo) * dp.aoMultTable[alphaMask]) >> dp.alphaShift;
                    qint64 zt = (qint64(zi) * dp.aiMultTable[alphaMask] + qint64(zo) * dp.aoMultTable[alphaMask]) >> dp.alphaShift;
                    qint64 &at = dp.alphaDivTable[alphaMask];

                    *xop = (*xop & DataType(dp.maskXo)) | (DataType(xt) << dp.xiShift);
                    *yop = (*yop & DataType(dp.maskYo)) | (DataType(yt) << dp.yiShift);
                    *zop = (*zop & DataType(dp.maskZo)) | (DataType(zt) << dp.ziShift);
                    *aop = (*aop & DataType(dp.maskAo)) | (DataType(at) << dp.aiShift);

                    auto xot = this->swapBytes(DataType(*xop), dp.endianness);
                    auto yot = this->swapBytes(DataType(*yop), dp.endianness);
                    auto zot = this->swapBytes(DataType(*zop), dp.endianness);
                    auto aot = this->swapBytes(DataType(*aop), dp.endianness);

                    *xop = xot;
                    *yop = yot;
                    *zop = zot;
                    *aop = aot;
                }
            }
        }

        template <typename DataType>
        void draw3A(const DrawParameters &dp,
                    const AkVideoPacket &src,
                    AkVideoPacket &dst) const
        {
            for (int y = dp.oY; y < dp.oHeight; ++y) {
                auto &ys = dp.srcHeight[y];

                auto src_line_x = src.constLine(dp.planeXi, ys) + dp.xiOffset;
                auto src_line_y = src.constLine(dp.planeYi, ys) + dp.yiOffset;
                auto src_line_z = src.constLine(dp.planeZi, ys) + dp.ziOffset;
                auto src_line_a = src.constLine(dp.planeAi, ys) + dp.aiOffset;

                auto dst_line_x = dst.line(dp.planeXi, y) + dp.xiOffset;
                auto dst_line_y = dst.line(dp.planeYi, y) + dp.yiOffset;
                auto dst_line_z = dst.line(dp.planeZi, y) + dp.ziOffset;
                auto dst_line_a = dst.line(dp.planeAi, y) + dp.aiOffset;

                for (int x = dp.oX; x < dp.oWidth; ++x) {
                    int &xs_x = dp.srcWidthOffsetX[x];
                    int &xs_y = dp.srcWidthOffsetY[x];
                    int &xs_z = dp.srcWidthOffsetZ[x];
                    int &xs_a = dp.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const DataType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const DataType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const DataType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const DataType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(DataType(xi), dp.endianness) >> dp.xiShift) & dp.maxXi;
                    yi = (this->swapBytes(DataType(yi), dp.endianness) >> dp.yiShift) & dp.maxYi;
                    zi = (this->swapBytes(DataType(zi), dp.endianness) >> dp.ziShift) & dp.maxZi;
                    ai = (this->swapBytes(DataType(ai), dp.endianness) >> dp.aiShift) & dp.maxAi;

                    auto xop = reinterpret_cast<DataType *>(dst_line_x + xs_x);
                    auto yop = reinterpret_cast<DataType *>(dst_line_y + xs_y);
                    auto zop = reinterpret_cast<DataType *>(dst_line_z + xs_z);
                    auto aop = reinterpret_cast<DataType *>(dst_line_a + xs_a);

                    auto xo = (this->swapBytes(DataType(*xop), dp.endianness) >> dp.xiShift) & dp.maxXi;
                    auto yo = (this->swapBytes(DataType(*yop), dp.endianness) >> dp.yiShift) & dp.maxYi;
                    auto zo = (this->swapBytes(DataType(*zop), dp.endianness) >> dp.ziShift) & dp.maxZi;
                    auto ao = (this->swapBytes(DataType(*aop), dp.endianness) >> dp.aiShift) & dp.maxAi;

                    auto diffAi = qint64(dp.maxAi) - qint64(ai);
                    auto a = qint64(dp.maxAi2) - (qint64(dp.maxAi) - qint64(ao)) * diffAi;

                    qint64 xt = 0;
                    qint64 yt = 0;
                    qint64 zt = 0;
                    qint64 at = 0;

                    if (a != 0) {
                        auto mi = qint64(ai) * qint64(dp.maxAi);
                        auto mo = qint64(ao) * diffAi;
                        xt = (qint64(xi) * mi + qint64(xo) * mo) / a;
                        yt = (qint64(yi) * mi + qint64(yo) * mo) / a;
                        zt = (qint64(zi) * mi + qint64(zo) * mo) / a;
                        at = a / qint64(dp.maxAi);
                    }

                    *xop = (*xop & DataType(dp.maskXo)) | (DataType(xt) << dp.xiShift);
                    *yop = (*yop & DataType(dp.maskYo)) | (DataType(yt) << dp.yiShift);
                    *zop = (*zop & DataType(dp.maskZo)) | (DataType(zt) << dp.ziShift);
                    *aop = (*aop & DataType(dp.maskAo)) | (DataType(at) << dp.aiShift);

                    auto xot = this->swapBytes(DataType(*xop), dp.endianness);
                    auto yot = this->swapBytes(DataType(*yop), dp.endianness);
                    auto zot = this->swapBytes(DataType(*zop), dp.endianness);
                    auto aot = this->swapBytes(DataType(*aop), dp.endianness);

                    *xop = xot;
                    *yop = yot;
                    *zop = zot;
                    *aop = aot;
                }
            }
        }

        template <typename DataType>
        void draw3(const DrawParameters &dp,
                   const AkVideoPacket &src,
                   AkVideoPacket &dst) const
        {
            for (int y = dp.oY; y < dp.oHeight; ++y) {
                auto &ys = dp.srcHeight[y];

                auto src_line_x = src.constLine(dp.planeXi, ys) + dp.xiOffset;
                auto src_line_y = src.constLine(dp.planeYi, ys) + dp.yiOffset;
                auto src_line_z = src.constLine(dp.planeZi, ys) + dp.ziOffset;

                auto dst_line_x = dst.line(dp.planeXi, y) + dp.xiOffset;
                auto dst_line_y = dst.line(dp.planeYi, y) + dp.yiOffset;
                auto dst_line_z = dst.line(dp.planeZi, y) + dp.ziOffset;

                for (int x = dp.oX; x < dp.oWidth; ++x) {
                    int &xs_x = dp.srcWidthOffsetX[x];
                    int &xs_y = dp.srcWidthOffsetY[x];
                    int &xs_z = dp.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const DataType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const DataType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const DataType *>(src_line_z + xs_z);

                    xi = this->swapBytes(DataType(xi), dp.endianness);
                    yi = this->swapBytes(DataType(yi), dp.endianness);
                    zi = this->swapBytes(DataType(zi), dp.endianness);

                    auto xo = reinterpret_cast<DataType *>(dst_line_x + xs_x);
                    auto yo = reinterpret_cast<DataType *>(dst_line_y + xs_y);
                    auto zo = reinterpret_cast<DataType *>(dst_line_z + xs_z);

                    *xo = (*xo & DataType(dp.maskXo)) | (DataType(xi) << dp.xiShift);
                    *yo = (*yo & DataType(dp.maskYo)) | (DataType(yi) << dp.yiShift);
                    *zo = (*zo & DataType(dp.maskZo)) | (DataType(zi) << dp.ziShift);

                    auto xot = this->swapBytes(DataType(*xo), dp.endianness);
                    auto yot = this->swapBytes(DataType(*yo), dp.endianness);
                    auto zot = this->swapBytes(DataType(*zo), dp.endianness);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename DataType>
        void draw8bits1A(const DrawParameters &dp,
                         const AkVideoPacket &src,
                         AkVideoPacket &dst) const
        {
            for (int y = dp.oY; y < dp.oHeight; ++y) {
                auto &ys = dp.srcHeight[y];

                auto src_line_x = src.constLine(dp.planeXi, ys) + dp.xiOffset;
                auto src_line_a = src.constLine(dp.planeAi, ys) + dp.aiOffset;

                auto dst_line_x = dst.line(dp.planeXi, y) + dp.xiOffset;
                auto dst_line_a = dst.line(dp.planeAi, y) + dp.aiOffset;

                for (int x = dp.oX; x < dp.oWidth; ++x) {
                    int &xs_x = dp.srcWidthOffsetX[x];
                    int &xs_a = dp.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const DataType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const DataType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(DataType(xi), dp.endianness) >> dp.xiShift) & dp.maxXi;
                    ai = (this->swapBytes(DataType(ai), dp.endianness) >> dp.aiShift) & dp.maxAi;

                    auto xop = reinterpret_cast<DataType *>(dst_line_x + xs_x);
                    auto aop = reinterpret_cast<DataType *>(dst_line_a + xs_a);

                    auto xo = (this->swapBytes(DataType(*xop), dp.endianness) >> dp.xiShift) & dp.maxXi;
                    auto ao = (this->swapBytes(DataType(*aop), dp.endianness) >> dp.aiShift) & dp.maxAi;

                    auto alphaMask = (size_t(ai) << dp.lengthAi) | size_t(ao);
                    qint64 xt = (qint64(xi) * dp.aiMultTable[alphaMask] + qint64(xo) * dp.aoMultTable[alphaMask]) >> dp.alphaShift;
                    qint64 &at = dp.alphaDivTable[alphaMask];

                    *xop = (*xop & DataType(dp.maskXo)) | (DataType(xt) << dp.xiShift);
                    *aop = (*aop & DataType(dp.maskAo)) | (DataType(at) << dp.aiShift);

                    auto xot = this->swapBytes(DataType(*xop), dp.endianness);
                    auto aot = this->swapBytes(DataType(*aop), dp.endianness);

                    *xop = xot;
                    *aop = aot;
                }
            }
        }

        template <typename DataType>
        void draw1A(const DrawParameters &dp,
                    const AkVideoPacket &src,
                    AkVideoPacket &dst) const
        {
            for (int y = dp.oY; y < dp.oHeight; ++y) {
                auto &ys = dp.srcHeight[y];

                auto src_line_x = src.constLine(dp.planeXi, ys) + dp.xiOffset;
                auto src_line_a = src.constLine(dp.planeAi, ys) + dp.aiOffset;

                auto dst_line_x = dst.line(dp.planeXi, y) + dp.xiOffset;
                auto dst_line_a = dst.line(dp.planeAi, y) + dp.aiOffset;

                for (int x = dp.oX; x < dp.oWidth; ++x) {
                    int &xs_x = dp.srcWidthOffsetX[x];
                    int &xs_a = dp.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const DataType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const DataType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(DataType(xi), dp.endianness) >> dp.xiShift) & dp.maxXi;
                    ai = (this->swapBytes(DataType(ai), dp.endianness) >> dp.aiShift) & dp.maxAi;

                    auto xop = reinterpret_cast<DataType *>(dst_line_x + xs_x);
                    auto aop = reinterpret_cast<DataType *>(dst_line_a + xs_a);

                    auto xo = (this->swapBytes(DataType(*xop), dp.endianness) >> dp.xiShift) & dp.maxXi;
                    auto ao = (this->swapBytes(DataType(*aop), dp.endianness) >> dp.aiShift) & dp.maxAi;

                    auto diffAi = qint64(dp.maxAi) - qint64(ai);
                    auto a = qint64(dp.maxAi2) - (qint64(dp.maxAi) - qint64(ao)) * diffAi;

                    qint64 xt = 0;
                    qint64 at = 0;

                    if (a != 1) {
                        auto mi = qint64(ai) * qint64(dp.maxAi);
                        auto mo = qint64(ao) * diffAi;
                        xt = (qint64(xi) * mi + qint64(xo) * mo) / a;
                        at = a / qint64(dp.maxAi);
                    }

                    *xop = (*xop & DataType(dp.maskXo)) | (DataType(xt) << dp.xiShift);
                    *aop = (*aop & DataType(dp.maskAo)) | (DataType(at) << dp.aiShift);

                    auto xot = this->swapBytes(DataType(*xop), dp.endianness);
                    auto aot = this->swapBytes(DataType(*aop), dp.endianness);

                    *xop = xot;
                    *aop = aot;
                }
            }
        }

        template <typename DataType>
        void draw1(const DrawParameters &dp,
                   const AkVideoPacket &src,
                   AkVideoPacket &dst) const
        {
            for (int y = dp.oY; y < dp.oHeight; ++y) {
                auto &ys = dp.srcHeight[y];
                auto src_line_x = src.constLine(dp.planeXi, ys) + dp.xiOffset;
                auto dst_line_x = dst.line(dp.planeXi, y) + dp.xiOffset;

                for (int x = dp.oX; x < dp.oWidth; ++x) {
                    int &xs_x = dp.srcWidthOffsetX[x];

                    auto xi = *reinterpret_cast<const DataType *>(src_line_x + xs_x);
                    xi = (this->swapBytes(DataType(xi), dp.endianness) >> dp.xiShift) & dp.maxXi;

                    auto xo = reinterpret_cast<DataType *>(dst_line_x + xs_x);
                    *xo = (*xo & DataType(dp.maskXo)) | (DataType(xi) << dp.xiShift);
                    *xo = this->swapBytes(DataType(*xo), dp.endianness);
                }
            }
        }

#define DRAW_FUNC(components) \
        template <typename DataType> \
        inline void drawFrame##components(const DrawParameters &dp, \
                                          const AkVideoPacket &src, \
                                          AkVideoPacket &dst) const \
        { \
            if (dp.hasAlpha) { \
                if (dp.optimizedFor8bits) \
                    this->draw8bits##components##A<DataType>(dp, src, dst); \
                else \
                    this->draw##components##A<DataType>(dp, src, dst); \
            } else { \
                this->draw##components<DataType>(dp, src, dst); \
            } \
        }

        DRAW_FUNC(1)
        DRAW_FUNC(3)

        template <typename DataType>
        inline void draw(const DrawParameters &dp,
                         const AkVideoPacket &src,
                         AkVideoPacket &dst)
        {
            switch (dp.drawType) {
            case DrawType_3_components:
                this->drawFrame3<DataType>(dp, src, dst);
                break;
            case DrawType_1_component:
                this->drawFrame1<DataType>(dp, src, dst);
                break;
            }
        }

        inline void draw(int x, int y, const AkVideoPacket &packet);
};

AkVideoMixer::AkVideoMixer(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoMixerPrivate();
}

AkVideoMixer::AkVideoMixer(const AkVideoMixer &other):
    QObject()
{
    this->d = new AkVideoMixerPrivate();
    this->d->m_baseFrame = other.d->m_baseFrame;
}

AkVideoMixer::~AkVideoMixer()
{
    if (this->d->m_dp) {
        delete [] this->d->m_dp;
        this->d->m_dp = nullptr;
    }

    delete this->d;
}

AkVideoMixer &AkVideoMixer::operator =(const AkVideoMixer &other)
{
    if (this != &other) {
        this->d->m_baseFrame = other.d->m_baseFrame;
    }

    return *this;
}

QObject *AkVideoMixer::create()
{
    return new AkVideoMixer();
}

bool AkVideoMixer::begin(AkVideoPacket *packet)
{
    this->d->m_baseFrame = packet;
    this->d->m_cacheIndex = 0;

    return true;
}

void AkVideoMixer::end()
{
    this->d->m_baseFrame = nullptr;
    this->d->m_cacheIndex = 0;
}

void AkVideoMixer::draw(const AkVideoPacket &packet)
{
    this->draw(0, 0, packet);
}

bool AkVideoMixer::draw(int x, int y, const AkVideoPacket &packet)
{
    if (!this->d->m_baseFrame
        || !*this->d->m_baseFrame
        || !packet
        || packet.caps().format() != this->d->m_baseFrame->caps().format()) {
        return false;
    }

    this->d->draw(x, y, packet);

    return true;
}

void AkVideoMixer::reset()
{
    if (this->d->m_dp) {
        delete [] this->d->m_dp;
        this->d->m_dp = nullptr;
    }

    this->d->m_dpSize = 0;
}

void AkVideoMixer::registerTypes()
{
    qRegisterMetaType<AkVideoMixer>("AkVideoMixer");
    qmlRegisterSingletonType<AkVideoMixer>("Ak", 1, 0, "AkVideoMixer",
                                           [] (QQmlEngine *qmlEngine,
                                               QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkVideoMixer();
    });
}

DrawParameters::~DrawParameters()
{
    this->clearBuffers();
}

void DrawParameters::allocateBuffers(const AkVideoCaps &caps)
{
    this->clearBuffers();

    this->srcWidthOffsetX = new int [caps.width()];
    this->srcWidthOffsetY = new int [caps.width()];
    this->srcWidthOffsetZ = new int [caps.width()];
    this->srcWidthOffsetA = new int [caps.width()];
    this->srcHeight = new int [caps.height()];

    auto aBitLen = 1 << this->lengthAi;
    auto alphaMult = 1 << (2 * this->lengthAi);
    this->aiMultTable = new qint64 [alphaMult];
    this->aoMultTable = new qint64 [alphaMult];
    this->alphaDivTable = new qint64 [alphaMult];

    for (int ai = 0; ai < aBitLen; ai++)
        for (int ao = 0; ao < aBitLen; ao++) {
            auto alphaMask = (size_t(ai) << this->lengthAi) | size_t(ao);
            auto a = this->maxAi2 - (this->maxAi - ao) * (this->maxAi - ao);
            this->aiMultTable[alphaMask] = a? alphaMult * qint64(ai) * qint64(this->maxAi) / a: 0;
            this->aoMultTable[alphaMask] = a? alphaMult * qint64(ao) * (qint64(this->maxAi) - qint64(ai)) / a: 0;
            this->alphaDivTable[alphaMask] = a / this->maxAi;
        }
}

void DrawParameters::clearBuffers()
{
    if (this->srcWidthOffsetX) {
        delete [] this->srcWidthOffsetX;
        this->srcWidthOffsetX = nullptr;
    }

    if (this->srcWidthOffsetY) {
        delete [] this->srcWidthOffsetY;
        this->srcWidthOffsetY = nullptr;
    }

    if (this->srcWidthOffsetZ) {
        delete [] this->srcWidthOffsetZ;
        this->srcWidthOffsetZ = nullptr;
    }

    if (this->srcWidthOffsetA) {
        delete [] this->srcWidthOffsetA;
        this->srcWidthOffsetA = nullptr;
    }

    if (this->srcHeight) {
        delete [] this->srcHeight;
        this->srcHeight = nullptr;
    }

    if (this->aiMultTable) {
        delete [] this->aiMultTable;
        this->aiMultTable = nullptr;
    }

    if (this->aoMultTable) {
        delete [] this->aoMultTable;
        this->aoMultTable = nullptr;
    }

    if (this->alphaDivTable) {
        delete [] this->alphaDivTable;
        this->alphaDivTable = nullptr;
    }
}

#define DEFINE_DRAW_TYPES(size) \
    if (ispecs.byteLength() == (size / 8)) \
        this->drawDataTypes = DrawDataTypes_##size;

void DrawParameters::configure(int x, int y,
                               const AkVideoCaps &icaps,
                               const AkVideoCaps &ocaps)
{
    auto ispecs = AkVideoCaps::formatSpecs(icaps.format());

    DEFINE_DRAW_TYPES(8);
    DEFINE_DRAW_TYPES(16);
    DEFINE_DRAW_TYPES(32);

    switch (ispecs.mainComponents()) {
    case 1:
        this->drawType = DrawType_1_component;

        break;
    case 3:
        this->drawType = DrawType_3_components;

        break;
    default:
        break;
    }

    this->endianness = ispecs.endianness();

    switch (ispecs.type()) {
    case AkVideoFormatSpec::VFT_RGB:
        this->planeXi = ispecs.componentPlane(AkColorComponent::CT_R);
        this->planeYi = ispecs.componentPlane(AkColorComponent::CT_G);
        this->planeZi = ispecs.componentPlane(AkColorComponent::CT_B);

        this->compXi = ispecs.component(AkColorComponent::CT_R);
        this->compYi = ispecs.component(AkColorComponent::CT_G);
        this->compZi = ispecs.component(AkColorComponent::CT_B);

        break;

    case AkVideoFormatSpec::VFT_YUV:
        this->planeXi = ispecs.componentPlane(AkColorComponent::CT_Y);
        this->planeYi = ispecs.componentPlane(AkColorComponent::CT_U);
        this->planeZi = ispecs.componentPlane(AkColorComponent::CT_V);

        this->compXi = ispecs.component(AkColorComponent::CT_Y);
        this->compYi = ispecs.component(AkColorComponent::CT_U);
        this->compZi = ispecs.component(AkColorComponent::CT_V);

        break;

    default:
        break;
    }

    this->planeAi = ispecs.componentPlane(AkColorComponent::CT_A);
    this->compAi = ispecs.component(AkColorComponent::CT_A);

    this->xiOffset = this->compXi.offset();
    this->yiOffset = this->compYi.offset();
    this->ziOffset = this->compZi.offset();
    this->aiOffset = this->compAi.offset();

    this->xiShift = this->compXi.shift();
    this->yiShift = this->compYi.shift();
    this->ziShift = this->compZi.shift();
    this->aiShift = this->compAi.shift();

    this->maxXi = this->compXi.max<quint64>();
    this->maxYi = this->compYi.max<quint64>();
    this->maxZi = this->compZi.max<quint64>();
    this->maxAi = this->compAi.max<quint64>();
    this->maxAi2 = this->maxAi * this->maxAi;

    this->maskXo = ~(this->compXi.max<quint64>() << this->compXi.shift());
    this->maskYo = ~(this->compYi.max<quint64>() << this->compYi.shift());
    this->maskZo = ~(this->compZi.max<quint64>() << this->compZi.shift());
    this->maskAo = ~(this->compAi.max<quint64>() << this->compAi.shift());

    this->lengthAi = this->compAi.length();
    this->alphaShift = 2 * this->lengthAi;
    this->hasAlpha = ispecs.contains(AkColorComponent::CT_A);
    this->optimizedFor8bits = this->compAi.length() <= 8;

    if (x < 0) {
        this->iX = -x;
        this->oX = 0;
    } else {
        this->iX = 0;
        this->oX = x;
    }

    if (x + icaps.width() <= ocaps.width()) {
        this->iWidth = icaps.width();
        this->oWidth = icaps.width() + x;
    } else {
        this->iWidth = ocaps.width() - x;
        this->oWidth = ocaps.width();
    }

    if (y < 0) {
        this->iY = -y;
        this->oY = 0;
    } else {
        this->iY = 0;
        this->oY = y;
    }

    if (y + icaps.height() <= ocaps.height()) {
        this->iHeight = icaps.height();
        this->oHeight = icaps.height() + y;
    } else {
        this->iHeight = ocaps.height() - y;
        this->oHeight = ocaps.height();
    }

    this->allocateBuffers(ocaps);

    auto iDiffX = this->iWidth - this->iX - 1;
    auto oDiffX = this->oWidth - this->oX - 1;

    for (int x = 0; x < ocaps.width(); ++x) {
        auto xs = (x - this->oX) * (iDiffX + this->iX * oDiffX) / (oDiffX);

        this->srcWidthOffsetX[x] = (xs >> this->compXi.widthDiv()) * this->compXi.step();
        this->srcWidthOffsetY[x] = (xs >> this->compYi.widthDiv()) * this->compYi.step();
        this->srcWidthOffsetZ[x] = (xs >> this->compZi.widthDiv()) * this->compZi.step();
        this->srcWidthOffsetA[x] = (xs >> this->compAi.widthDiv()) * this->compAi.step();
    }

    auto iDiffY = this->iHeight - this->iY - 1;
    auto oDiffY = this->oHeight - this->oY - 1;

    for (int y = 0; y < ocaps.height(); ++y) {
        auto ys = (y - this->oY) * (iDiffY + this->iY * oDiffY) / (oDiffY);
        this->srcHeight[y] = ys;
    }
}

void DrawParameters::reset()
{
    this->inputCaps = AkVideoCaps();
    this->outputCaps = AkVideoCaps();
    this->drawType = DrawType_1_component;
    this->drawDataTypes = DrawDataTypes_8;
    this->hasAlpha = false;

    this->iX = 0;
    this->iY = 0;
    this->iWidth = 0;
    this->iHeight = 0;

    this->oX = 0;
    this->oY = 0;
    this->oWidth = 0;
    this->oHeight = 0;

    this->endianness = Q_BYTE_ORDER;

    this->clearBuffers();

    this->planeXi = 0;
    this->planeYi = 0;
    this->planeZi = 0;
    this->planeAi = 0;

    this->compXi = {};
    this->compYi = {};
    this->compZi = {};
    this->compAi = {};

    this->xiOffset = 0;
    this->yiOffset = 0;
    this->ziOffset = 0;
    this->aiOffset = 0;

    this->xiShift = 0;
    this->yiShift = 0;
    this->ziShift = 0;
    this->aiShift = 0;

    this->maxXi = 0;
    this->maxYi = 0;
    this->maxZi = 0;
    this->maxAi = 0;
    this->maxAi2 = 0;

    this->maskXo = 0;
    this->maskYo = 0;
    this->maskZo = 0;
    this->maskAo = 0;

    this->lengthAi = 0;
    this->alphaShift = 0;
    this->optimizedFor8bits = false;
}

#define DEFINE_DRAW_FUNC(size) \
    case DrawDataTypes_##size: \
        this->draw<quint##size>(dp, packet, *this->m_baseFrame); \
        break;

void AkVideoMixerPrivate::draw(int x, int y, const AkVideoPacket &packet)
{
    if (this->m_cacheIndex >= this->m_dpSize) {
        static const int cacheBlockSize = 8;
        auto newSize = (this->m_cacheIndex + cacheBlockSize) & ~(cacheBlockSize - 1);
        auto dp = new DrawParameters[newSize];

        for (int i = 0; i < this->m_dpSize; ++i)
            dp[i] = this->m_dp[i];

        delete [] this->m_dp;
        this->m_dp = dp;
        this->m_dpSize = newSize;
    }

    auto &dp = this->m_dp[this->m_cacheIndex];

    if (packet.caps() != dp.inputCaps
        || this->m_baseFrame->caps() != dp.outputCaps
        || x != dp.x
        || y != dp.y) {
        dp.configure(x,
                     y,
                     packet.caps(),
                     this->m_baseFrame->caps());
        dp.inputCaps = packet.caps();
        dp.outputCaps = this->m_baseFrame->caps();
        dp.x = x;
        dp.y = y;
    }

    switch (dp.drawDataTypes) {
    DEFINE_DRAW_FUNC(8)
    DEFINE_DRAW_FUNC(16)
    DEFINE_DRAW_FUNC(32)
    }

    this->m_cacheIndex++;
}

#include "moc_akvideomixer.cpp"
