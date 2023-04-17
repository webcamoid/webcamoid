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
#include <QGenericMatrix>
#include <QMutex>
#include <QQmlEngine>
#include <QRect>
#include <QtEndian>
#include <QtMath>

#include "akvideoconverter.h"
#include "akvideocaps.h"
#include "akvideopacket.h"
#include "akvideoformatspec.h"
#include "akfrac.h"

#define SCALE_EMULT 8

/*
 * NOTE: Using integer numbers is much faster but can overflow with high
 * resolution and depth frames.
 */

#if 0
using DlSumType = quint64;
#else
using DlSumType = qreal;
#endif

enum ConvertType
{
    ConvertType_Vector,
    ConvertType_1to1,
    ConvertType_1to3,
    ConvertType_3to1,
    ConvertType_3to3,
};

enum ConvertDataTypes
{
    ConvertDataTypes_8_8,
    ConvertDataTypes_8_16,
    ConvertDataTypes_8_32,
    ConvertDataTypes_16_8,
    ConvertDataTypes_16_16,
    ConvertDataTypes_16_32,
    ConvertDataTypes_32_8,
    ConvertDataTypes_32_16,
    ConvertDataTypes_32_32,
};

enum AlphaMode
{
    AlphaMode_AI_AO,
    AlphaMode_AI_O,
    AlphaMode_I_AO,
    AlphaMode_I_O,
};

enum ResizeMode
{
    ResizeMode_Keep,
    ResizeMode_Up,
    ResizeMode_Down,
};

class FrameConvertParameters
{
    public:
        AkColorConvert colorConvert;

        AkVideoCaps inputCaps;
        AkVideoCaps outputCaps;
        AkVideoCaps outputConvertCaps;
        AkVideoPacket outputFrame;
        QRect inputRect;
        AkColorConvert::YuvColorSpace yuvColorSpace {AkColorConvert::YuvColorSpace_ITUR_BT601};
        AkColorConvert::YuvColorSpaceType yuvColorSpaceType {AkColorConvert::YuvColorSpaceType_StudioSwing};
        AkVideoConverter::ScalingMode scalingMode {AkVideoConverter::ScalingMode_Fast};
        AkVideoConverter::AspectRatioMode aspectRatioMode {AkVideoConverter::AspectRatioMode_Ignore};
        ConvertType convertType {ConvertType_Vector};
        ConvertDataTypes convertDataTypes {ConvertDataTypes_8_8};
        AlphaMode alphaMode {AlphaMode_AI_AO};
        ResizeMode resizeMode {ResizeMode_Keep};

        int fromEndian {Q_BYTE_ORDER};
        int toEndian {Q_BYTE_ORDER};

        int inputWidth {0};
        int inputWidth_1 {0};
        int inputHeight {0};
        int outputWidth {0};
        int outputHeight {0};

        int *srcWidth {nullptr};
        int *srcWidth_1 {nullptr};
        int *srcWidthOffsetX {nullptr};
        int *srcWidthOffsetY {nullptr};
        int *srcWidthOffsetZ {nullptr};
        int *srcWidthOffsetA {nullptr};
        int *srcHeight {nullptr};

        int *dlSrcWidthOffsetX {nullptr};
        int *dlSrcWidthOffsetY {nullptr};
        int *dlSrcWidthOffsetZ {nullptr};
        int *dlSrcWidthOffsetA {nullptr};

        int *srcWidthOffsetX_1 {nullptr};
        int *srcWidthOffsetY_1 {nullptr};
        int *srcWidthOffsetZ_1 {nullptr};
        int *srcWidthOffsetA_1 {nullptr};
        int *srcHeight_1 {nullptr};

        int *dstWidthOffsetX {nullptr};
        int *dstWidthOffsetY {nullptr};
        int *dstWidthOffsetZ {nullptr};
        int *dstWidthOffsetA {nullptr};

        size_t *srcHeightDlOffset {nullptr};
        size_t *srcHeightDlOffset_1 {nullptr};

        DlSumType *integralImageDataX {nullptr};
        DlSumType *integralImageDataY {nullptr};
        DlSumType *integralImageDataZ {nullptr};
        DlSumType *integralImageDataA {nullptr};

        qint64 *kx {nullptr};
        qint64 *ky {nullptr};
        DlSumType *kdl {nullptr};

        int planeXi {0};
        int planeYi {0};
        int planeZi {0};
        int planeAi {0};

        AkColorComponent compXi;
        AkColorComponent compYi;
        AkColorComponent compZi;
        AkColorComponent compAi;

        int planeXo {0};
        int planeYo {0};
        int planeZo {0};
        int planeAo {0};

        AkColorComponent compXo;
        AkColorComponent compYo;
        AkColorComponent compZo;
        AkColorComponent compAo;

        size_t xiOffset {0};
        size_t yiOffset {0};
        size_t ziOffset {0};
        size_t aiOffset {0};

        size_t xoOffset {0};
        size_t yoOffset {0};
        size_t zoOffset {0};
        size_t aoOffset {0};

        size_t xiShift {0};
        size_t yiShift {0};
        size_t ziShift {0};
        size_t aiShift {0};

        size_t xoShift {0};
        size_t yoShift {0};
        size_t zoShift {0};
        size_t aoShift {0};

        quint64 maxXi {0};
        quint64 maxYi {0};
        quint64 maxZi {0};
        quint64 maxAi {0};

        quint64 maskXo {0};
        quint64 maskYo {0};
        quint64 maskZo {0};
        quint64 maskAo {0};

        quint64 alphaMask {0};

        FrameConvertParameters();
        FrameConvertParameters(const FrameConvertParameters &other);
        ~FrameConvertParameters();
        FrameConvertParameters &operator =(const FrameConvertParameters &other);
        inline void clearBuffers();
        inline void clearDlBuffers();
        inline void allocateBuffers(const AkVideoCaps &ocaps);
        inline void allocateDlBuffers(const AkVideoCaps &icaps,
                                      const AkVideoCaps &ocaps);
        void configure(const AkVideoCaps &icaps,
                       const AkVideoCaps &ocaps,
                       AkColorConvert &colorConvert,
                       AkColorConvert::YuvColorSpace yuvColorSpace,
                       AkColorConvert::YuvColorSpaceType yuvColorSpaceType);
        void configureScaling(const AkVideoCaps &icaps,
                              const AkVideoCaps &ocaps,
                              const QRect &inputRect,
                              AkVideoConverter::AspectRatioMode aspectRatioMode);
        void reset();
};

class AkVideoConverterPrivate
{
    public:
        QMutex m_mutex;
        AkVideoCaps m_outputCaps;
        FrameConvertParameters *m_fc {nullptr};
        size_t m_fcSize {0};
        int m_cacheIndex {0};
        AkColorConvert::YuvColorSpace m_yuvColorSpace {AkColorConvert::YuvColorSpace_ITUR_BT601};
        AkColorConvert::YuvColorSpaceType m_yuvColorSpaceType {AkColorConvert::YuvColorSpaceType_StudioSwing};
        AkVideoConverter::ScalingMode m_scalingMode {AkVideoConverter::ScalingMode_Fast};
        AkVideoConverter::AspectRatioMode m_aspectRatioMode {AkVideoConverter::AspectRatioMode_Ignore};
        QRect m_inputRect;

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

        /* Integral image functions */

        template <typename InputType>
        inline void integralImage1(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src) const
        {
            auto dst_line_x = fc.integralImageDataX;
            auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;

            for (int y = 0; y < fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;

                // Reset current line summation.

                DlSumType sumX = 0;

                for (int x = 0; x < fc.inputWidth; ++x) {
                    int &xs_x = fc.dlSrcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);

                    // Accumulate pixels in current line.

                    sumX += (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                }

                dst_line_x += fc.inputWidth_1;
                dst_line_x_1 += fc.inputWidth_1;
            }
        }

