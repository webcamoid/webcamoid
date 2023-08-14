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

#include <QQmlContext>
#include <QRect>
#include <akpacket.h>
#include <akvideoformatspec.h>
#include <akvideopacket.h>

#include "zoomelement.h"

#define SCALE_EMULT 8

enum ZoomType
{
    ZoomType_1_component,
    ZoomType_3_components,
};

enum ZoomDataTypes
{
    ZoomDataTypes_8,
    ZoomDataTypes_16,
    ZoomDataTypes_32,
};

class ZoomElementPrivate
{
    public:
        qreal m_zoom {1.0};
        qreal m_currentZoom {0.0};

        AkVideoCaps m_inputCaps;
        ZoomType m_zoomType {ZoomType_1_component};
        ZoomDataTypes m_zoomDataTypes {ZoomDataTypes_8};
        int m_endianness {Q_BYTE_ORDER};

        int m_inputWidth {0};
        int m_inputHeight {0};

        int *m_srcWidthOffsetX {nullptr};
        int *m_srcWidthOffsetY {nullptr};
        int *m_srcWidthOffsetZ {nullptr};
        int *m_srcWidthOffsetA {nullptr};
        int *m_srcHeight {nullptr};

        int *m_srcWidthOffsetX_1 {nullptr};
        int *m_srcWidthOffsetY_1 {nullptr};
        int *m_srcWidthOffsetZ_1 {nullptr};
        int *m_srcWidthOffsetA_1 {nullptr};
        int *m_srcHeight_1 {nullptr};

        int *m_dstWidthOffsetX {nullptr};
        int *m_dstWidthOffsetY {nullptr};
        int *m_dstWidthOffsetZ {nullptr};
        int *m_dstWidthOffsetA {nullptr};

        qint64 *m_kx {nullptr};
        qint64 *m_ky {nullptr};

        int m_planeXi {0};
        int m_planeYi {0};
        int m_planeZi {0};
        int m_planeAi {0};

        AkColorComponent m_compXi;
        AkColorComponent m_compYi;
        AkColorComponent m_compZi;
        AkColorComponent m_compAi;

        size_t m_xiOffset {0};
        size_t m_yiOffset {0};
        size_t m_ziOffset {0};
        size_t m_aiOffset {0};

        size_t m_xiShift {0};
        size_t m_yiShift {0};
        size_t m_ziShift {0};
        size_t m_aiShift {0};

        quint64 m_maxXi {0};
        quint64 m_maxYi {0};
        quint64 m_maxZi {0};
        quint64 m_maxAi {0};

        quint64 m_maskXi {0};
        quint64 m_maskYi {0};
        quint64 m_maskZi {0};
        quint64 m_maskAi {0};

        bool m_hasAlpha {false};

        inline void clearBuffers();
        inline void allocateBuffers(const AkVideoCaps &caps);
        void configure(const AkVideoCaps &caps);
        void configureScaling(const AkVideoCaps &caps, qreal zoom);

        /* Color blendig functions
         *
         * kx and ky must be in the range of [0, 2^N]
         */

        template <int N>
        inline void blend(qint64 a,
                          qint64 bx, qint64 by,
                          qint64 kx, qint64 ky,
                          qint64 *c) const
        {
            *c = (kx * (bx - a) + ky * (by - a) + (a << (N + 1))) >> (N + 1);
        }

        template <int N>
        inline void blend2(const qint64 *ax,
                           const qint64 *bx, const qint64 *by,
                           qint64 kx, qint64 ky,
                           qint64 *c) const
        {
            this->blend<N>(ax[0],
                           bx[0], by[0],
                           kx, ky,
                           c);
            this->blend<N>(ax[1],
                           bx[1], by[1],
                           kx, ky,
                           c + 1);
        }

        template <int N>
        inline void blend3(const qint64 *ax,
                           const qint64 *bx, const qint64 *by,
                           qint64 kx, qint64 ky,
                           qint64 *c) const
        {
            this->blend<N>(ax[0],
                           bx[0], by[0],
                           kx, ky,
                           c);
            this->blend<N>(ax[1],
                           bx[1], by[1],
                           kx, ky,
                           c + 1);
            this->blend<N>(ax[2],
                           bx[2], by[2],
                           kx, ky,
                           c + 2);
        }