        template <typename InputType>
        inline void integralImage1A(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src) const
        {
            auto dst_line_x = fc.integralImageDataX;
            auto dst_line_a = fc.integralImageDataA;
            auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;
            auto dst_line_a_1 = dst_line_a + fc.inputWidth_1;

            for (int y = 0; y < fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, y) + fc.aiOffset;

                // Reset current line summation.

                DlSumType sumX = 0;
                DlSumType sumA = 0;

                for (int x = 0; x < fc.inputWidth; ++x) {
                    int &xs_x = fc.dlSrcWidthOffsetX[x];
                    int &xs_a = fc.dlSrcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    // Accumulate pixels in current line.

                    sumX += (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    sumA += (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                    dst_line_a_1[x_1] = sumA + dst_line_a[x_1];
                }

                dst_line_x += fc.inputWidth_1;
                dst_line_a += fc.inputWidth_1;
                dst_line_x_1 += fc.inputWidth_1;
                dst_line_a_1 += fc.inputWidth_1;
            }
        }

        template <typename InputType>
        inline void integralImage3(const FrameConvertParameters &fc,
                                   const AkVideoPacket &src) const
        {
            auto dst_line_x = fc.integralImageDataX;
            auto dst_line_y = fc.integralImageDataY;
            auto dst_line_z = fc.integralImageDataZ;
            auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;
            auto dst_line_y_1 = dst_line_y + fc.inputWidth_1;
            auto dst_line_z_1 = dst_line_z + fc.inputWidth_1;

            for (int y = 0; y < fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, y) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, y) + fc.ziOffset;

                // Reset current line summation.

                DlSumType sumX = 0;
                DlSumType sumY = 0;
                DlSumType sumZ = 0;

                for (int x = 0; x < fc.inputWidth; ++x) {
                    int &xs_x = fc.dlSrcWidthOffsetX[x];
                    int &xs_y = fc.dlSrcWidthOffsetY[x];
                    int &xs_z = fc.dlSrcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    // Accumulate pixels in current line.

                    sumX += (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    sumY += (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    sumZ += (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                    dst_line_y_1[x_1] = sumY + dst_line_y[x_1];
                    dst_line_z_1[x_1] = sumZ + dst_line_z[x_1];
                }

                dst_line_x += fc.inputWidth_1;
                dst_line_y += fc.inputWidth_1;
                dst_line_z += fc.inputWidth_1;
                dst_line_x_1 += fc.inputWidth_1;
                dst_line_y_1 += fc.inputWidth_1;
                dst_line_z_1 += fc.inputWidth_1;
            }
        }

        template <typename InputType>
        inline void integralImage3A(const FrameConvertParameters &fc,
                                    const AkVideoPacket &src) const
        {
            auto dst_line_x = fc.integralImageDataX;
            auto dst_line_y = fc.integralImageDataY;
            auto dst_line_z = fc.integralImageDataZ;
            auto dst_line_a = fc.integralImageDataA;
            auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;
            auto dst_line_y_1 = dst_line_y + fc.inputWidth_1;
            auto dst_line_z_1 = dst_line_z + fc.inputWidth_1;
            auto dst_line_a_1 = dst_line_a + fc.inputWidth_1;

            for (int y = 0; y < fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, y) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, y) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, y) + fc.aiOffset;

                // Reset current line summation.

                DlSumType sumX = 0;
                DlSumType sumY = 0;
                DlSumType sumZ = 0;
                DlSumType sumA = 0;

                for (int x = 0; x < fc.inputWidth; ++x) {
                    int &xs_x = fc.dlSrcWidthOffsetX[x];
                    int &xs_y = fc.dlSrcWidthOffsetY[x];
                    int &xs_z = fc.dlSrcWidthOffsetZ[x];
                    int &xs_a = fc.dlSrcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    // Accumulate pixels in current line.

                    sumX += (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    sumY += (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    sumZ += (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    sumA += (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                    dst_line_y_1[x_1] = sumY + dst_line_y[x_1];
                    dst_line_z_1[x_1] = sumZ + dst_line_z[x_1];
                    dst_line_a_1[x_1] = sumA + dst_line_a[x_1];
                }

                dst_line_x += fc.inputWidth_1;
                dst_line_y += fc.inputWidth_1;
                dst_line_z += fc.inputWidth_1;
                dst_line_a += fc.inputWidth_1;
                dst_line_x_1 += fc.inputWidth_1;
                dst_line_y_1 += fc.inputWidth_1;
                dst_line_z_1 += fc.inputWidth_1;
                dst_line_a_1 += fc.inputWidth_1;
            }
        }

        /* Fast conversion functions */

        // Conversion functions for 3 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convert3to3(const FrameConvertParameters &fc,
                         const AkVideoPacket &src,
                         AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3to3A(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato3(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);
                    fc.colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato3A(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(ai) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 3 components to 3 components formats
        // (same color space)

        template <typename InputType, typename OutputType>
        void convertV3to3(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertV3to3A(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertV3Ato3(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);
                    fc.colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertV3Ato3A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(ai) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 3 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convert3to1(const FrameConvertParameters &fc,
                         const AkVideoPacket &src,
                         AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3to1A(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato1(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo_);
                    fc.colorConvert.applyAlpha(ai, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato1A(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(ai) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convert1to3(const FrameConvertParameters &fc,
                         const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1to3A(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato3(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    ai = (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);
                    fc.colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato3A(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    ai = (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(ai) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convert1to1(const FrameConvertParameters &fc,
                         const AkVideoPacket &src,
                         AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1to1A(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato1(const FrameConvertParameters &fc,
                          const AkVideoPacket &src,
                          AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    ai = (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_);
                    fc.colorConvert.applyAlpha(ai, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato1A(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    ai = (this->swapBytes(InputType(ai), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(ai) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        /* Linear downscaling conversion funtions */

        // Conversion functions for 3 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertDL3to3(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3to3A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo_,
                                                &yo_,
                                                &zo_);
                    fc.colorConvert.applyAlpha(ai,
                                               &xo_,
                                               &yo_,
                                               &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xi,
                                                yi,
                                                zi,
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(ai) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }

                kdl += fc.inputWidth;
            }
        }

        // Conversion functions for 3 components to 3 components formats
        // (same color space)

        template <typename InputType, typename OutputType>
        void convertDLV3to3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDLV3to3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDLV3Ato3(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo_,
                                                &yo_,
                                                &zo_);
                    fc.colorConvert.applyAlpha(ai,
                                               &xo_,
                                               &yo_,
                                               &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDLV3Ato3A(const FrameConvertParameters &fc,
                              const AkVideoPacket &src,
                              AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xi,
                                                yi,
                                                zi,
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(ai) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }

                kdl += fc.inputWidth;
            }
        }

        // Conversion functions for 3 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertDL3to1(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y);

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3to1A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato1(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y);

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo_);
                    fc.colorConvert.applyAlpha(ai, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato1A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_y = fc.integralImageDataY + yOffset;
                auto src_line_z = fc.integralImageDataZ + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, yi, zi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(ai) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }

                kdl += fc.inputWidth;
            }
        }

        // Conversion functions for 1 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertDL1to3(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1to3A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);
                    fc.colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(ai) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }

                kdl += fc.inputWidth;
            }
        }

        // Conversion functions for 1 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertDL1to1(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1to1A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato1(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_);
                    fc.colorConvert.applyAlpha(ai, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }

                kdl += fc.inputWidth;
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato1A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            auto kdl = fc.kdl;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &yOffset = fc.srcHeightDlOffset[y];
                auto &y1Offset = fc.srcHeightDlOffset_1[y];

                auto src_line_x = fc.integralImageDataX + yOffset;
                auto src_line_a = fc.integralImageDataA + yOffset;

                auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                for (int x = 0; x < fc.outputWidth; ++x) {
                    auto &xs = fc.srcWidth[x];
                    auto &xs_1 = fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(ai) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }

                kdl += fc.inputWidth;
            }
        }

        /* Linear upscaling conversion funtions */

        // Conversion functions for 3 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertUL3to3(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xyzib[0],
                                                xyzib[1],
                                                xyzib[2],
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3to3A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xyzib[0],
                                                xyzib[1],
                                                xyzib[2],
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                    xyzai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzai[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzai[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzai[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xyzai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzai_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzai_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzai_x[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xyzai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzai_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzai_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);
                    xyzai_y[3] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xyzaib[0],
                                                xyzaib[1],
                                                xyzaib[2],
                                                &xo_,
                                                &yo_,
                                                &zo_);
                    fc.colorConvert.applyAlpha(xyzaib[3],
                                               &xo_,
                                               &yo_,
                                               &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                    xyzai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzai[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzai[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzai[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xyzai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzai_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzai_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzai_x[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xyzai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzai_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzai_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);
                    xyzai_y[3] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyMatrix(xyzaib[0],
                                                xyzaib[1],
                                                xyzaib[2],
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(xyzaib[3]) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 3 components to 3 components formats
        // (same color space)

        template <typename InputType, typename OutputType>
        void convertULV3to3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xyzib[0],
                                                xyzib[1],
                                                xyzib[2],
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertULV3to3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xyzib[0],
                                                xyzib[1],
                                                xyzib[2],
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertULV3Ato3(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                    xyzai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzai[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzai[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzai[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xyzai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzai_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzai_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzai_x[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xyzai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzai_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzai_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);
                    xyzai_y[3] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xyzaib[0],
                                                xyzaib[1],
                                                xyzaib[2],
                                                &xo_,
                                                &yo_,
                                                &zo_);
                    fc.colorConvert.applyAlpha(xyzaib[3],
                                               &xo_,
                                               &yo_,
                                               &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertULV3Ato3A(const FrameConvertParameters &fc,
                              const AkVideoPacket &src,
                              AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                    xyzai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzai[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzai[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzai[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xyzai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzai_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzai_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzai_x[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xyzai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzai_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzai_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);
                    xyzai_y[3] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyVector(xyzaib[0],
                                                xyzaib[1],
                                                xyzaib[2],
                                                &xo_,
                                                &yo_,
                                                &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(xyzaib[3]) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 3 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertUL3to1(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y);

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xyzib[0],
                                               xyzib[1],
                                               xyzib[2],
                                               &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3to1A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xyzib[0],
                                               xyzib[1],
                                               xyzib[2],
                                               &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato1(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y);

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                    xyzai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzai[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzai[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzai[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xyzai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzai_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzai_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzai_x[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xyzai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzai_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzai_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);
                    xyzai_y[3] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xyzaib[0],
                                               xyzaib[1],
                                               xyzaib[2],
                                               &xo_);
                    fc.colorConvert.applyAlpha(xyzaib[3], &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato1A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_y = fc.srcWidthOffsetY[x];
                    int &xs_z = fc.srcWidthOffsetZ[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                    xyzai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzai[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzai[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzai[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xyzai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzai_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzai_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzai_x[3] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xyzai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzai_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzai_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);
                    xyzai_y[3] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), fc.fromEndian) >> fc.yiShift) & fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), fc.fromEndian) >> fc.ziShift) & fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xyzaib[0],
                                               xyzaib[1],
                                               xyzaib[2],
                                               &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(xyzaib[3]) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertUL1to3(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            qint64 xib = 0;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto xi_x = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    auto xi_y = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xi_x = (this->swapBytes(InputType(xi_x), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xi_y = (this->swapBytes(InputType(xi_y), fc.fromEndian) >> fc.xiShift) & fc.maxXi;

                    this->blend<SCALE_EMULT>(xi,
                                             xi_x, xi_y,
                                             fc.kx[x], ky,
                                             &xib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xib, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1to3A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            qint64 xib = 0;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto xi_x = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    auto xi_y = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xi_x = (this->swapBytes(InputType(xi_x), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xi_y = (this->swapBytes(InputType(xi_y), fc.fromEndian) >> fc.xiShift) & fc.maxXi;

                    this->blend<SCALE_EMULT>(xi,
                                             xi_x, xi_y,
                                             fc.kx[x], ky,
                                             &xib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xib, &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato3(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            qint64 xai[2];
            qint64 xai_x[2];
            qint64 xai_y[2];
            qint64 xaib[2];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                    xai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xai[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xai_x[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xai_y[1] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xai[0] = (this->swapBytes(InputType(xai[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai[1] = (this->swapBytes(InputType(xai[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xai_x[0] = (this->swapBytes(InputType(xai_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai_x[1] = (this->swapBytes(InputType(xai_x[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xai_y[0] = (this->swapBytes(InputType(xai_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai_y[1] = (this->swapBytes(InputType(xai_y[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    this->blend2<SCALE_EMULT>(xai,
                                              xai_x, xai_y,
                                              fc.kx[x], ky,
                                              xaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xaib[0], &xo_, &yo_, &zo_);
                    fc.colorConvert.applyAlpha(xaib[1], &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato3A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            qint64 xai[2];
            qint64 xai_x[2];
            qint64 xai_y[2];
            qint64 xaib[2];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                    xai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xai[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xai_x[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xai_y[1] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xai[0] = (this->swapBytes(InputType(xai[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai[1] = (this->swapBytes(InputType(xai[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xai_x[0] = (this->swapBytes(InputType(xai_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai_x[1] = (this->swapBytes(InputType(xai_x[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xai_y[0] = (this->swapBytes(InputType(xai_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai_y[1] = (this->swapBytes(InputType(xai_y[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    this->blend2<SCALE_EMULT>(xai,
                                              xai_x, xai_y,
                                              fc.kx[x], ky,
                                              xaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    fc.colorConvert.applyPoint(xaib[0], &xo_, &yo_, &zo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *yo = (*yo & OutputType(fc.maskYo)) | (OutputType(yo_) << fc.yoShift);
                    *zo = (*zo & OutputType(fc.maskZo)) | (OutputType(zo_) << fc.zoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(xaib[1]) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertUL1to1(const FrameConvertParameters &fc,
                           const AkVideoPacket &src,
                           AkVideoPacket &dst) const
        {
            qint64 xib = 0;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto xi_x = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    auto xi_y = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xi_x = (this->swapBytes(InputType(xi_x), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xi_y = (this->swapBytes(InputType(xi_y), fc.fromEndian) >> fc.xiShift) & fc.maxXi;

                    this->blend<SCALE_EMULT>(xi,
                                             xi_x, xi_y,
                                             fc.kx[x], ky,
                                             &xib);

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xib, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1to1A(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            qint64 xib = 0;

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto xi_x = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    auto xi_y = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);

                    xi = (this->swapBytes(InputType(xi), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xi_x = (this->swapBytes(InputType(xi_x), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xi_y = (this->swapBytes(InputType(xi_y), fc.fromEndian) >> fc.xiShift) & fc.maxXi;

                    this->blend<SCALE_EMULT>(xi,
                                             xi_x, xi_y,
                                             fc.kx[x], ky,
                                             &xib);

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xib, &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = *ao | OutputType(fc.alphaMask);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato1(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst) const
        {
            qint64 xai[2];
            qint64 xai_x[2];
            qint64 xai_y[2];
            qint64 xaib[2];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                    xai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xai[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xai_x[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xai_y[1] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xai[0] = (this->swapBytes(InputType(xai[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai[1] = (this->swapBytes(InputType(xai[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xai_x[0] = (this->swapBytes(InputType(xai_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai_x[1] = (this->swapBytes(InputType(xai_x[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xai_y[0] = (this->swapBytes(InputType(xai_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai_y[1] = (this->swapBytes(InputType(xai_y[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    this->blend2<SCALE_EMULT>(xai,
                                              xai_x, xai_y,
                                              fc.kx[x], ky,
                                              xaib);

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xaib[0], &xo_);
                    fc.colorConvert.applyAlpha(xaib[1], &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato1A(const FrameConvertParameters &fc,
                             const AkVideoPacket &src,
                             AkVideoPacket &dst) const
        {
            qint64 xai[2];
            qint64 xai_x[2];
            qint64 xai_y[2];
            qint64 xaib[2];

            for (int y = 0; y < fc.outputHeight; ++y) {
                auto &ys = fc.srcHeight[y];
                auto &ys_1 = fc.srcHeight_1[y];

                auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                auto &ky = fc.ky[y];

                for (int x = 0; x < fc.outputWidth; ++x) {
                    int &xs_x = fc.srcWidthOffsetX[x];
                    int &xs_a = fc.srcWidthOffsetA[x];

                    int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                    int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                    xai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xai[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xai_x[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xai_y[1] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xai[0] = (this->swapBytes(InputType(xai[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai[1] = (this->swapBytes(InputType(xai[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xai_x[0] = (this->swapBytes(InputType(xai_x[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai_x[1] = (this->swapBytes(InputType(xai_x[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;
                    xai_y[0] = (this->swapBytes(InputType(xai_y[0]), fc.fromEndian) >> fc.xiShift) & fc.maxXi;
                    xai_y[1] = (this->swapBytes(InputType(xai_y[1]), fc.fromEndian) >> fc.aiShift) & fc.maxAi;

                    this->blend2<SCALE_EMULT>(xai,
                                              xai_x, xai_y,
                                              fc.kx[x], ky,
                                              xaib);

                    qint64 xo_ = 0;
                    fc.colorConvert.applyPoint(xaib[0], &xo_);

                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & OutputType(fc.maskXo)) | (OutputType(xo_) << fc.xoShift);
                    *ao = (*ao & OutputType(fc.maskAo)) | (OutputType(xaib[1]) << fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

#define CONVERT_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertFormat##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                const AkVideoPacket &src, \
                                                                AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convert##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convert##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convert##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_O: \
                this->convert##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERTV_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertVFormat##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                 const AkVideoPacket &src, \
                                                                 AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convertV##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convertV##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convertV##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_O: \
                this->convertV##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERTDL_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertDLFormat##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                  const AkVideoPacket &src, \
                                                                  AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case AlphaMode_AI_AO: \
            case AlphaMode_AI_O: \
                this->integralImage##icomponents##A<InputType>(fc, src); \
                break; \
            default: \
                this->integralImage##icomponents<InputType>(fc, src); \
                break; \
            } \
            \
            switch (fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convertDL##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convertDL##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convertDL##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_O: \
                this->convertDL##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERTDLV_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertDLVFormat##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                   const AkVideoPacket &src, \
                                                                   AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case AlphaMode_AI_AO: \
            case AlphaMode_AI_O: \
                this->integralImage##icomponents##A<InputType>(fc, src); \
                break; \
            default: \
                this->integralImage##icomponents<InputType>(fc, src); \
                break; \
            } \
            \
            switch (fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convertDLV##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convertDLV##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convertDLV##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_O: \
                this->convertDLV##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERTUL_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertULFormat##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                  const AkVideoPacket &src, \
                                                                  AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convertUL##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convertUL##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convertUL##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_O: \
                this->convertUL##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

#define CONVERTULV_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertULVFormat##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                   const AkVideoPacket &src, \
                                                                   AkVideoPacket &dst) const \
        { \
            switch (fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convertULV##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convertULV##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convertULV##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                break; \
            case AlphaMode_I_O: \
                this->convertULV##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                break; \
            }; \
        }

        CONVERT_FUNC(3, 3)
        CONVERT_FUNC(3, 1)
        CONVERT_FUNC(1, 3)
        CONVERT_FUNC(1, 1)
        CONVERTV_FUNC(3, 3)
        CONVERTDL_FUNC(3, 3)
        CONVERTDL_FUNC(3, 1)
        CONVERTDL_FUNC(1, 3)
        CONVERTDL_FUNC(1, 1)
        CONVERTDLV_FUNC(3, 3)
        CONVERTUL_FUNC(3, 3)
        CONVERTUL_FUNC(3, 1)
        CONVERTUL_FUNC(1, 3)
        CONVERTUL_FUNC(1, 1)
        CONVERTULV_FUNC(3, 3)

        template <typename InputType, typename OutputType>
        inline void convert(const FrameConvertParameters &fc,
                            const AkVideoPacket &src,
                            AkVideoPacket &dst)
        {
            if (this->m_scalingMode == AkVideoConverter::ScalingMode_Linear
                && fc.resizeMode == ResizeMode_Up) {
                switch (fc.convertType) {
                case ConvertType_Vector:
                    this->convertULVFormat3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertULFormat3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertULFormat3to1<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertULFormat1to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertULFormat1to1<InputType, OutputType>(fc, src, dst);
                    break;
                }
            } else if (this->m_scalingMode == AkVideoConverter::ScalingMode_Linear
                       && fc.resizeMode == ResizeMode_Down) {
                switch (fc.convertType) {
                case ConvertType_Vector:
                    this->convertDLVFormat3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertDLFormat3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertDLFormat3to1<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertDLFormat1to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertDLFormat1to1<InputType, OutputType>(fc, src, dst);
                    break;
                }
            } else {
                switch (fc.convertType) {
                case ConvertType_Vector:
                    this->convertVFormat3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertFormat3to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertFormat3to1<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertFormat1to3<InputType, OutputType>(fc, src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertFormat1to1<InputType, OutputType>(fc, src, dst);
                    break;
                }
            }
        }

    inline AkVideoPacket convert(const AkVideoPacket &packet,
                                 const AkVideoCaps &ocaps);
};

AkVideoConverter::AkVideoConverter(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoConverterPrivate();
}

AkVideoConverter::AkVideoConverter(const AkVideoCaps &outputCaps,
                                   QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoConverterPrivate();
    this->d->m_outputCaps = outputCaps;
}

AkVideoConverter::AkVideoConverter(const AkVideoConverter &other):
    QObject()
{
    this->d = new AkVideoConverterPrivate();
    this->d->m_outputCaps = other.d->m_outputCaps;
    this->d->m_yuvColorSpace = other.d->m_yuvColorSpace;
    this->d->m_yuvColorSpaceType = other.d->m_yuvColorSpaceType;
    this->d->m_scalingMode = other.d->m_scalingMode;
    this->d->m_aspectRatioMode = other.d->m_aspectRatioMode;
    this->d->m_inputRect = other.d->m_inputRect;
}

AkVideoConverter::~AkVideoConverter()
{
    if (this->d->m_fc) {
        delete [] this->d->m_fc;
        this->d->m_fc = nullptr;
    }

    delete this->d;
}

AkVideoConverter &AkVideoConverter::operator =(const AkVideoConverter &other)
{
    if (this != &other) {
        this->d->m_yuvColorSpace = other.d->m_yuvColorSpace;
        this->d->m_yuvColorSpaceType = other.d->m_yuvColorSpaceType;
        this->d->m_outputCaps = other.d->m_outputCaps;
        this->d->m_scalingMode = other.d->m_scalingMode;
        this->d->m_aspectRatioMode = other.d->m_aspectRatioMode;
        this->d->m_inputRect = other.d->m_inputRect;
    }

    return *this;
}

QObject *AkVideoConverter::create()
{
    return new AkVideoConverter();
}

AkVideoCaps AkVideoConverter::outputCaps() const
{
    return this->d->m_outputCaps;
}

AkColorConvert::YuvColorSpace AkVideoConverter::yuvColorSpace() const
{
    return this->d->m_yuvColorSpace;
}

AkColorConvert::YuvColorSpaceType AkVideoConverter::yuvColorSpaceType() const
{
    return this->d->m_yuvColorSpaceType;
}

AkVideoConverter::ScalingMode AkVideoConverter::scalingMode() const
{
    return this->d->m_scalingMode;
}

AkVideoConverter::AspectRatioMode AkVideoConverter::aspectRatioMode() const
{
    return this->d->m_aspectRatioMode;
}

QRect AkVideoConverter::inputRect() const
{
    return this->d->m_inputRect;
}

bool AkVideoConverter::begin()
{
    this->d->m_cacheIndex = 0;

    return true;
}

void AkVideoConverter::end()
{
    this->d->m_cacheIndex = 0;
}

AkVideoPacket AkVideoConverter::convert(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    auto &caps = packet.caps();

    if (caps.format() == this->d->m_outputCaps.format()
        && caps.width() == this->d->m_outputCaps.width()
        && caps.height() == this->d->m_outputCaps.height()
        && this->d->m_inputRect.isEmpty())
        return packet;

    return this->d->convert(packet, this->d->m_outputCaps);
}

void AkVideoConverter::setCacheIndex(int index)
{
    this->d->m_cacheIndex = index;
}

void AkVideoConverter::setOutputCaps(const AkVideoCaps &outputCaps)
{
    if (this->d->m_outputCaps == outputCaps)
        return;

    this->d->m_mutex.lock();
    this->d->m_outputCaps = outputCaps;
    this->d->m_mutex.unlock();
    emit this->outputCapsChanged(outputCaps);
}

void AkVideoConverter::setYuvColorSpace(AkColorConvert::YuvColorSpace yuvColorSpace)
{
    if (this->d->m_yuvColorSpace == yuvColorSpace)
        return;

    this->d->m_yuvColorSpace = yuvColorSpace;
    emit this->yuvColorSpaceChanged(yuvColorSpace);
}

void AkVideoConverter::setYuvColorSpaceType(AkColorConvert::YuvColorSpaceType yuvColorSpaceType)
{
    if (this->d->m_yuvColorSpaceType == yuvColorSpaceType)
        return;

    this->d->m_yuvColorSpaceType = yuvColorSpaceType;
    emit this->yuvColorSpaceTypeChanged(yuvColorSpaceType);
}

void AkVideoConverter::setScalingMode(AkVideoConverter::ScalingMode scalingMode)
{
    if (this->d->m_scalingMode == scalingMode)
        return;

    this->d->m_scalingMode = scalingMode;
    emit this->scalingModeChanged(scalingMode);
}

void AkVideoConverter::setAspectRatioMode(AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    if (this->d->m_aspectRatioMode == aspectRatioMode)
        return;

    this->d->m_aspectRatioMode = aspectRatioMode;
    emit this->aspectRatioModeChanged(aspectRatioMode);
}

void AkVideoConverter::setInputRect(const QRect &inputRect)
{
    if (this->d->m_inputRect == inputRect)
        return;

    this->d->m_inputRect = inputRect;
    emit this->inputRectChanged(inputRect);
}

void AkVideoConverter::resetOutputCaps()
{
    this->setOutputCaps({});
}

void AkVideoConverter::resetYuvColorSpace()
{
    this->setYuvColorSpace(AkColorConvert::YuvColorSpace_ITUR_BT601);
}

void AkVideoConverter::resetYuvColorSpaceType()
{
    this->setYuvColorSpaceType(AkColorConvert::YuvColorSpaceType_StudioSwing);
}

void AkVideoConverter::resetScalingMode()
{
    this->setScalingMode(ScalingMode_Fast);
}

void AkVideoConverter::resetAspectRatioMode()
{
    this->setAspectRatioMode(AspectRatioMode_Ignore);
}

void AkVideoConverter::resetInputRect()
{
    this->setInputRect({});
}

void AkVideoConverter::reset()
{
    if (this->d->m_fc) {
        delete [] this->d->m_fc;
        this->d->m_fc = nullptr;
    }

    this->d->m_fcSize = 0;
}

void AkVideoConverter::registerTypes()
{
    qRegisterMetaType<AkVideoConverter>("AkVideoConverter");
    qRegisterMetaType<ScalingMode>("ScalingMode");
    QMetaType::registerDebugStreamOperator<AkVideoConverter::ScalingMode>();
    qRegisterMetaType<AspectRatioMode>("AspectRatioMode");
    QMetaType::registerDebugStreamOperator<AkVideoConverter::AspectRatioMode>();
    qmlRegisterSingletonType<AkVideoConverter>("Ak", 1, 0, "AkVideoConverter",
                                               [] (QQmlEngine *qmlEngine,
                                                   QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkVideoConverter();
    });
}

QDebug operator <<(QDebug debug, AkVideoConverter::ScalingMode mode)
{
    AkVideoConverter converter;
    int scalingModeIndex = converter.metaObject()->indexOfEnumerator("ScalingMode");
    QMetaEnum scalingModeEnum = converter.metaObject()->enumerator(scalingModeIndex);
    QString scalingModeStr(scalingModeEnum.valueToKey(mode));
    scalingModeStr.remove("ScalingMode_");
    QDebugStateSaver saver(debug);
    debug.nospace() << scalingModeStr.toStdString().c_str();

    return debug;
}

QDebug operator <<(QDebug debug, AkVideoConverter::AspectRatioMode mode)
{
    AkVideoConverter converter;
    int aspectRatioModeIndex = converter.metaObject()->indexOfEnumerator("AspectRatioMode");
    QMetaEnum aspectRatioModeEnum = converter.metaObject()->enumerator(aspectRatioModeIndex);
    QString aspectRatioModeStr(aspectRatioModeEnum.valueToKey(mode));
    aspectRatioModeStr.remove("AspectRatioMode_");
    QDebugStateSaver saver(debug);
    debug.nospace() << aspectRatioModeStr.toStdString().c_str();

    return debug;
}

#define DEFINE_CONVERT_FUNC(isize, osize) \
    case ConvertDataTypes_##isize##_##osize: \
        this->convert<quint##isize, quint##osize>(fc, \
                                                  packet, \
                                                  fc.outputFrame); \
        break;

AkVideoPacket AkVideoConverterPrivate::convert(const AkVideoPacket &packet,
                                               const AkVideoCaps &ocaps)
{
    if (this->m_cacheIndex >= this->m_fcSize) {
        static const int cacheBlockSize = 8;
        auto newSize = (this->m_cacheIndex + cacheBlockSize) & ~(cacheBlockSize - 1);
        auto fc = new FrameConvertParameters[newSize];

        if (this->m_fc) {
            for (int i = 0; i < this->m_fcSize; ++i)
                fc[i] = this->m_fc[i];

            delete [] this->m_fc;
        }

        this->m_fc = fc;
        this->m_fcSize = newSize;
    }

    auto &fc = this->m_fc[this->m_cacheIndex];

    if (packet.caps() != fc.inputCaps
        || ocaps != fc.outputCaps
        || this->m_yuvColorSpace != fc.yuvColorSpace
        || this->m_yuvColorSpaceType != fc.yuvColorSpaceType
        || this->m_scalingMode != fc.scalingMode
        || this->m_aspectRatioMode != fc.aspectRatioMode
        || this->m_inputRect != fc.inputRect) {
        fc.configure(packet.caps(),
                     ocaps,
                     fc.colorConvert,
                     this->m_yuvColorSpace,
                     this->m_yuvColorSpaceType);
        fc.configureScaling(packet.caps(),
                            ocaps,
                            this->m_inputRect,
                            this->m_aspectRatioMode);
        fc.inputCaps = packet.caps();
        fc.outputCaps = ocaps;
        fc.yuvColorSpace = this->m_yuvColorSpace;
        fc.yuvColorSpaceType = this->m_yuvColorSpaceType;
        fc.scalingMode = this->m_scalingMode;
        fc.aspectRatioMode = this->m_aspectRatioMode;
        fc.inputRect = this->m_inputRect;
    }

    if (fc.outputConvertCaps.isSameFormat(packet.caps())) {
        this->m_cacheIndex++;

        return packet;
    }

    switch (fc.convertDataTypes) {
    DEFINE_CONVERT_FUNC(8 , 8 )
    DEFINE_CONVERT_FUNC(8 , 16)
    DEFINE_CONVERT_FUNC(8 , 32)
    DEFINE_CONVERT_FUNC(16, 8 )
    DEFINE_CONVERT_FUNC(16, 16)
    DEFINE_CONVERT_FUNC(16, 32)
    DEFINE_CONVERT_FUNC(32, 8 )
    DEFINE_CONVERT_FUNC(32, 16)
    DEFINE_CONVERT_FUNC(32, 32)
    }

    fc.outputFrame.copyMetadata(packet);
    this->m_cacheIndex++;

    return fc.outputFrame;
}

FrameConvertParameters::FrameConvertParameters()
{
}

FrameConvertParameters::FrameConvertParameters(const FrameConvertParameters &other):
    inputCaps(other.inputCaps),
    outputCaps(other.outputCaps),
    outputConvertCaps(other.outputConvertCaps),
    outputFrame(other.outputFrame),
    scalingMode(other.scalingMode),
    aspectRatioMode(other.aspectRatioMode),
    convertType(other.convertType),
    convertDataTypes(other.convertDataTypes),
    alphaMode(other.alphaMode),
    resizeMode(other.resizeMode),
    fromEndian(other.fromEndian),
    toEndian(other.toEndian),
    inputWidth(other.inputWidth),
    inputWidth_1(other.inputWidth_1),
    inputHeight(other.inputHeight),
    outputWidth(other.outputWidth),
    outputHeight(other.outputHeight),
    planeXi(other.planeXi),
    planeYi(other.planeYi),
    planeZi(other.planeZi),
    planeAi(other.planeAi),
    compXi(other.compXi),
    compYi(other.compYi),
    compZi(other.compZi),
    compAi(other.compAi),
    planeXo(other.planeXo),
    planeYo(other.planeYo),
    planeZo(other.planeZo),
    planeAo(other.planeAo),
    compXo(other.compXo),
    compYo(other.compYo),
    compZo(other.compZo),
    compAo(other.compAo),
    xiOffset(other.xiOffset),
    yiOffset(other.yiOffset),
    ziOffset(other.ziOffset),
    aiOffset(other.aiOffset),
    xoOffset(other.xoOffset),
    yoOffset(other.yoOffset),
    zoOffset(other.zoOffset),
    aoOffset(other.aoOffset),
    xiShift(other.xiShift),
    yiShift(other.yiShift),
    ziShift(other.ziShift),
    aiShift(other.aiShift),
    xoShift(other.xoShift),
    yoShift(other.yoShift),
    zoShift(other.zoShift),
    aoShift(other.aoShift),
    maxXi(other.maxXi),
    maxYi(other.maxYi),
    maxZi(other.maxZi),
    maxAi(other.maxAi),
    maskXo(other.maskXo),
    maskYo(other.maskYo),
    maskZo(other.maskZo),
    maskAo(other.maskAo),
    alphaMask(other.alphaMask)
{
    auto oWidth = this->outputCaps.width();
    auto oHeight = this->outputCaps.height();

    size_t oWidthDataSize = sizeof(int) * oWidth;
    size_t oHeightDataSize = sizeof(int) * oHeight;

    if (other.srcWidth) {
        this->srcWidth = new int [oWidth];
        memcpy(this->srcWidth, other.srcWidth, oWidthDataSize);
    }

    if (other.srcWidth_1) {
        this->srcWidth_1 = new int [oWidth];
        memcpy(this->srcWidth_1, other.srcWidth_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetX) {
        this->srcWidthOffsetX = new int [oWidth];
        memcpy(this->srcWidthOffsetX, other.srcWidthOffsetX, oWidthDataSize);
    }

    if (other.srcWidthOffsetY) {
        this->srcWidthOffsetY = new int [oWidth];
        memcpy(this->srcWidthOffsetY, other.srcWidthOffsetY, oWidthDataSize);
    }

    if (other.srcWidthOffsetZ) {
        this->srcWidthOffsetZ = new int [oWidth];
        memcpy(this->srcWidthOffsetZ, other.srcWidthOffsetZ, oWidthDataSize);
    }

    if (other.srcWidthOffsetA) {
        this->srcWidthOffsetA = new int [oWidth];
        memcpy(this->srcWidthOffsetA, other.srcWidthOffsetA, oWidthDataSize);
    }

    if (other.srcHeight) {
        this->srcHeight = new int [oHeight];
        memcpy(this->srcHeight, other.srcHeight, oHeightDataSize);
    }

    auto iWidth = this->inputCaps.width();
    size_t iWidthDataSize = sizeof(int) * iWidth;

    if (other.dlSrcWidthOffsetX) {
        this->dlSrcWidthOffsetX = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetX, other.dlSrcWidthOffsetX, iWidthDataSize);
    }

    if (other.dlSrcWidthOffsetY) {
        this->dlSrcWidthOffsetY = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetY, other.dlSrcWidthOffsetY, iWidthDataSize);
    }

    if (other.dlSrcWidthOffsetZ) {
        this->dlSrcWidthOffsetZ = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetZ, other.dlSrcWidthOffsetZ, iWidthDataSize);
    }

    if (other.dlSrcWidthOffsetA) {
        this->dlSrcWidthOffsetA = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetA, other.dlSrcWidthOffsetA, iWidthDataSize);
    }

    if (other.srcWidthOffsetX_1) {
        this->srcWidthOffsetX_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetX_1, other.srcWidthOffsetX_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetY_1) {
        this->srcWidthOffsetY_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetY_1, other.srcWidthOffsetY_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetZ_1) {
        this->srcWidthOffsetZ_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetZ_1, other.srcWidthOffsetZ_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetA_1) {
        this->srcWidthOffsetA_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetA_1, other.srcWidthOffsetA_1, oWidthDataSize);
    }

    if (other.srcHeight_1) {
        this->srcHeight_1 = new int [oHeight];
        memcpy(this->srcHeight_1, other.srcHeight_1, oHeightDataSize);
    }

    if (other.dstWidthOffsetX) {
        this->dstWidthOffsetX = new int [oWidth];
        memcpy(this->dstWidthOffsetX, other.dstWidthOffsetX, oWidthDataSize);
    }

    if (other.dstWidthOffsetY) {
        this->dstWidthOffsetY = new int [oWidth];
        memcpy(this->dstWidthOffsetY, other.dstWidthOffsetY, oWidthDataSize);
    }

    if (other.dstWidthOffsetZ) {
        this->dstWidthOffsetZ = new int [oWidth];
        memcpy(this->dstWidthOffsetZ, other.dstWidthOffsetZ, oWidthDataSize);
    }

    if (other.dstWidthOffsetA) {
        this->dstWidthOffsetA = new int [oWidth];
        memcpy(this->dstWidthOffsetA, other.dstWidthOffsetA, oWidthDataSize);
    }

    size_t oHeightDlDataSize = sizeof(size_t) * oHeight;

    if (other.srcHeightDlOffset) {
        this->srcHeightDlOffset = new size_t [oHeight];
        memcpy(this->srcHeightDlOffset, other.srcHeightDlOffset, oHeightDlDataSize);
    }

    if (other.srcHeightDlOffset_1) {
        this->srcHeightDlOffset_1 = new size_t [oHeight];
        memcpy(this->srcHeightDlOffset_1, other.srcHeightDlOffset_1, oHeightDlDataSize);
    }

    size_t iWidth_1 = this->inputCaps.width() + 1;
    size_t iHeight_1 = this->inputCaps.height() + 1;
    size_t integralImageSize = iWidth_1 * iHeight_1;
    size_t integralImageDataSize = sizeof(DlSumType) * integralImageSize;

    if (other.integralImageDataX) {
        this->integralImageDataX = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataX, other.integralImageDataX, integralImageDataSize);
    }

    if (other.integralImageDataY) {
        this->integralImageDataY = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataY, other.integralImageDataY, integralImageDataSize);
    }

    if (other.integralImageDataZ) {
        this->integralImageDataZ = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataZ, other.integralImageDataZ, integralImageDataSize);
    }

    if (other.integralImageDataA) {
        this->integralImageDataA = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataA, other.integralImageDataA, integralImageDataSize);
    }

    if (other.kx) {
        this->kx = new qint64 [oWidth];
        memcpy(this->kx, other.kx, sizeof(qint64) * oWidth);
    }

    if (other.ky) {
        this->ky = new qint64 [oHeight];
        memcpy(this->ky, other.ky, sizeof(qint64) * oHeight);
    }

    auto kdlSize = size_t(this->inputCaps.width()) * this->inputCaps.height();
    auto kdlDataSize = sizeof(DlSumType) * kdlSize;

    if (other.kdl) {
        this->kdl = new DlSumType [kdlSize];
        memcpy(this->kdl, other.kdl, kdlDataSize);
    }
}

FrameConvertParameters::~FrameConvertParameters()
{
    this->clearBuffers();
    this->clearDlBuffers();
}

FrameConvertParameters &FrameConvertParameters::operator =(const FrameConvertParameters &other)
{
    if (this != &other) {
        this->inputCaps = other.inputCaps;
        this->outputCaps = other.outputCaps;
        this->outputConvertCaps = other.outputConvertCaps;
        this->outputFrame = other.outputFrame;
        this->scalingMode = other.scalingMode;
        this->aspectRatioMode = other.aspectRatioMode;
        this->convertType = other.convertType;
        this->convertDataTypes = other.convertDataTypes;
        this->alphaMode = other.alphaMode;
        this->resizeMode = other.resizeMode;
        this->fromEndian = other.fromEndian;
        this->toEndian = other.toEndian;
        this->inputWidth = other.inputWidth;
        this->inputWidth_1 = other.inputWidth_1;
        this->inputHeight = other.inputHeight;
        this->outputWidth = other.outputWidth;
        this->outputHeight = other.outputHeight;
        this->planeXi = other.planeXi;
        this->planeYi = other.planeYi;
        this->planeZi = other.planeZi;
        this->planeAi = other.planeAi;
        this->compXi = other.compXi;
        this->compYi = other.compYi;
        this->compZi = other.compZi;
        this->compAi = other.compAi;
        this->planeXo = other.planeXo;
        this->planeYo = other.planeYo;
        this->planeZo = other.planeZo;
        this->planeAo = other.planeAo;
        this->compXo = other.compXo;
        this->compYo = other.compYo;
        this->compZo = other.compZo;
        this->compAo = other.compAo;
        this->xiOffset = other.xiOffset;
        this->yiOffset = other.yiOffset;
        this->ziOffset = other.ziOffset;
        this->aiOffset = other.aiOffset;
        this->xoOffset = other.xoOffset;
        this->yoOffset = other.yoOffset;
        this->zoOffset = other.zoOffset;
        this->aoOffset = other.aoOffset;
        this->xiShift = other.xiShift;
        this->yiShift = other.yiShift;
        this->ziShift = other.ziShift;
        this->aiShift = other.aiShift;
        this->xoShift = other.xoShift;
        this->yoShift = other.yoShift;
        this->zoShift = other.zoShift;
        this->aoShift = other.aoShift;
        this->maxXi = other.maxXi;
        this->maxYi = other.maxYi;
        this->maxZi = other.maxZi;
        this->maxAi = other.maxAi;
        this->maskXo = other.maskXo;
        this->maskYo = other.maskYo;
        this->maskZo = other.maskZo;
        this->maskAo = other.maskAo;
        this->alphaMask = other.alphaMask;

        this->clearBuffers();
        this->clearDlBuffers();

        auto oWidth = this->outputCaps.width();
        auto oHeight = this->outputCaps.height();

        size_t oWidthDataSize = sizeof(int) * oWidth;
        size_t oHeightDataSize = sizeof(int) * oHeight;

        if (other.srcWidth) {
            this->srcWidth = new int [oWidth];
            memcpy(this->srcWidth, other.srcWidth, oWidthDataSize);
        }

        if (other.srcWidth_1) {
            this->srcWidth_1 = new int [oWidth];
            memcpy(this->srcWidth_1, other.srcWidth_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetX) {
            this->srcWidthOffsetX = new int [oWidth];
            memcpy(this->srcWidthOffsetX, other.srcWidthOffsetX, oWidthDataSize);
        }

        if (other.srcWidthOffsetY) {
            this->srcWidthOffsetY = new int [oWidth];
            memcpy(this->srcWidthOffsetY, other.srcWidthOffsetY, oWidthDataSize);
        }

        if (other.srcWidthOffsetZ) {
            this->srcWidthOffsetZ = new int [oWidth];
            memcpy(this->srcWidthOffsetZ, other.srcWidthOffsetZ, oWidthDataSize);
        }

        if (other.srcWidthOffsetA) {
            this->srcWidthOffsetA = new int [oWidth];
            memcpy(this->srcWidthOffsetA, other.srcWidthOffsetA, oWidthDataSize);
        }

        if (other.srcHeight) {
            this->srcHeight = new int [oHeight];
            memcpy(this->srcHeight, other.srcHeight, oHeightDataSize);
        }

        auto iWidth = this->inputCaps.width();
        size_t iWidthDataSize = sizeof(int) * iWidth;

        if (other.dlSrcWidthOffsetX) {
            this->dlSrcWidthOffsetX = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetX, other.dlSrcWidthOffsetX, iWidthDataSize);
        }

        if (other.dlSrcWidthOffsetY) {
            this->dlSrcWidthOffsetY = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetY, other.dlSrcWidthOffsetY, iWidthDataSize);
        }

        if (other.dlSrcWidthOffsetZ) {
            this->dlSrcWidthOffsetZ = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetZ, other.dlSrcWidthOffsetZ, iWidthDataSize);
        }

        if (other.dlSrcWidthOffsetA) {
            this->dlSrcWidthOffsetA = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetA, other.dlSrcWidthOffsetA, iWidthDataSize);
        }

        if (other.srcWidthOffsetX_1) {
            this->srcWidthOffsetX_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetX_1, other.srcWidthOffsetX_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetY_1) {
            this->srcWidthOffsetY_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetY_1, other.srcWidthOffsetY_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetZ_1) {
            this->srcWidthOffsetZ_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetZ_1, other.srcWidthOffsetZ_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetA_1) {
            this->srcWidthOffsetA_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetA_1, other.srcWidthOffsetA_1, oWidthDataSize);
        }

        if (other.srcHeight_1) {
            this->srcHeight_1 = new int [oHeight];
            memcpy(this->srcHeight_1, other.srcHeight_1, oHeightDataSize);
        }

        if (other.dstWidthOffsetX) {
            this->dstWidthOffsetX = new int [oWidth];
            memcpy(this->dstWidthOffsetX, other.dstWidthOffsetX, oWidthDataSize);
        }

        if (other.dstWidthOffsetY) {
            this->dstWidthOffsetY = new int [oWidth];
            memcpy(this->dstWidthOffsetY, other.dstWidthOffsetY, oWidthDataSize);
        }

        if (other.dstWidthOffsetZ) {
            this->dstWidthOffsetZ = new int [oWidth];
            memcpy(this->dstWidthOffsetZ, other.dstWidthOffsetZ, oWidthDataSize);
        }

        if (other.dstWidthOffsetA) {
            this->dstWidthOffsetA = new int [oWidth];
            memcpy(this->dstWidthOffsetA, other.dstWidthOffsetA, oWidthDataSize);
        }

        size_t oHeightDlDataSize = sizeof(size_t) * oHeight;

        if (other.srcHeightDlOffset) {
            this->srcHeightDlOffset = new size_t [oHeight];
            memcpy(this->srcHeightDlOffset, other.srcHeightDlOffset, oHeightDlDataSize);
        }

        if (other.srcHeightDlOffset_1) {
            this->srcHeightDlOffset_1 = new size_t [oHeight];
            memcpy(this->srcHeightDlOffset_1, other.srcHeightDlOffset_1, oHeightDlDataSize);
        }

        size_t iWidth_1 = this->inputCaps.width() + 1;
        size_t iHeight_1 = this->inputCaps.height() + 1;
        size_t integralImageSize = iWidth_1 * iHeight_1;
        size_t integralImageDataSize = sizeof(DlSumType) * integralImageSize;

        if (other.integralImageDataX) {
            this->integralImageDataX = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataX, other.integralImageDataX, integralImageDataSize);
        }

        if (other.integralImageDataY) {
            this->integralImageDataY = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataY, other.integralImageDataY, integralImageDataSize);
        }

        if (other.integralImageDataZ) {
            this->integralImageDataZ = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataZ, other.integralImageDataZ, integralImageDataSize);
        }

        if (other.integralImageDataA) {
            this->integralImageDataA = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataA, other.integralImageDataA, integralImageDataSize);
        }

        if (other.kx) {
            this->kx = new qint64 [oWidth];
            memcpy(this->kx, other.kx, sizeof(qint64) * oWidth);
        }

        if (other.ky) {
            this->ky = new qint64 [oHeight];
            memcpy(this->ky, other.ky, sizeof(qint64) * oHeight);
        }

        auto kdlSize = size_t(this->inputCaps.width()) * this->inputCaps.height();
        auto kdlDataSize = sizeof(DlSumType) * kdlSize;

        if (other.kdl) {
            this->kdl = new DlSumType [kdlSize];
            memcpy(this->kdl, other.kdl, kdlDataSize);
        }
    }

    return *this;
}

void FrameConvertParameters::clearBuffers()
{
    if (this->srcWidth) {
        delete [] this->srcWidth;
        this->srcWidth = nullptr;
    }

    if (this->srcWidth_1) {
        delete [] this->srcWidth_1;
        this->srcWidth_1 = nullptr;
    }

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

    if (this->srcWidthOffsetX_1) {
        delete [] this->srcWidthOffsetX_1;
        this->srcWidthOffsetX_1 = nullptr;
    }

    if (this->srcWidthOffsetY_1) {
        delete [] this->srcWidthOffsetY_1;
        this->srcWidthOffsetY_1 = nullptr;
    }

    if (this->srcWidthOffsetZ_1) {
        delete [] this->srcWidthOffsetZ_1;
        this->srcWidthOffsetZ_1 = nullptr;
    }

    if (this->srcWidthOffsetA_1) {
        delete [] this->srcWidthOffsetA_1;
        this->srcWidthOffsetA_1 = nullptr;
    }

    if (this->srcHeight_1) {
        delete [] this->srcHeight_1;
        this->srcHeight_1 = nullptr;
    }

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

    if (this->kx) {
        delete [] this->kx;
        this->kx = nullptr;
    }

    if (this->ky) {
        delete [] this->ky;
        this->ky = nullptr;
    }
}

void FrameConvertParameters::clearDlBuffers()
{
    if (this->integralImageDataX) {
        delete [] this->integralImageDataX;
        this->integralImageDataX = nullptr;
    }

    if (this->integralImageDataY) {
        delete [] this->integralImageDataY;
        this->integralImageDataY = nullptr;
    }

    if (this->integralImageDataZ) {
        delete [] this->integralImageDataZ;
        this->integralImageDataZ = nullptr;
    }

    if (this->integralImageDataA) {
        delete [] this->integralImageDataA;
        this->integralImageDataA = nullptr;
    }

    if (this->kdl) {
        delete [] this->kdl;
        this->kdl = nullptr;
    }

    if (this->srcHeightDlOffset) {
        delete [] this->srcHeightDlOffset;
        this->srcHeightDlOffset = nullptr;
    }

    if (this->srcHeightDlOffset_1) {
        delete [] this->srcHeightDlOffset_1;
        this->srcHeightDlOffset_1 = nullptr;
    }

    if (this->dlSrcWidthOffsetX) {
        delete [] this->dlSrcWidthOffsetX;
        this->dlSrcWidthOffsetX = nullptr;
    }

    if (this->dlSrcWidthOffsetY) {
        delete [] this->dlSrcWidthOffsetY;
        this->dlSrcWidthOffsetY = nullptr;
    }

    if (this->dlSrcWidthOffsetZ) {
        delete [] this->dlSrcWidthOffsetZ;
        this->dlSrcWidthOffsetZ = nullptr;
    }

    if (this->dlSrcWidthOffsetA) {
        delete [] this->dlSrcWidthOffsetA;
        this->dlSrcWidthOffsetA = nullptr;
    }
}

void FrameConvertParameters::allocateBuffers(const AkVideoCaps &ocaps)
{
    this->clearBuffers();

    this->srcWidth = new int [ocaps.width()];
    this->srcWidth_1 = new int [ocaps.width()];
    this->srcWidthOffsetX = new int [ocaps.width()];
    this->srcWidthOffsetY = new int [ocaps.width()];
    this->srcWidthOffsetZ = new int [ocaps.width()];
    this->srcWidthOffsetA = new int [ocaps.width()];
    this->srcHeight = new int [ocaps.height()];

    this->srcWidthOffsetX_1 = new int [ocaps.width()];
    this->srcWidthOffsetY_1 = new int [ocaps.width()];
    this->srcWidthOffsetZ_1 = new int [ocaps.width()];
    this->srcWidthOffsetA_1 = new int [ocaps.width()];
    this->srcHeight_1 = new int [ocaps.height()];

    this->dstWidthOffsetX = new int [ocaps.width()];
    this->dstWidthOffsetY = new int [ocaps.width()];
    this->dstWidthOffsetZ = new int [ocaps.width()];
    this->dstWidthOffsetA = new int [ocaps.width()];

    this->kx = new qint64 [ocaps.width()];
    this->ky = new qint64 [ocaps.height()];
}

void FrameConvertParameters::allocateDlBuffers(const AkVideoCaps &icaps,
                                               const AkVideoCaps &ocaps)
{
    size_t width_1 = icaps.width() + 1;
    size_t height_1 = icaps.height() + 1;
    auto integralImageSize = width_1 * height_1;

    this->integralImageDataX = new DlSumType [integralImageSize];
    this->integralImageDataY = new DlSumType [integralImageSize];
    this->integralImageDataZ = new DlSumType [integralImageSize];
    this->integralImageDataA = new DlSumType [integralImageSize];
    memset(this->integralImageDataX, 0, integralImageSize);
    memset(this->integralImageDataY, 0, integralImageSize);
    memset(this->integralImageDataZ, 0, integralImageSize);
    memset(this->integralImageDataA, 0, integralImageSize);

    auto kdlSize = size_t(icaps.width()) * icaps.height();
    this->kdl = new DlSumType [kdlSize];
    memset(this->kdl, 0, kdlSize);

    this->srcHeightDlOffset = new size_t [ocaps.height()];
    this->srcHeightDlOffset_1 = new size_t [ocaps.height()];

    this->dlSrcWidthOffsetX = new int [icaps.width()];
    this->dlSrcWidthOffsetY = new int [icaps.width()];
    this->dlSrcWidthOffsetZ = new int [icaps.width()];
    this->dlSrcWidthOffsetA = new int [icaps.width()];
}

#define DEFINE_CONVERT_TYPES(isize, osize) \
    if (ispecs.byteLength() == (isize / 8) && ospecs.byteLength() == (osize / 8)) \
        this->convertDataTypes = ConvertDataTypes_##isize##_##osize;

void FrameConvertParameters::configure(const AkVideoCaps &icaps,
                                       const AkVideoCaps &ocaps,
                                       AkColorConvert &colorConvert,
                                       AkColorConvert::YuvColorSpace yuvColorSpace,
                                       AkColorConvert::YuvColorSpaceType yuvColorSpaceType)
{
    auto ispecs = AkVideoCaps::formatSpecs(icaps.format());
    auto ospecs = AkVideoCaps::formatSpecs(ocaps.format());

    DEFINE_CONVERT_TYPES(8, 8);
    DEFINE_CONVERT_TYPES(8, 16);
    DEFINE_CONVERT_TYPES(8, 32);
    DEFINE_CONVERT_TYPES(16, 8);
    DEFINE_CONVERT_TYPES(16, 16);
    DEFINE_CONVERT_TYPES(16, 32);
    DEFINE_CONVERT_TYPES(32, 8);
    DEFINE_CONVERT_TYPES(32, 16);
    DEFINE_CONVERT_TYPES(32, 32);

    auto icomponents = ispecs.mainComponents();
    auto ocomponents = ospecs.mainComponents();

    if (icomponents == 3 && ispecs.type() == ospecs.type())
        this->convertType = ConvertType_Vector;
    else if (icomponents == 3 && ocomponents == 3)
        this->convertType = ConvertType_3to3;
    else if (icomponents == 3 && ocomponents == 1)
        this->convertType = ConvertType_3to1;
    else if (icomponents == 1 && ocomponents == 3)
        this->convertType = ConvertType_1to3;
    else if (icomponents == 1 && ocomponents == 1)
        this->convertType = ConvertType_1to1;

    this->fromEndian = ispecs.endianness();
    this->toEndian = ospecs.endianness();
    colorConvert.setYuvColorSpace(yuvColorSpace);
    colorConvert.setYuvColorSpaceType(yuvColorSpaceType);
    colorConvert.loadMatrix(ispecs, ospecs);

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

    case AkVideoFormatSpec::VFT_Gray:
        this->planeXi = ispecs.componentPlane(AkColorComponent::CT_Y);
        this->compXi = ispecs.component(AkColorComponent::CT_Y);

        break;

    default:
        break;
    }

    this->planeAi = ispecs.componentPlane(AkColorComponent::CT_A);
    this->compAi = ispecs.component(AkColorComponent::CT_A);

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

    this->xiOffset = this->compXi.offset();
    this->yiOffset = this->compYi.offset();
    this->ziOffset = this->compZi.offset();
    this->aiOffset = this->compAi.offset();

    this->xoOffset = this->compXo.offset();
    this->yoOffset = this->compYo.offset();
    this->zoOffset = this->compZo.offset();
    this->aoOffset = this->compAo.offset();

    this->xiShift = this->compXi.shift();
    this->yiShift = this->compYi.shift();
    this->ziShift = this->compZi.shift();
    this->aiShift = this->compAi.shift();

    this->xoShift = this->compXo.shift();
    this->yoShift = this->compYo.shift();
    this->zoShift = this->compZo.shift();
    this->aoShift = this->compAo.shift();

    this->maxXi = this->compXi.max<quint64>();
    this->maxYi = this->compYi.max<quint64>();
    this->maxZi = this->compZi.max<quint64>();
    this->maxAi = this->compAi.max<quint64>();

    this->maskXo = ~(this->compXo.max<quint64>() << this->compXo.shift());
    this->maskYo = ~(this->compYo.max<quint64>() << this->compYo.shift());
    this->maskZo = ~(this->compZo.max<quint64>() << this->compZo.shift());
    this->alphaMask = this->compAo.max<quint64>() << this->compAo.shift();
    this->maskAo = ~this->alphaMask;

    bool hasAlphaIn = ispecs.contains(AkColorComponent::CT_A);
    bool hasAlphaOut = ospecs.contains(AkColorComponent::CT_A);

    if (hasAlphaIn && hasAlphaOut)
        this->alphaMode = AlphaMode_AI_AO;
    else if (hasAlphaIn && !hasAlphaOut)
        this->alphaMode = AlphaMode_AI_O;
    else if (!hasAlphaIn && hasAlphaOut)
        this->alphaMode = AlphaMode_I_AO;
    else if (!hasAlphaIn && !hasAlphaOut)
        this->alphaMode = AlphaMode_I_O;
}

void FrameConvertParameters::configureScaling(const AkVideoCaps &icaps,
                                              const AkVideoCaps &ocaps,
                                              const QRect &inputRect,
                                              AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    QRect irect(0, 0, icaps.width(), icaps.height());

    if (!inputRect.isEmpty())
        irect = irect.intersected(inputRect);

    this->outputConvertCaps = ocaps;

    if (this->outputConvertCaps.format() == AkVideoCaps::Format_none)
        this->outputConvertCaps.setFormat(icaps.format());

    int width = this->outputConvertCaps.width() > 1?
                    this->outputConvertCaps.width():
                    irect.width();
    int height = this->outputConvertCaps.height() > 1?
                     this->outputConvertCaps.height():
                     irect.height();

    if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Keep) {
        auto w = height * irect.width() / irect.height();
        auto h = width * irect.height() / irect.width();

        if (w > width)
            w = width;
        else if (h > height)
            h = height;

        width = w;
        height = h;
    }

    this->outputConvertCaps.setWidth(width);
    this->outputConvertCaps.setHeight(height);
    this->outputConvertCaps.setFps(icaps.fps());

    if (this->outputConvertCaps.width() > irect.width()
        || this->outputConvertCaps.height() > irect.height())
        this->resizeMode = ResizeMode_Up;
    else if (this->outputConvertCaps.width() < irect.width()
             || this->outputConvertCaps.height() < irect.height())
        this->resizeMode = ResizeMode_Down;
    else
        this->resizeMode = ResizeMode_Keep;

    if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Expanding) {
        auto w = irect.height() * this->outputConvertCaps.width() / this->outputConvertCaps.height();
        auto h = irect.width() * this->outputConvertCaps.height() / this->outputConvertCaps.width();

        if (w > irect.width())
            w = irect.width();

        if (h > irect.height())
            h = irect.height();

        auto x = (irect.x() + irect.width() - w) / 2;
        auto y = (irect.y() + irect.height() - h) / 2;
        irect = {x, y, w, h};
    }

    this->allocateBuffers(this->outputConvertCaps);

    int wi_1 = qMax(1, irect.width() - 1);
    int wo_1 = qMax(1, this->outputConvertCaps.width() - 1);

    auto xSrcToDst = [&irect, &wi_1, &wo_1] (int x) -> int {
        return (x - irect.x()) * wo_1 / wi_1;
    };

    auto xDstToSrc = [&irect, &wi_1, &wo_1] (int x) -> int {
        return (x * wi_1 + irect.x() * wo_1) / wo_1;
    };

    for (int x = 0; x < this->outputConvertCaps.width(); ++x) {
        auto xs = xDstToSrc(x);
        auto xs_1 = xDstToSrc(qMin(x + 1, this->outputConvertCaps.width() - 1));
        auto xmin = xSrcToDst(xs);
        auto xmax = xSrcToDst(xs + 1);

        this->srcWidth[x]   = xs;
        this->srcWidth_1[x] = qMin(xDstToSrc(x + 1), icaps.width());
        this->srcWidthOffsetX[x] = (xs >> this->compXi.widthDiv()) * this->compXi.step();
        this->srcWidthOffsetY[x] = (xs >> this->compYi.widthDiv()) * this->compYi.step();
        this->srcWidthOffsetZ[x] = (xs >> this->compZi.widthDiv()) * this->compZi.step();
        this->srcWidthOffsetA[x] = (xs >> this->compAi.widthDiv()) * this->compAi.step();

        this->srcWidthOffsetX_1[x] = (xs_1 >> this->compXi.widthDiv()) * this->compXi.step();
        this->srcWidthOffsetY_1[x] = (xs_1 >> this->compYi.widthDiv()) * this->compYi.step();
        this->srcWidthOffsetZ_1[x] = (xs_1 >> this->compZi.widthDiv()) * this->compZi.step();
        this->srcWidthOffsetA_1[x] = (xs_1 >> this->compAi.widthDiv()) * this->compAi.step();

        this->dstWidthOffsetX[x] = (x >> this->compXo.widthDiv()) * this->compXo.step();
        this->dstWidthOffsetY[x] = (x >> this->compYo.widthDiv()) * this->compYo.step();
        this->dstWidthOffsetZ[x] = (x >> this->compZo.widthDiv()) * this->compZo.step();
        this->dstWidthOffsetA[x] = (x >> this->compAo.widthDiv()) * this->compAo.step();

        if (xmax > xmin)
            this->kx[x] = SCALE_EMULT * (x - xmin) / (xmax - xmin);
        else
            this->kx[x] = 0;
    }

    int hi_1 = qMax(1, irect.height() - 1);
    int ho_1 = qMax(1, this->outputConvertCaps.height() - 1);

    auto ySrcToDst = [&irect, &hi_1, &ho_1] (int y) -> int {
        return (y - irect.y()) * ho_1 / hi_1;
    };

    auto yDstToSrc = [&irect, &hi_1, &ho_1] (int y) -> int {
        return (y * hi_1 + irect.y() * ho_1) / ho_1;
    };

    for (int y = 0; y < this->outputConvertCaps.height(); ++y) {
        if (this->resizeMode == ResizeMode_Down) {
            this->srcHeight[y] = yDstToSrc(y);
            this->srcHeight_1[y] = qMin(yDstToSrc(y + 1), icaps.height());
        } else {
            auto ys = yDstToSrc(y);
            auto ys_1 = yDstToSrc(qMin(y + 1, this->outputConvertCaps.height() - 1));
            auto ymin = ySrcToDst(ys);
            auto ymax = ySrcToDst(ys + 1);

            this->srcHeight[y] = ys;
            this->srcHeight_1[y] = ys_1;

            if (ymax > ymin)
                this->ky[y] = SCALE_EMULT * (y - ymin) / (ymax - ymin);
            else
                this->ky[y] = 0;
        }
    }

    this->inputWidth = icaps.width();
    this->inputWidth_1 = icaps.width() + 1;
    this->inputHeight = icaps.height();
    this->outputWidth = this->outputConvertCaps.width();
    this->outputHeight = this->outputConvertCaps.height();

    this->clearDlBuffers();

    if (this->resizeMode == ResizeMode_Down) {
        this->allocateDlBuffers(icaps, this->outputConvertCaps);

        for (int x = 0; x < icaps.width(); ++x) {
            this->dlSrcWidthOffsetX[x] = (x >> this->compXi.widthDiv()) * this->compXi.step();
            this->dlSrcWidthOffsetY[x] = (x >> this->compYi.widthDiv()) * this->compYi.step();
            this->dlSrcWidthOffsetZ[x] = (x >> this->compZi.widthDiv()) * this->compZi.step();
            this->dlSrcWidthOffsetA[x] = (x >> this->compAi.widthDiv()) * this->compAi.step();
        }

        for (int y = 0; y < this->outputHeight; ++y) {
            auto &ys = this->srcHeight[y];
            auto &ys_1 = this->srcHeight_1[y];

            this->srcHeightDlOffset[y] = size_t(ys) * this->inputWidth_1;
            this->srcHeightDlOffset_1[y] = size_t(ys_1) * this->inputWidth_1;

            auto diffY = ys_1 - ys;
            auto line = this->kdl + size_t(y) * icaps.width();

            for (int x = 0; x < this->outputWidth; ++x) {
                auto diffX = this->srcWidth_1[x] - this->srcWidth[x];
                line[x] = diffX * diffY;
            }
        }
    }

    this->outputFrame = {this->outputConvertCaps};
}

void FrameConvertParameters::reset()
{
    this->inputCaps = AkVideoCaps();
    this->outputCaps = AkVideoCaps();
    this->outputConvertCaps = AkVideoCaps();
    this->outputFrame = AkVideoPacket();
    this->scalingMode = AkVideoConverter::ScalingMode_Fast;
    this->aspectRatioMode = AkVideoConverter::AspectRatioMode_Ignore;
    this->convertType = ConvertType_Vector;
    this->convertDataTypes = ConvertDataTypes_8_8;
    this->alphaMode = AlphaMode_AI_AO;
    this->resizeMode = ResizeMode_Keep;

    this->fromEndian = Q_BYTE_ORDER;
    this->toEndian = Q_BYTE_ORDER;

    this->clearBuffers();
    this->clearDlBuffers();

    this->inputWidth = 0;
    this->inputWidth_1 = 0;
    this->inputHeight = 0;
    this->outputWidth = 0;
    this->outputHeight = 0;

    this->planeXi = 0;
    this->planeYi = 0;
    this->planeZi = 0;
    this->planeAi = 0;

    this->compXi = {};
    this->compYi = {};
    this->compZi = {};
    this->compAi = {};

    this->planeXo = 0;
    this->planeYo = 0;
    this->planeZo = 0;
    this->planeAo = 0;

    this->compXo = {};
    this->compYo = {};
    this->compZo = {};
    this->compAo = {};

    this->xiOffset = 0;
    this->yiOffset = 0;
    this->ziOffset = 0;
    this->aiOffset = 0;

    this->xoOffset = 0;
    this->yoOffset = 0;
    this->zoOffset = 0;
    this->aoOffset = 0;

    this->xiShift = 0;
    this->yiShift = 0;
    this->ziShift = 0;
    this->aiShift = 0;

    this->xoShift = 0;
    this->yoShift = 0;
    this->zoShift = 0;
    this->aoShift = 0;

    this->maxXi = 0;
    this->maxYi = 0;
    this->maxZi = 0;
    this->maxAi = 0;

    this->maskXo = 0;
    this->maskYo = 0;
    this->maskZo = 0;
    this->maskAo = 0;

    this->alphaMask = 0;
}

#include "moc_akvideoconverter.cpp"