        template <int N>
        inline void blend4(const qint64 *ax,
                           const qint64 *bx, const qint64 *by,
                           qint64 kx, qint64 ky,
                           qint64 *c) const
        {
            this->blend<N>(ax[0],
                           bx[0], by[0],
                           kx, ky,
                           c);
            this->blend<N>(ax[1],
                           bx[1], by[1],
                           kx, ky,
                           c + 1);
            this->blend<N>(ax[2],
                           bx[2], by[2],
                           kx, ky,
                           c + 2);
            this->blend<N>(ax[3],
                           bx[3], by[3],
                           kx, ky,
                           c + 3);
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

        template <typename DataType>
        void zoom3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < this->m_inputHeight; ++y) {
                auto &ys = this->m_srcHeight[y];
                auto &ys_1 = this->m_srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_xiOffset;
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_yiOffset;
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_ziOffset;
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_aiOffset;

                auto src_line_x_1 = src.constLine(this->m_planeXi, ys_1) + this->m_xiOffset;
                auto src_line_y_1 = src.constLine(this->m_planeYi, ys_1) + this->m_yiOffset;
                auto src_line_z_1 = src.constLine(this->m_planeZi, ys_1) + this->m_ziOffset;
                auto src_line_a_1 = src.constLine(this->m_planeAi, ys_1) + this->m_aiOffset;

                auto dst_line_x = dst.line(this->m_planeXi, y) + this->m_xiOffset;
                auto dst_line_y = dst.line(this->m_planeYi, y) + this->m_yiOffset;
                auto dst_line_z = dst.line(this->m_planeZi, y) + this->m_ziOffset;
                auto dst_line_a = dst.line(this->m_planeAi, y) + this->m_aiOffset;

                auto &ky = this->m_ky[y];

                for (int x = 0; x < this->m_inputWidth; ++x) {
                    int &xs_x = this->m_srcWidthOffsetX[x];
                    int &xs_y = this->m_srcWidthOffsetY[x];
                    int &xs_z = this->m_srcWidthOffsetZ[x];
                    int &xs_a = this->m_srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_srcWidthOffsetZ_1[x];
                    int &xs_a_1 = this->m_srcWidthOffsetA_1[x];

                    xyzai[0] = *reinterpret_cast<const DataType *>(src_line_x + xs_x);
                    xyzai[1] = *reinterpret_cast<const DataType *>(src_line_y + xs_y);
                    xyzai[2] = *reinterpret_cast<const DataType *>(src_line_z + xs_z);
                    xyzai[3] = *reinterpret_cast<const DataType *>(src_line_a + xs_a);
                    xyzai_x[0] = *reinterpret_cast<const DataType *>(src_line_x + xs_x_1);
                    xyzai_x[1] = *reinterpret_cast<const DataType *>(src_line_y + xs_y_1);
                    xyzai_x[2] = *reinterpret_cast<const DataType *>(src_line_z + xs_z_1);
                    xyzai_x[3] = *reinterpret_cast<const DataType *>(src_line_a + xs_a_1);
                    xyzai_y[0] = *reinterpret_cast<const DataType *>(src_line_x_1 + xs_x);
                    xyzai_y[1] = *reinterpret_cast<const DataType *>(src_line_y_1 + xs_y);
                    xyzai_y[2] = *reinterpret_cast<const DataType *>(src_line_z_1 + xs_z);
                    xyzai_y[3] = *reinterpret_cast<const DataType *>(src_line_a_1 + xs_a);

                    xyzai[0] = (this->swapBytes(DataType(xyzai[0]), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xyzai[1] = (this->swapBytes(DataType(xyzai[1]), this->m_endianness) >> this->m_yiShift) & this->m_maxYi;
                    xyzai[2] = (this->swapBytes(DataType(xyzai[2]), this->m_endianness) >> this->m_ziShift) & this->m_maxZi;
                    xyzai[3] = (this->swapBytes(DataType(xyzai[3]), this->m_endianness) >> this->m_aiShift) & this->m_maxAi;
                    xyzai_x[0] = (this->swapBytes(DataType(xyzai_x[0]), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xyzai_x[1] = (this->swapBytes(DataType(xyzai_x[1]), this->m_endianness) >> this->m_yiShift) & this->m_maxYi;
                    xyzai_x[2] = (this->swapBytes(DataType(xyzai_x[2]), this->m_endianness) >> this->m_ziShift) & this->m_maxZi;
                    xyzai_x[3] = (this->swapBytes(DataType(xyzai_x[3]), this->m_endianness) >> this->m_aiShift) & this->m_maxAi;
                    xyzai_y[0] = (this->swapBytes(DataType(xyzai_y[0]), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xyzai_y[1] = (this->swapBytes(DataType(xyzai_y[1]), this->m_endianness) >> this->m_yiShift) & this->m_maxYi;
                    xyzai_y[2] = (this->swapBytes(DataType(xyzai_y[2]), this->m_endianness) >> this->m_ziShift) & this->m_maxZi;
                    xyzai_y[3] = (this->swapBytes(DataType(xyzai_y[3]), this->m_endianness) >> this->m_aiShift) & this->m_maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              this->m_kx[x], ky,
                                              xyzaib);

                    int &xd_x = this->m_dstWidthOffsetX[x];
                    int &xd_y = this->m_dstWidthOffsetY[x];
                    int &xd_z = this->m_dstWidthOffsetZ[x];
                    int &xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<DataType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<DataType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<DataType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<DataType *>(dst_line_a + xd_a);

                    *xo = (*xo & DataType(this->m_maskXi)) | (DataType(xyzaib[0]) << this->m_xiShift);
                    *yo = (*yo & DataType(this->m_maskYi)) | (DataType(xyzaib[1]) << this->m_yiShift);
                    *zo = (*zo & DataType(this->m_maskZi)) | (DataType(xyzaib[2]) << this->m_ziShift);
                    *ao = (*ao & DataType(this->m_maskAi)) | (DataType(xyzaib[3]) << this->m_aiShift);

                    auto xot = this->swapBytes(DataType(*xo), this->m_endianness);
                    auto yot = this->swapBytes(DataType(*yo), this->m_endianness);
                    auto zot = this->swapBytes(DataType(*zo), this->m_endianness);
                    auto aot = this->swapBytes(DataType(*ao), this->m_endianness);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename DataType>
        void zoom3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < this->m_inputHeight; ++y) {
                auto &ys = this->m_srcHeight[y];
                auto &ys_1 = this->m_srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_xiOffset;
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_yiOffset;
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_ziOffset;

                auto src_line_x_1 = src.constLine(this->m_planeXi, ys_1) + this->m_xiOffset;
                auto src_line_y_1 = src.constLine(this->m_planeYi, ys_1) + this->m_yiOffset;
                auto src_line_z_1 = src.constLine(this->m_planeZi, ys_1) + this->m_ziOffset;

                auto dst_line_x = dst.line(this->m_planeXi, y) + this->m_xiOffset;
                auto dst_line_y = dst.line(this->m_planeYi, y) + this->m_yiOffset;
                auto dst_line_z = dst.line(this->m_planeZi, y) + this->m_ziOffset;

                auto &ky = this->m_ky[y];

                for (int x = 0; x < this->m_inputWidth; ++x) {
                    int &xs_x = this->m_srcWidthOffsetX[x];
                    int &xs_y = this->m_srcWidthOffsetY[x];
                    int &xs_z = this->m_srcWidthOffsetZ[x];

                    int &xs_x_1 = this->m_srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const DataType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const DataType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const DataType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const DataType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const DataType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const DataType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const DataType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const DataType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const DataType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(DataType(xyzi[0]), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xyzi[1] = (this->swapBytes(DataType(xyzi[1]), this->m_endianness) >> this->m_yiShift) & this->m_maxYi;
                    xyzi[2] = (this->swapBytes(DataType(xyzi[2]), this->m_endianness) >> this->m_ziShift) & this->m_maxZi;
                    xyzi_x[0] = (this->swapBytes(DataType(xyzi_x[0]), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xyzi_x[1] = (this->swapBytes(DataType(xyzi_x[1]), this->m_endianness) >> this->m_yiShift) & this->m_maxYi;
                    xyzi_x[2] = (this->swapBytes(DataType(xyzi_x[2]), this->m_endianness) >> this->m_ziShift) & this->m_maxZi;
                    xyzi_y[0] = (this->swapBytes(DataType(xyzi_y[0]), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xyzi_y[1] = (this->swapBytes(DataType(xyzi_y[1]), this->m_endianness) >> this->m_yiShift) & this->m_maxYi;
                    xyzi_y[2] = (this->swapBytes(DataType(xyzi_y[2]), this->m_endianness) >> this->m_ziShift) & this->m_maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              this->m_kx[x], ky,
                                              xyzib);

                    int &xd_x = this->m_dstWidthOffsetX[x];
                    int &xd_y = this->m_dstWidthOffsetY[x];
                    int &xd_z = this->m_dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<DataType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<DataType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<DataType *>(dst_line_z + xd_z);

                    *xo = (*xo & DataType(this->m_maskXi)) | (DataType(xyzib[0]) << this->m_xiShift);
                    *yo = (*yo & DataType(this->m_maskYi)) | (DataType(xyzib[1]) << this->m_yiShift);
                    *zo = (*zo & DataType(this->m_maskZi)) | (DataType(xyzib[2]) << this->m_ziShift);

                    auto xot = this->swapBytes(DataType(*xo), this->m_endianness);
                    auto yot = this->swapBytes(DataType(*yo), this->m_endianness);
                    auto zot = this->swapBytes(DataType(*zo), this->m_endianness);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename DataType>
        void zoom1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xai[2];
            qint64 xai_x[2];
            qint64 xai_y[2];
            qint64 xaib[2];

            for (int y = 0; y < this->m_inputHeight; ++y) {
                auto &ys = this->m_srcHeight[y];
                auto &ys_1 = this->m_srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_xiOffset;
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_aiOffset;
                auto src_line_x_1 = src.constLine(this->m_planeXi, ys_1) + this->m_xiOffset;
                auto src_line_a_1 = src.constLine(this->m_planeAi, ys_1) + this->m_aiOffset;

                auto dst_line_x = dst.line(this->m_planeXi, y) + this->m_xiOffset;
                auto dst_line_a = dst.line(this->m_planeAi, y) + this->m_aiOffset;

                auto &ky = this->m_ky[y];

                for (int x = 0; x < this->m_inputWidth; ++x) {
                    int &xs_x = this->m_srcWidthOffsetX[x];
                    int &xs_a = this->m_srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_srcWidthOffsetX_1[x];
                    int &xs_a_1 = this->m_srcWidthOffsetA_1[x];

                    xai[0] = *reinterpret_cast<const DataType *>(src_line_x + xs_x);
                    xai[1] = *reinterpret_cast<const DataType *>(src_line_a + xs_a);
                    xai_x[0] = *reinterpret_cast<const DataType *>(src_line_x + xs_x_1);
                    xai_x[1] = *reinterpret_cast<const DataType *>(src_line_a + xs_a_1);
                    xai_y[0] = *reinterpret_cast<const DataType *>(src_line_x_1 + xs_x);
                    xai_y[1] = *reinterpret_cast<const DataType *>(src_line_a_1 + xs_a);

                    xai[0] = (this->swapBytes(DataType(xai[0]), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xai[1] = (this->swapBytes(DataType(xai[1]), this->m_endianness) >> this->m_aiShift) & this->m_maxAi;
                    xai_x[0] = (this->swapBytes(DataType(xai_x[0]), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xai_x[1] = (this->swapBytes(DataType(xai_x[1]), this->m_endianness) >> this->m_aiShift) & this->m_maxAi;
                    xai_y[0] = (this->swapBytes(DataType(xai_y[0]), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xai_y[1] = (this->swapBytes(DataType(xai_y[1]), this->m_endianness) >> this->m_aiShift) & this->m_maxAi;

                    this->blend2<SCALE_EMULT>(xai,
                                              xai_x, xai_y,
                                              this->m_kx[x], ky,
                                              xaib);

                    int &xd_x = this->m_dstWidthOffsetX[x];
                    int &xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<DataType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<DataType *>(dst_line_a + xd_a);

                    *xo = (*xo & DataType(this->m_maskXi)) | (DataType(xaib[0]) << this->m_xiShift);
                    *ao = (*ao & DataType(this->m_maskAi)) | (DataType(xaib[1]) << this->m_aiShift);

                    auto xot = this->swapBytes(DataType(*xo), this->m_endianness);
                    auto aot = this->swapBytes(DataType(*ao), this->m_endianness);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename DataType>
        void zoom1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xib = 0;

            for (int y = 0; y < this->m_inputHeight; ++y) {
                auto &ys = this->m_srcHeight[y];
                auto &ys_1 = this->m_srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_xiOffset;
                auto src_line_x_1 = src.constLine(this->m_planeXi, ys_1) + this->m_xiOffset;

                auto dst_line_x = dst.line(this->m_planeXi, y) + this->m_xiOffset;

                auto &ky = this->m_ky[y];

                for (int x = 0; x < this->m_inputWidth; ++x) {
                    int &xs_x = this->m_srcWidthOffsetX[x];
                    int &xs_x_1 = this->m_srcWidthOffsetX_1[x];

                    auto xi = *reinterpret_cast<const DataType *>(src_line_x + xs_x);
                    auto xi_x = *reinterpret_cast<const DataType *>(src_line_x + xs_x_1);
                    auto xi_y = *reinterpret_cast<const DataType *>(src_line_x_1 + xs_x);

                    xi = (this->swapBytes(DataType(xi), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xi_x = (this->swapBytes(DataType(xi_x), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;
                    xi_y = (this->swapBytes(DataType(xi_y), this->m_endianness) >> this->m_xiShift) & this->m_maxXi;

                    this->blend<SCALE_EMULT>(xi,
                                             xi_x, xi_y,
                                             this->m_kx[x], ky,
                                             &xib);

                    int &xd_x = this->m_dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<DataType *>(dst_line_x + xd_x);
                    *xo = (*xo & DataType(this->m_maskXi)) | (DataType(xib) << this->m_xiShift);
                    *xo = this->swapBytes(DataType(*xo), this->m_endianness);
                }
            }
        }

#define ZOOM_FUNC(components) \
        template <typename DataType> \
        inline void zoomFrame##components(const AkVideoPacket &src, \
                                          AkVideoPacket &dst) const \
        { \
            if (this->m_hasAlpha) \
                this->zoom##components##A<DataType>(src, dst); \
            else \
                this->zoom##components<DataType>(src, dst); \
        }

        ZOOM_FUNC(1)
        ZOOM_FUNC(3)

        template <typename DataType>
        inline void zoom(const AkVideoPacket &src, AkVideoPacket &dst)
        {
            switch (this->m_zoomType) {
            case ZoomType_1_component:
                this->zoomFrame1<DataType>(src, dst);
                break;
            case ZoomType_3_components:
                this->zoomFrame3<DataType>(src, dst);
                break;
            }
        }
};

ZoomElement::ZoomElement(): AkElement()
{
    this->d = new ZoomElementPrivate;
}

ZoomElement::~ZoomElement()
{
    this->d->clearBuffers();
    delete this->d;
}

qreal ZoomElement::zoom() const
{
    return this->d->m_zoom;
}

QString ZoomElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Zoom/share/qml/main.qml");
}

void ZoomElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Zoom", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

#define DEFINE_ZOOM_FUNC(size) \
    case ZoomDataTypes_##size: \
        this->d->zoom<quint##size>(packet, dst); \
        break;

AkPacket ZoomElement::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    auto zoom = qMax(this->d->m_zoom, 1.0);

    if (qFuzzyCompare(zoom, 1.0)) {
        emit this->oStream(packet);

        return packet;
    }

    if (!packet.caps().isSameFormat(this->d->m_inputCaps)
        || !qFuzzyCompare(zoom, this->d->m_currentZoom)) {
        this->d->m_inputCaps = packet.caps();
        this->d->m_currentZoom = zoom;
        this->d->configure(packet.caps());
        this->d->configureScaling(packet.caps(), zoom);
    }

    AkVideoPacket dst(packet.caps());
    dst.copyMetadata(packet);

    switch (this->d->m_zoomDataTypes) {
    DEFINE_ZOOM_FUNC(8)
    DEFINE_ZOOM_FUNC(16)
    DEFINE_ZOOM_FUNC(32)
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void ZoomElement::setZoom(qreal zoom)
{
    if (qFuzzyCompare(this->d->m_zoom, zoom))
        return;

    this->d->m_zoom = zoom;
    emit this->zoomChanged(this->d->m_zoom);
}

void ZoomElement::resetZoom()
{
    this->setZoom(1.0);
}

void ZoomElementPrivate::clearBuffers()
{
    if (this->m_srcWidthOffsetX) {
        delete [] this->m_srcWidthOffsetX;
        this->m_srcWidthOffsetX = nullptr;
    }

    if (this->m_srcWidthOffsetY) {
        delete [] this->m_srcWidthOffsetY;
        this->m_srcWidthOffsetY = nullptr;
    }

    if (this->m_srcWidthOffsetZ) {
        delete [] this->m_srcWidthOffsetZ;
        this->m_srcWidthOffsetZ = nullptr;
    }

    if (this->m_srcWidthOffsetA) {
        delete [] this->m_srcWidthOffsetA;
        this->m_srcWidthOffsetA = nullptr;
    }

    if (this->m_srcHeight) {
        delete [] this->m_srcHeight;
        this->m_srcHeight = nullptr;
    }

    if (this->m_srcWidthOffsetX_1) {
        delete [] this->m_srcWidthOffsetX_1;
        this->m_srcWidthOffsetX_1 = nullptr;
    }

    if (this->m_srcWidthOffsetY_1) {
        delete [] this->m_srcWidthOffsetY_1;
        this->m_srcWidthOffsetY_1 = nullptr;
    }

    if (this->m_srcWidthOffsetZ_1) {
        delete [] this->m_srcWidthOffsetZ_1;
        this->m_srcWidthOffsetZ_1 = nullptr;
    }

    if (this->m_srcWidthOffsetA_1) {
        delete [] this->m_srcWidthOffsetA_1;
        this->m_srcWidthOffsetA_1 = nullptr;
    }

    if (this->m_srcHeight_1) {
        delete [] this->m_srcHeight_1;
        this->m_srcHeight_1 = nullptr;
    }

    if (this->m_dstWidthOffsetX) {
        delete [] this->m_dstWidthOffsetX;
        this->m_dstWidthOffsetX = nullptr;
    }

    if (this->m_dstWidthOffsetY) {
        delete [] this->m_dstWidthOffsetY;
        this->m_dstWidthOffsetY = nullptr;
    }

    if (this->m_dstWidthOffsetZ) {
        delete [] this->m_dstWidthOffsetZ;
        this->m_dstWidthOffsetZ = nullptr;
    }

    if (this->m_dstWidthOffsetA) {
        delete [] this->m_dstWidthOffsetA;
        this->m_dstWidthOffsetA = nullptr;
    }

    if (this->m_kx) {
        delete [] this->m_kx;
        this->m_kx = nullptr;
    }

    if (this->m_ky) {
        delete [] this->m_ky;
        this->m_ky = nullptr;
    }
}

void ZoomElementPrivate::allocateBuffers(const AkVideoCaps &caps)
{
    this->clearBuffers();

    this->m_srcWidthOffsetX = new int [caps.width()];
    this->m_srcWidthOffsetY = new int [caps.width()];
    this->m_srcWidthOffsetZ = new int [caps.width()];
    this->m_srcWidthOffsetA = new int [caps.width()];
    this->m_srcHeight = new int [caps.height()];

    this->m_srcWidthOffsetX_1 = new int [caps.width()];
    this->m_srcWidthOffsetY_1 = new int [caps.width()];
    this->m_srcWidthOffsetZ_1 = new int [caps.width()];
    this->m_srcWidthOffsetA_1 = new int [caps.width()];
    this->m_srcHeight_1 = new int [caps.height()];

    this->m_dstWidthOffsetX = new int [caps.width()];
    this->m_dstWidthOffsetY = new int [caps.width()];
    this->m_dstWidthOffsetZ = new int [caps.width()];
    this->m_dstWidthOffsetA = new int [caps.width()];

    this->m_kx = new qint64 [caps.width()];
    this->m_ky = new qint64 [caps.height()];
}

void ZoomElementPrivate::configure(const AkVideoCaps &caps)
{
    auto specs = AkVideoCaps::formatSpecs(caps.format());

    switch (specs.byteLength()) {
    case 1:
        this->m_zoomDataTypes = ZoomDataTypes_8;
        break;
    case 2:
        this->m_zoomDataTypes = ZoomDataTypes_16;
        break;
    case 4:
        this->m_zoomDataTypes = ZoomDataTypes_32;
        break;
    default:
        break;
    }

    auto components = specs.mainComponents();
    this->m_zoomType = components == 1?
                           ZoomType_1_component:
                           ZoomType_3_components;
    this->m_endianness = specs.endianness();

    switch (specs.type()) {
    case AkVideoFormatSpec::VFT_RGB:
        this->m_planeXi = specs.componentPlane(AkColorComponent::CT_R);
        this->m_planeYi = specs.componentPlane(AkColorComponent::CT_G);
        this->m_planeZi = specs.componentPlane(AkColorComponent::CT_B);

        this->m_compXi = specs.component(AkColorComponent::CT_R);
        this->m_compYi = specs.component(AkColorComponent::CT_G);
        this->m_compZi = specs.component(AkColorComponent::CT_B);

        break;

    case AkVideoFormatSpec::VFT_YUV:
        this->m_planeXi = specs.componentPlane(AkColorComponent::CT_Y);
        this->m_planeYi = specs.componentPlane(AkColorComponent::CT_U);
        this->m_planeZi = specs.componentPlane(AkColorComponent::CT_V);

        this->m_compXi = specs.component(AkColorComponent::CT_Y);
        this->m_compYi = specs.component(AkColorComponent::CT_U);
        this->m_compZi = specs.component(AkColorComponent::CT_V);

        break;

    case AkVideoFormatSpec::VFT_Gray:
        this->m_planeXi = specs.componentPlane(AkColorComponent::CT_Y);
        this->m_compXi = specs.component(AkColorComponent::CT_Y);

        break;

    default:
        break;
    }

    this->m_planeAi = specs.componentPlane(AkColorComponent::CT_A);
    this->m_compAi = specs.component(AkColorComponent::CT_A);

    this->m_xiOffset = this->m_compXi.offset();
    this->m_yiOffset = this->m_compYi.offset();
    this->m_ziOffset = this->m_compZi.offset();
    this->m_aiOffset = this->m_compAi.offset();

    this->m_xiShift = this->m_compXi.shift();
    this->m_yiShift = this->m_compYi.shift();
    this->m_ziShift = this->m_compZi.shift();
    this->m_aiShift = this->m_compAi.shift();

    this->m_maxXi = this->m_compXi.max<quint64>();
    this->m_maxYi = this->m_compYi.max<quint64>();
    this->m_maxZi = this->m_compZi.max<quint64>();
    this->m_maxAi = this->m_compAi.max<quint64>();

    this->m_maskXi = ~(this->m_compXi.max<quint64>() << this->m_compXi.shift());
    this->m_maskYi = ~(this->m_compYi.max<quint64>() << this->m_compYi.shift());
    this->m_maskZi = ~(this->m_compZi.max<quint64>() << this->m_compZi.shift());
    quint64 alphaMask = this->m_compAi.max<quint64>() << this->m_compAi.shift();
    this->m_maskAi = ~alphaMask;

    this->m_hasAlpha = specs.contains(AkColorComponent::CT_A);
}

void ZoomElementPrivate::configureScaling(const AkVideoCaps &caps, qreal zoom)
{
    this->allocateBuffers(caps);

    auto w = qRound(caps.width() / zoom);
    auto h = qRound(caps.height() / zoom);

    if (w > caps.width())
        w = caps.width();

    if (h > caps.height())
        h = caps.height();

    auto x = (caps.width() - w) / 2;
    auto y = (caps.height() - h) / 2;

    QRect inputRect(x, y, w, h);

    int wi_1 = qMax(1, inputRect.width() - 1);
    int wo_1 = qMax(1, caps.width() - 1);

    auto xSrcToDst = [&inputRect, &wi_1, &wo_1] (int x) -> int {
        return (x - inputRect.x()) * wo_1 / wi_1;
    };

    auto xDstToSrc = [&inputRect, &wi_1, &wo_1] (int x) -> int {
        return (x * wi_1 + inputRect.x() * wo_1) / wo_1;
    };

    for (int x = 0; x < caps.width(); ++x) {
        auto xs = xDstToSrc(x);
        auto xs_1 = xDstToSrc(qMin(x + 1, caps.width() - 1));
        auto xmin = xSrcToDst(xs);
        auto xmax = xSrcToDst(xs + 1);

        this->m_srcWidthOffsetX[x] = (xs >> this->m_compXi.widthDiv()) * this->m_compXi.step();
        this->m_srcWidthOffsetY[x] = (xs >> this->m_compYi.widthDiv()) * this->m_compYi.step();
        this->m_srcWidthOffsetZ[x] = (xs >> this->m_compZi.widthDiv()) * this->m_compZi.step();
        this->m_srcWidthOffsetA[x] = (xs >> this->m_compAi.widthDiv()) * this->m_compAi.step();

        this->m_srcWidthOffsetX_1[x] = (xs_1 >> this->m_compXi.widthDiv()) * this->m_compXi.step();
        this->m_srcWidthOffsetY_1[x] = (xs_1 >> this->m_compYi.widthDiv()) * this->m_compYi.step();
        this->m_srcWidthOffsetZ_1[x] = (xs_1 >> this->m_compZi.widthDiv()) * this->m_compZi.step();
        this->m_srcWidthOffsetA_1[x] = (xs_1 >> this->m_compAi.widthDiv()) * this->m_compAi.step();

        this->m_dstWidthOffsetX[x] = (x >> this->m_compXi.widthDiv()) * this->m_compXi.step();
        this->m_dstWidthOffsetY[x] = (x >> this->m_compYi.widthDiv()) * this->m_compYi.step();
        this->m_dstWidthOffsetZ[x] = (x >> this->m_compZi.widthDiv()) * this->m_compZi.step();
        this->m_dstWidthOffsetA[x] = (x >> this->m_compAi.widthDiv()) * this->m_compAi.step();

        if (xmax > xmin)
            this->m_kx[x] = SCALE_EMULT * (x - xmin) / (xmax - xmin);
        else
            this->m_kx[x] = 0;
    }

    int hi_1 = qMax(1, inputRect.height() - 1);
    int ho_1 = qMax(1, caps.height() - 1);

    auto ySrcToDst = [&inputRect, &hi_1, &ho_1] (int y) -> int {
        return (y - inputRect.y()) * ho_1 / hi_1;
    };

    auto yDstToSrc = [&inputRect, &hi_1, &ho_1] (int y) -> int {
        return (y * hi_1 + inputRect.y() * ho_1) / ho_1;
    };

    for (int y = 0; y < caps.height(); ++y) {
        auto ys = yDstToSrc(y);
        auto ys_1 = yDstToSrc(qMin(y + 1, caps.height() - 1));
        auto ymin = ySrcToDst(ys);
        auto ymax = ySrcToDst(ys + 1);

        this->m_srcHeight[y] = ys;
        this->m_srcHeight_1[y] = ys_1;

        if (ymax > ymin)
            this->m_ky[y] = SCALE_EMULT * (y - ymin) / (ymax - ymin);
        else
            this->m_ky[y] = 0;
    }

    this->m_inputWidth = caps.width();
    this->m_inputHeight = caps.height();
}

#include "moc_zoomelement.cpp"
