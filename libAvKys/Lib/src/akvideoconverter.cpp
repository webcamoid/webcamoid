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
#include <QImage>
#include <QMutex>
#include <QQmlEngine>
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

using ImageToPixelFormatMap = QMap<QImage::Format, AkVideoCaps::PixelFormat>;

inline ImageToPixelFormatMap initImageToPixelFormatMap()
{
    ImageToPixelFormatMap imageToAkFormat {
        {QImage::Format_RGB32     , AkVideoCaps::Format_0rgbpack},
        {QImage::Format_ARGB32    , AkVideoCaps::Format_argbpack},
        {QImage::Format_RGB16     , AkVideoCaps::Format_rgb565  },
        {QImage::Format_RGB555    , AkVideoCaps::Format_rgb555  },
        {QImage::Format_RGB888    , AkVideoCaps::Format_rgb24   },
        {QImage::Format_RGB444    , AkVideoCaps::Format_rgb444  },
        {QImage::Format_Grayscale8, AkVideoCaps::Format_gray8   }
    };

    return imageToAkFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImageToPixelFormatMap, imageToAkFormat, (initImageToPixelFormatMap()))

enum ColorMatrix
{
    ColorMatrix_ABC2XYZ,
    ColorMatrix_RGB2YUV,
    ColorMatrix_YUV2RGB,
    ColorMatrix_RGB2GRAY,
    ColorMatrix_GRAY2RGB,
    ColorMatrix_YUV2GRAY,
    ColorMatrix_GRAY2YUV,
};

enum YuvColorSpace
{
    YuvColorSpace_AVG,
    YuvColorSpace_ITUR_BT601,
    YuvColorSpace_ITUR_BT709,
    YuvColorSpace_ITUR_BT2020,
    YuvColorSpace_SMPTE_240M
};

enum YuvColorSpaceType
{
    YuvColorSpaceType_StudioSwing,
    YuvColorSpaceType_FullSwing
};

class ColorConvert
{
    public:
        ColorConvert();
        ColorConvert(YuvColorSpace colorSpace,
                     YuvColorSpaceType type=YuvColorSpaceType_StudioSwing);
        ColorConvert(YuvColorSpaceType type);
        inline void applyMatrix(qint64 a, qint64 b, qint64 c,
                                qint64 *x, qint64 *y, qint64 *z) const;
        inline void applyVector(qint64 a, qint64 b, qint64 c,
                                qint64 *x, qint64 *y, qint64 *z) const;
        inline void applyPoint(qint64 p,
                               qint64 *x, qint64 *y, qint64 *z) const;
        inline void applyPoint(qint64 a, qint64 b, qint64 c,
                               qint64 *p) const;
        inline void applyPoint(qint64 p, qint64 *q) const;
        inline void applyAlpha(qint64 x, qint64 y, qint64 z, qint64 a,
                               qint64 *xa, qint64 *ya, qint64 *za) const;
        inline void applyAlpha(qint64 a,
                               qint64 *x, qint64 *y, qint64 *z) const;
        inline void applyAlpha(qint64 p, qint64 a, qint64 *pa) const;
        inline void applyAlpha(qint64 a, qint64 *p) const;
        void setAlphaBits(int abits);
        void loadMatrix(ColorMatrix colorMatrix,
                        int ibitsa,
                        int ibitsb,
                        int ibitsc,
                        int obitsx,
                        int obitsy,
                        int obitsz);
        void loadMatrix(const AkVideoFormatSpec &from,
                        const AkVideoFormatSpec &to);
        void loadMatrix(const AkVideoCaps::PixelFormat &from,
                        const AkVideoCaps::PixelFormat &to);

    private:
        YuvColorSpace colorSpace {YuvColorSpace_ITUR_BT601};
        YuvColorSpaceType type {YuvColorSpaceType_StudioSwing};
        qint64 m00 {0}, m01 {0}, m02 {0}, m03 {0};
        qint64 m10 {0}, m11 {0}, m12 {0}, m13 {0};
        qint64 m20 {0}, m21 {0}, m22 {0}, m23 {0};
        qint64 xmin {0}, xmax {0};
        qint64 ymin {0}, ymax {0};
        qint64 zmin {0}, zmax {0};
        qint64 amax {0};
        qint64 shift {0};

        void rbConstants(YuvColorSpace colorSpace,
                         qint64 &kr,
                         qint64 &kb,
                         qint64 &div) const;
        qint64 roundedDiv(qint64 num, qint64 den) const;
        qint64 nearestPowOf2(qint64 value) const;
        void limitsY(int bits,
                     YuvColorSpaceType type,
                     qint64 &minY,
                     qint64 &maxY) const;
        void limitsUV(int bits,
                      YuvColorSpaceType type,
                      qint64 &minUV,
                      qint64 &maxUV) const;
        void loadAbc2xyzMatrix(int abits,
                               int bbits,
                               int cbits,
                               int xbits,
                               int ybits,
                               int zbits);
        void loadRgb2yuvMatrix(YuvColorSpace colorSpace,
                               YuvColorSpaceType type,
                               int rbits,
                               int gbits,
                               int bbits,
                               int ybits,
                               int ubits,
                               int vbits);
        void loadYuv2rgbMatrix(YuvColorSpace colorSpace,
                               YuvColorSpaceType type,
                               int ybits,
                               int ubits,
                               int vbits,
                               int rbits,
                               int gbits,
                               int bbits);
        void loadRgb2grayMatrix(YuvColorSpace colorSpace, int rbits,
                                int gbits,
                                int bbits,
                                int graybits);
        void loadGray2rgbMatrix(int graybits,
                                int rbits,
                                int gbits,
                                int bbits);
        void loadYuv2grayMatrix(YuvColorSpaceType type,
                                int ybits,
                                int ubits,
                                int vbits,
                                int graybits);
        void loadGray2yuvMatrix(YuvColorSpaceType type,
                                int graybits,
                                int ybits,
                                int ubits,
                                int vbits);
};

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
        AkVideoCaps inputCaps;
        AkVideoCaps outputCaps;
        AkVideoCaps outputConvertCaps;
        AkVideoPacket outputFrame;
        AkVideoConverter::ScalingMode scalingMode {AkVideoConverter::ScalingMode_Fast};
        AkVideoConverter::AspectRatioMode aspectRatioMode {AkVideoConverter::AspectRatioMode_Ignore};
        ConvertType convertType {ConvertType_Vector};
        ConvertDataTypes convertDataTypes {ConvertDataTypes_8_8};
        AlphaMode alphaMode {AlphaMode_AI_AO};
        ResizeMode resizeMode {ResizeMode_Keep};

        int fromEndian {Q_BYTE_ORDER};
        int toEndian {Q_BYTE_ORDER};

        int inputWidth {0};
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

        DlSumType **integralImageDataX {nullptr};
        DlSumType **integralImageDataY {nullptr};
        DlSumType **integralImageDataZ {nullptr};
        DlSumType **integralImageDataA {nullptr};

        qint64 *kx {nullptr};
        qint64 *ky {nullptr};
        DlSumType **kdl {nullptr};

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

        ~FrameConvertParameters();
        inline void clearBuffers();
        inline void clearDlBuffers();
        inline void allocateBuffers(int width, int height);
        inline void allocateDlBuffers(int width, int height);
        void setOutputConvertCaps(const AkVideoCaps &icaps,
                                  const AkVideoCaps &ocaps,
                                  AkVideoConverter::AspectRatioMode aspectRatioMode);
        void configure(const AkVideoCaps &icaps,
                       const AkVideoCaps &ocaps,
                       ColorConvert &colorConvert);
        void configureScaling(const AkVideoCaps &icaps,
                              const AkVideoCaps &ocaps,
                              AkVideoConverter::AspectRatioMode aspectRatioMode);
        void reset();
};

class ImageConvertParameters
{
    public:
        AkVideoCaps inputCaps;
        AkVideoCaps outputCaps;
        AkVideoCaps outputConvertCaps;
        QImage outputImage;
        QImage::Format outputImageFormat {QImage::Format_Invalid};
        QImage::Format inputImageFormat {QImage::Format_Invalid};
        AkVideoPacket inputPacket;
        int inputImageWidth {0};
        int inputImageHeight {0};
        size_t lineSizePi {0};
        size_t lineSizeIp {0};
        bool canConvert {false};
        bool directConvertPi {false};
        bool directConvertIp {false};

        inline size_t lineSize(const AkVideoCaps &caps) const;
        void configure(const QImage &image,
                       const AkVideoCaps &caps);
        void configure(const AkVideoCaps &icaps,
                       const AkVideoCaps &ocaps,
                       QImage::Format format,
                       AkVideoConverter::AspectRatioMode aspectRatioMode);
        void configure(const AkVideoCaps &icaps,
                       const AkVideoCaps &ocaps,
                       AkVideoConverter::AspectRatioMode aspectRatioMode);
        void reset();

        template<typename T>
        static inline T alignUp(const T &value, const T &align)
        {
            return (value + align - 1) & ~(align - 1);
        }
};

class AkVideoConverterPrivate
{
    public:
        QMutex m_mutex;
        ColorConvert m_colorConvert;
        AkVideoCaps m_outputCaps;
        FrameConvertParameters m_fc;
        ImageConvertParameters m_ic;
        AkVideoConverter::ScalingMode m_scalingMode {AkVideoConverter::ScalingMode_Fast};
        AkVideoConverter::AspectRatioMode m_aspectRatioMode {AkVideoConverter::AspectRatioMode_Ignore};

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
        inline void integralImage1(const AkVideoPacket &src) const
        {
            for (int y = 0; y < this->m_fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(this->m_fc.planeXi, y) + this->m_fc.xiOffset;
                auto dst_line_x = this->m_fc.integralImageDataX[y];
                auto dst_line_x_1 = this->m_fc.integralImageDataX[y + 1];

                // Reset current line summation.

                DlSumType sumX = 0;

                for (int x = 0; x < this->m_fc.inputWidth; ++x) {
                    int &xs_x = this->m_fc.dlSrcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);

                    // Accumulate pixels in current line.

                    sumX += (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                }
            }
        }

        template <typename InputType>
        inline void integralImage1A(const AkVideoPacket &src) const
        {
            for (int y = 0; y < this->m_fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(this->m_fc.planeXi, y) + this->m_fc.xiOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, y) + this->m_fc.aiOffset;

                auto dst_line_x = this->m_fc.integralImageDataX[y];
                auto dst_line_a = this->m_fc.integralImageDataA[y];

                int y_1 = y + 1;

                auto dst_line_x_1 = this->m_fc.integralImageDataX[y_1];
                auto dst_line_a_1 = this->m_fc.integralImageDataA[y_1];

                // Reset current line summation.

                DlSumType sumX = 0;
                DlSumType sumA = 0;

                for (int x = 0; x < this->m_fc.inputWidth; ++x) {
                    int &xs_x = this->m_fc.dlSrcWidthOffsetX[x];
                    int &xs_a = this->m_fc.dlSrcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    // Accumulate pixels in current line.

                    sumX += (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    sumA += (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                    dst_line_a_1[x_1] = sumA + dst_line_a[x_1];
                }
            }
        }

        template <typename InputType>
        inline void integralImage3(const AkVideoPacket &src) const
        {
            for (int y = 0; y < this->m_fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(this->m_fc.planeXi, y) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, y) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, y) + this->m_fc.ziOffset;

                auto dst_line_x = this->m_fc.integralImageDataX[y];
                auto dst_line_y = this->m_fc.integralImageDataY[y];
                auto dst_line_z = this->m_fc.integralImageDataZ[y];

                int y_1 = y + 1;

                auto dst_line_x_1 = this->m_fc.integralImageDataX[y_1];
                auto dst_line_y_1 = this->m_fc.integralImageDataY[y_1];
                auto dst_line_z_1 = this->m_fc.integralImageDataZ[y_1];

                // Reset current line summation.

                DlSumType sumX = 0;
                DlSumType sumY = 0;
                DlSumType sumZ = 0;

                for (int x = 0; x < this->m_fc.inputWidth; ++x) {
                    int &xs_x = this->m_fc.dlSrcWidthOffsetX[x];
                    int &xs_y = this->m_fc.dlSrcWidthOffsetY[x];
                    int &xs_z = this->m_fc.dlSrcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    // Accumulate pixels in current line.

                    sumX += (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    sumY += (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    sumZ += (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                    dst_line_y_1[x_1] = sumY + dst_line_y[x_1];
                    dst_line_z_1[x_1] = sumZ + dst_line_z[x_1];
                }
            }
        }

        template <typename InputType>
        inline void integralImage3A(const AkVideoPacket &src) const
        {
            for (int y = 0; y < this->m_fc.inputHeight; ++y) {
                auto src_line_x = src.constLine(this->m_fc.planeXi, y) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, y) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, y) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, y) + this->m_fc.aiOffset;

                auto dst_line_x = this->m_fc.integralImageDataX[y];
                auto dst_line_y = this->m_fc.integralImageDataY[y];
                auto dst_line_z = this->m_fc.integralImageDataZ[y];
                auto dst_line_a = this->m_fc.integralImageDataA[y];

                int y_1 = y + 1;

                auto dst_line_x_1 = this->m_fc.integralImageDataX[y_1];
                auto dst_line_y_1 = this->m_fc.integralImageDataY[y_1];
                auto dst_line_z_1 = this->m_fc.integralImageDataZ[y_1];
                auto dst_line_a_1 = this->m_fc.integralImageDataA[y_1];

                // Reset current line summation.

                DlSumType sumX = 0;
                DlSumType sumY = 0;
                DlSumType sumZ = 0;
                DlSumType sumA = 0;

                for (int x = 0; x < this->m_fc.inputWidth; ++x) {
                    int &xs_x = this->m_fc.dlSrcWidthOffsetX[x];
                    int &xs_y = this->m_fc.dlSrcWidthOffsetY[x];
                    int &xs_z = this->m_fc.dlSrcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.dlSrcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    // Accumulate pixels in current line.

                    sumX += (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    sumY += (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    sumZ += (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    sumA += (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    // Accumulate current line and previous line.

                    int x_1 = x + 1;
                    dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                    dst_line_y_1[x_1] = sumY + dst_line_y[x_1];
                    dst_line_z_1[x_1] = sumZ + dst_line_z[x_1];
                    dst_line_a_1[x_1] = sumA + dst_line_a[x_1];
                }
            }
        }

        /* Fast conversion functions */

        // Conversion functions for 3 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convert3to3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3to3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);
                    this->m_colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (ai << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

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
        void convertV3to3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertV3to3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertV3Ato3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);
                    this->m_colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertV3Ato3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (ai << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 3 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convert3to1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.zoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, yi, zi, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3to1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, yi, zi, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, yi, zi, &xo_);
                    this->m_colorConvert.applyAlpha(ai, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert3Ato1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    yi = (this->swapBytes(InputType(yi), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    zi = (this->swapBytes(InputType(zi), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    ai = (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, yi, zi, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (ai << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convert1to3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1to3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    ai = (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);
                    this->m_colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    ai = (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (ai << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convert1to1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1to1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    ai = (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_);
                    this->m_colorConvert.applyAlpha(ai, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convert1Ato1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    ai = (this->swapBytes(InputType(ai), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (ai << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        /* Linear downscaling conversion funtions */

        // Conversion functions for 3 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertDL3to3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi,
                                                     yi,
                                                     zi,
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3to3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi,
                                                     yi,
                                                     zi,
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];
                auto src_line_a = this->m_fc.integralImageDataA[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];
                auto src_line_a_1 = this->m_fc.integralImageDataA[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi,
                                                     yi,
                                                     zi,
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);
                    this->m_colorConvert.applyAlpha(ai,
                                                    &xo_,
                                                    &yo_,
                                                    &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];
                auto src_line_a = this->m_fc.integralImageDataA[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];
                auto src_line_a_1 = this->m_fc.integralImageDataA[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi,
                                                     yi,
                                                     zi,
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (OutputType(ai) << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

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
        void convertDLV3to3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi,
                                                     yi,
                                                     zi,
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDLV3to3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi,
                                                     yi,
                                                     zi,
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDLV3Ato3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];
                auto src_line_a = this->m_fc.integralImageDataA[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];
                auto src_line_a_1 = this->m_fc.integralImageDataA[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi,
                                                     yi,
                                                     zi,
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);
                    this->m_colorConvert.applyAlpha(ai,
                                                    &xo_,
                                                    &yo_,
                                                    &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDLV3Ato3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];
                auto src_line_a = this->m_fc.integralImageDataA[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];
                auto src_line_a_1 = this->m_fc.integralImageDataA[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi,
                                                     yi,
                                                     zi,
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (OutputType(ai) << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 3 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertDL3to1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y);

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi,
                                                    yi,
                                                    zi,
                                                    &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3to1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi,
                                                    yi,
                                                    zi,
                                                    &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];
                auto src_line_a = this->m_fc.integralImageDataA[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];
                auto src_line_a_1 = this->m_fc.integralImageDataA[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y);

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi,
                                                    yi,
                                                    zi,
                                                    &xo_);
                    this->m_colorConvert.applyAlpha(ai, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL3Ato1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_y = this->m_fc.integralImageDataY[ys];
                auto src_line_z = this->m_fc.integralImageDataZ[ys];
                auto src_line_a = this->m_fc.integralImageDataA[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_y_1 = this->m_fc.integralImageDataY[ys_1];
                auto src_line_z_1 = this->m_fc.integralImageDataZ[ys_1];
                auto src_line_a_1 = this->m_fc.integralImageDataA[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                    auto zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi,
                                                    yi,
                                                    zi,
                                                    &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (OutputType(ai) << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertDL1to3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi,
                                                    &xo_,
                                                    &yo_,
                                                    &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1to3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi,
                                                    &xo_,
                                                    &yo_,
                                                    &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_a = this->m_fc.integralImageDataA[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_a_1 = this->m_fc.integralImageDataA[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi,
                                                    &xo_,
                                                    &yo_,
                                                    &zo_);
                    this->m_colorConvert.applyAlpha(ai,
                                                    &xo_,
                                                    &yo_,
                                                    &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_a = this->m_fc.integralImageDataA[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_a_1 = this->m_fc.integralImageDataA[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi,
                                                    &xo_,
                                                    &yo_,
                                                    &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (OutputType(ai) << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertDL1to1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1to1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_a = this->m_fc.integralImageDataA[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_a_1 = this->m_fc.integralImageDataA[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_);
                    this->m_colorConvert.applyAlpha(ai, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertDL1Ato1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = this->m_fc.integralImageDataX[ys];
                auto src_line_a = this->m_fc.integralImageDataA[ys];

                auto src_line_x_1 = this->m_fc.integralImageDataX[ys_1];
                auto src_line_a_1 = this->m_fc.integralImageDataA[ys_1];

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto kdl = this->m_fc.kdl[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    auto &xs = this->m_fc.srcWidth[x];
                    auto &xs_1 = this->m_fc.srcWidth_1[x];
                    auto &k = kdl[x];

                    auto xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                    auto ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (OutputType(ai) << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        /* Linear upscaling conversion funtions */

        // Conversion functions for 3 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertUL3to3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              this->m_fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xyzib[0],
                                                     xyzib[1],
                                                     xyzib[2],
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3to3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              this->m_fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xyzib[0],
                                                     xyzib[1],
                                                     xyzib[2],
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;
                auto src_line_a_1 = src.constLine(this->m_fc.planeAi, ys_1) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = this->m_fc.srcWidthOffsetA_1[x];

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

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              this->m_fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xyzaib[0],
                                                     xyzaib[1],
                                                     xyzaib[2],
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);
                    this->m_colorConvert.applyAlpha(xyzaib[3],
                                                    &xo_,
                                                    &yo_,
                                                    &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;
                auto src_line_a_1 = src.constLine(this->m_fc.planeAi, ys_1) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = this->m_fc.srcWidthOffsetA_1[x];

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

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              this->m_fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xyzaib[0],
                                                     xyzaib[1],
                                                     xyzaib[2],
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (xyzaib[3] << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

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
        void convertULV3to3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              this->m_fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xyzib[0],
                                                     xyzib[1],
                                                     xyzib[2],
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertULV3to3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              this->m_fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xyzib[0],
                                                     xyzib[1],
                                                     xyzib[2],
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertULV3Ato3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;
                auto src_line_a_1 = src.constLine(this->m_fc.planeAi, ys_1) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = this->m_fc.srcWidthOffsetA_1[x];

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

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              this->m_fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xyzaib[0],
                                                     xyzaib[1],
                                                     xyzaib[2],
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);
                    this->m_colorConvert.applyAlpha(xyzaib[3],
                                                    &xo_,
                                                    &yo_,
                                                    &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertULV3Ato3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;
                auto src_line_a_1 = src.constLine(this->m_fc.planeAi, ys_1) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = this->m_fc.srcWidthOffsetA_1[x];

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

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              this->m_fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xyzaib[0],
                                                     xyzaib[1],
                                                     xyzaib[2],
                                                     &xo_,
                                                     &yo_,
                                                     &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (xyzaib[3] << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 3 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertUL3to1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y);

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              this->m_fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xyzib[0],
                                                    xyzib[1],
                                                    xyzib[2],
                                                    &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3to1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzi[3];
            qint64 xyzi_x[3];
            qint64 xyzi_y[3];
            qint64 xyzib[3];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];

                    xyzi[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xyzi[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    xyzi[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    xyzi_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xyzi_x[1] = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                    xyzi_x[2] = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                    xyzi_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xyzi_y[1] = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                    xyzi_y[2] = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                    xyzi[0] = (this->swapBytes(InputType(xyzi[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi[1] = (this->swapBytes(InputType(xyzi[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi[2] = (this->swapBytes(InputType(xyzi[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_x[0] = (this->swapBytes(InputType(xyzi_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_x[1] = (this->swapBytes(InputType(xyzi_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_x[2] = (this->swapBytes(InputType(xyzi_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzi_y[0] = (this->swapBytes(InputType(xyzi_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzi_y[1] = (this->swapBytes(InputType(xyzi_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzi_y[2] = (this->swapBytes(InputType(xyzi_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;

                    this->blend3<SCALE_EMULT>(xyzi,
                                              xyzi_x, xyzi_y,
                                              this->m_fc.kx[x], ky,
                                              xyzib);

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xyzib[0],
                                                    xyzib[1],
                                                    xyzib[2],
                                                    &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;
                auto src_line_a_1 = src.constLine(this->m_fc.planeAi, ys_1) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y);

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = this->m_fc.srcWidthOffsetA_1[x];

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

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              this->m_fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xyzaib[0],
                                                    xyzaib[1],
                                                    xyzaib[2],
                                                    &xo_);
                    this->m_colorConvert.applyAlpha(xyzaib[3], &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL3Ato1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xyzai[4];
            qint64 xyzai_x[4];
            qint64 xyzai_y[4];
            qint64 xyzaib[4];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_y = src.constLine(this->m_fc.planeYi, ys) + this->m_fc.yiOffset;
                auto src_line_z = src.constLine(this->m_fc.planeZi, ys) + this->m_fc.ziOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;

                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_y_1 = src.constLine(this->m_fc.planeYi, ys_1) + this->m_fc.yiOffset;
                auto src_line_z_1 = src.constLine(this->m_fc.planeZi, ys_1) + this->m_fc.ziOffset;
                auto src_line_a_1 = src.constLine(this->m_fc.planeAi, ys_1) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_y = this->m_fc.srcWidthOffsetY[x];
                    int &xs_z = this->m_fc.srcWidthOffsetZ[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_y_1 = this->m_fc.srcWidthOffsetY_1[x];
                    int &xs_z_1 = this->m_fc.srcWidthOffsetZ_1[x];
                    int &xs_a_1 = this->m_fc.srcWidthOffsetA_1[x];

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

                    xyzai[0] = (this->swapBytes(InputType(xyzai[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai[1] = (this->swapBytes(InputType(xyzai[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai[2] = (this->swapBytes(InputType(xyzai[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai[3] = (this->swapBytes(InputType(xyzai[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_x[0] = (this->swapBytes(InputType(xyzai_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_x[1] = (this->swapBytes(InputType(xyzai_x[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_x[2] = (this->swapBytes(InputType(xyzai_x[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_x[3] = (this->swapBytes(InputType(xyzai_x[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xyzai_y[0] = (this->swapBytes(InputType(xyzai_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xyzai_y[1] = (this->swapBytes(InputType(xyzai_y[1]), this->m_fc.fromEndian) >> this->m_fc.yiShift) & this->m_fc.maxYi;
                    xyzai_y[2] = (this->swapBytes(InputType(xyzai_y[2]), this->m_fc.fromEndian) >> this->m_fc.ziShift) & this->m_fc.maxZi;
                    xyzai_y[3] = (this->swapBytes(InputType(xyzai_y[3]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    this->blend4<SCALE_EMULT>(xyzai,
                                              xyzai_x, xyzai_y,
                                              this->m_fc.kx[x], ky,
                                              xyzaib);

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xyzaib[0],
                                                    xyzaib[1],
                                                    xyzaib[2],
                                                    &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (xyzaib[3] << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 3 components formats

        template <typename InputType, typename OutputType>
        void convertUL1to3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xib = 0;

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto xi_x = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    auto xi_y = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xi_x = (this->swapBytes(InputType(xi_x), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xi_y = (this->swapBytes(InputType(xi_y), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;

                    this->blend<SCALE_EMULT>(xi,
                                             xi_x, xi_y,
                                             this->m_fc.kx[x], ky,
                                             &xib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xib, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1to3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xib = 0;

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto xi_x = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    auto xi_y = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xi_x = (this->swapBytes(InputType(xi_x), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xi_y = (this->swapBytes(InputType(xi_y), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;

                    this->blend<SCALE_EMULT>(xi,
                                             xi_x, xi_y,
                                             this->m_fc.kx[x], ky,
                                             &xib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xib, &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato3(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xai[2];
            qint64 xai_x[2];
            qint64 xai_y[2];
            qint64 xaib[2];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;
                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_a_1 = src.constLine(this->m_fc.planeAi, ys_1) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_a_1 = this->m_fc.srcWidthOffsetA_1[x];

                    xai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xai[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xai_x[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xai_y[1] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xai[0] = (this->swapBytes(InputType(xai[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai[1] = (this->swapBytes(InputType(xai[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xai_x[0] = (this->swapBytes(InputType(xai_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai_x[1] = (this->swapBytes(InputType(xai_x[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xai_y[0] = (this->swapBytes(InputType(xai_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai_y[1] = (this->swapBytes(InputType(xai_y[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    this->blend2<SCALE_EMULT>(xai,
                                              xai_x, xai_y,
                                              this->m_fc.kx[x], ky,
                                              xaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xaib[0], &xo_, &yo_, &zo_);
                    this->m_colorConvert.applyAlpha(xaib[1], &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato3A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xai[2];
            qint64 xai_x[2];
            qint64 xai_y[2];
            qint64 xaib[2];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;
                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_a_1 = src.constLine(this->m_fc.planeAi, ys_1) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_y = dst.line(this->m_fc.planeYo, y) + this->m_fc.yoOffset;
                auto dst_line_z = dst.line(this->m_fc.planeZo, y) + this->m_fc.zoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_a_1 = this->m_fc.srcWidthOffsetA_1[x];

                    xai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xai[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xai_x[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xai_y[1] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xai[0] = (this->swapBytes(InputType(xai[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai[1] = (this->swapBytes(InputType(xai[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xai_x[0] = (this->swapBytes(InputType(xai_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai_x[1] = (this->swapBytes(InputType(xai_x[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xai_y[0] = (this->swapBytes(InputType(xai_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai_y[1] = (this->swapBytes(InputType(xai_y[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    this->blend2<SCALE_EMULT>(xai,
                                              xai_x, xai_y,
                                              this->m_fc.kx[x], ky,
                                              xaib);

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xaib[0], &xo_, &yo_, &zo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_y = this->m_fc.dstWidthOffsetY[x];
                    int &xd_z = this->m_fc.dstWidthOffsetZ[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *yo = (*yo & this->m_fc.maskYo) | (yo_ << this->m_fc.yoShift);
                    *zo = (*zo & this->m_fc.maskZo) | (zo_ << this->m_fc.zoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (xaib[1] << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto yot = this->swapBytes(OutputType(*yo), this->m_fc.toEndian);
                    auto zot = this->swapBytes(OutputType(*zo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 1 components formats

        template <typename InputType, typename OutputType>
        void convertUL1to1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xib = 0;

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto xi_x = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    auto xi_y = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xi_x = (this->swapBytes(InputType(xi_x), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xi_y = (this->swapBytes(InputType(xi_y), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;

                    this->blend<SCALE_EMULT>(xi,
                                             xi_x, xi_y,
                                             this->m_fc.kx[x], ky,
                                             &xib);

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xib, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1to1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xib = 0;

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto xi_x = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    auto xi_y = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);

                    xi = (this->swapBytes(InputType(xi), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xi_x = (this->swapBytes(InputType(xi_x), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xi_y = (this->swapBytes(InputType(xi_y), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;

                    this->blend<SCALE_EMULT>(xi,
                                             xi_x, xi_y,
                                             this->m_fc.kx[x], ky,
                                             &xib);

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xib, &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = *ao | this->m_fc.alphaMask;

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato1(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xai[2];
            qint64 xai_x[2];
            qint64 xai_y[2];
            qint64 xaib[2];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;
                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_a_1 = src.constLine(this->m_fc.planeAi, ys_1) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_a_1 = this->m_fc.srcWidthOffsetA_1[x];

                    xai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xai[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xai_x[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xai_y[1] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xai[0] = (this->swapBytes(InputType(xai[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai[1] = (this->swapBytes(InputType(xai[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xai_x[0] = (this->swapBytes(InputType(xai_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai_x[1] = (this->swapBytes(InputType(xai_x[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xai_y[0] = (this->swapBytes(InputType(xai_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai_y[1] = (this->swapBytes(InputType(xai_y[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    this->blend2<SCALE_EMULT>(xai,
                                              xai_x, xai_y,
                                              this->m_fc.kx[x], ky,
                                              xaib);

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xaib[0], &xo_);
                    this->m_colorConvert.applyAlpha(xaib[1], &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *xo = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                }
            }
        }

        template <typename InputType, typename OutputType>
        void convertUL1Ato1A(const AkVideoPacket &src, AkVideoPacket &dst) const
        {
            qint64 xai[2];
            qint64 xai_x[2];
            qint64 xai_y[2];
            qint64 xaib[2];

            for (int y = 0; y < this->m_fc.outputHeight; ++y) {
                auto &ys = this->m_fc.srcHeight[y];
                auto &ys_1 = this->m_fc.srcHeight_1[y];

                auto src_line_x = src.constLine(this->m_fc.planeXi, ys) + this->m_fc.xiOffset;
                auto src_line_a = src.constLine(this->m_fc.planeAi, ys) + this->m_fc.aiOffset;
                auto src_line_x_1 = src.constLine(this->m_fc.planeXi, ys_1) + this->m_fc.xiOffset;
                auto src_line_a_1 = src.constLine(this->m_fc.planeAi, ys_1) + this->m_fc.aiOffset;

                auto dst_line_x = dst.line(this->m_fc.planeXo, y) + this->m_fc.xoOffset;
                auto dst_line_a = dst.line(this->m_fc.planeAo, y) + this->m_fc.aoOffset;

                auto &ky = this->m_fc.ky[y];

                for (int x = 0; x < this->m_fc.outputWidth; ++x) {
                    int &xs_x = this->m_fc.srcWidthOffsetX[x];
                    int &xs_a = this->m_fc.srcWidthOffsetA[x];

                    int &xs_x_1 = this->m_fc.srcWidthOffsetX_1[x];
                    int &xs_a_1 = this->m_fc.srcWidthOffsetA_1[x];

                    xai[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xai[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                    xai_x[0] = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                    xai_x[1] = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                    xai_y[0] = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                    xai_y[1] = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                    xai[0] = (this->swapBytes(InputType(xai[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai[1] = (this->swapBytes(InputType(xai[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xai_x[0] = (this->swapBytes(InputType(xai_x[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai_x[1] = (this->swapBytes(InputType(xai_x[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;
                    xai_y[0] = (this->swapBytes(InputType(xai_y[0]), this->m_fc.fromEndian) >> this->m_fc.xiShift) & this->m_fc.maxXi;
                    xai_y[1] = (this->swapBytes(InputType(xai_y[1]), this->m_fc.fromEndian) >> this->m_fc.aiShift) & this->m_fc.maxAi;

                    this->blend2<SCALE_EMULT>(xai,
                                              xai_x, xai_y,
                                              this->m_fc.kx[x], ky,
                                              xaib);

                    qint64 xo_ = 0;
                    this->m_colorConvert.applyPoint(xaib[0], &xo_);

                    int &xd_x = this->m_fc.dstWidthOffsetX[x];
                    int &xd_a = this->m_fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_fc.maskXo) | (xo_ << this->m_fc.xoShift);
                    *ao = (*ao & this->m_fc.maskAo) | (xaib[1] << this->m_fc.aoShift);

                    auto xot = this->swapBytes(OutputType(*xo), this->m_fc.toEndian);
                    auto aot = this->swapBytes(OutputType(*ao), this->m_fc.toEndian);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

#define CONVERT_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertFormat##icomponents##to##ocomponents(const AkVideoPacket &src, \
                                                                AkVideoPacket &dst) const \
        { \
            switch (this->m_fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convert##icomponents##Ato##ocomponents##A<InputType, OutputType>(src, \
                                                                                       dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convert##icomponents##Ato##ocomponents<InputType, OutputType>(src, \
                                                                                    dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convert##icomponents##to##ocomponents##A<InputType, OutputType>(src, \
                                                                                      dst); \
                break; \
            case AlphaMode_I_O: \
                this->convert##icomponents##to##ocomponents<InputType, OutputType>(src, \
                                                                                   dst); \
                break; \
            }; \
        }

#define CONVERTV_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertVFormat##icomponents##to##ocomponents(const AkVideoPacket &src, \
                                                                 AkVideoPacket &dst) const \
        { \
            switch (this->m_fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convertV##icomponents##Ato##ocomponents##A<InputType, OutputType>(src, \
                                                                                        dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convertV##icomponents##Ato##ocomponents<InputType, OutputType>(src, \
                                                                                     dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convertV##icomponents##to##ocomponents##A<InputType, OutputType>(src, \
                                                                                       dst); \
                break; \
            case AlphaMode_I_O: \
                this->convertV##icomponents##to##ocomponents<InputType, OutputType>(src, \
                                                                                    dst); \
                break; \
            }; \
        }

#define CONVERTDL_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertDLFormat##icomponents##to##ocomponents(const AkVideoPacket &src, \
                                                                  AkVideoPacket &dst) const \
        { \
            switch (this->m_fc.alphaMode) { \
            case AlphaMode_AI_AO: \
            case AlphaMode_AI_O: \
                this->integralImage##icomponents##A<InputType>(src); \
                break; \
            default: \
                this->integralImage##icomponents<InputType>(src); \
                break; \
            } \
            \
            switch (this->m_fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convertDL##icomponents##Ato##ocomponents##A<InputType, OutputType>(src, \
                                                                                         dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convertDL##icomponents##Ato##ocomponents<InputType, OutputType>(src, \
                                                                                      dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convertDL##icomponents##to##ocomponents##A<InputType, OutputType>(src, \
                                                                                        dst); \
                break; \
            case AlphaMode_I_O: \
                this->convertDL##icomponents##to##ocomponents<InputType, OutputType>(src, \
                                                                                     dst); \
                break; \
            }; \
        }

#define CONVERTDLV_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertDLVFormat##icomponents##to##ocomponents(const AkVideoPacket &src, \
                                                                   AkVideoPacket &dst) const \
        { \
            switch (this->m_fc.alphaMode) { \
            case AlphaMode_AI_AO: \
            case AlphaMode_AI_O: \
                this->integralImage##icomponents##A<InputType>(src); \
                break; \
            default: \
                this->integralImage##icomponents<InputType>(src); \
                break; \
            } \
            \
            switch (this->m_fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convertDLV##icomponents##Ato##ocomponents##A<InputType, OutputType>(src, \
                                                                                          dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convertDLV##icomponents##Ato##ocomponents<InputType, OutputType>(src, \
                                                                                       dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convertDLV##icomponents##to##ocomponents##A<InputType, OutputType>(src, \
                                                                                         dst); \
                break; \
            case AlphaMode_I_O: \
                this->convertDLV##icomponents##to##ocomponents<InputType, OutputType>(src, \
                                                                                      dst); \
                break; \
            }; \
        }

#define CONVERTUL_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertULFormat##icomponents##to##ocomponents(const AkVideoPacket &src, \
                                                                  AkVideoPacket &dst) const \
        { \
            switch (this->m_fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convertUL##icomponents##Ato##ocomponents##A<InputType, OutputType>(src, \
                                                                                         dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convertUL##icomponents##Ato##ocomponents<InputType, OutputType>(src, \
                                                                                      dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convertUL##icomponents##to##ocomponents##A<InputType, OutputType>(src, \
                                                                                        dst); \
                break; \
            case AlphaMode_I_O: \
                this->convertUL##icomponents##to##ocomponents<InputType, OutputType>(src, \
                                                                                     dst); \
                break; \
            }; \
        }

#define CONVERTULV_FUNC(icomponents, ocomponents) \
        template <typename InputType, typename OutputType> \
        inline void convertULVFormat##icomponents##to##ocomponents(const AkVideoPacket &src, \
                                                                   AkVideoPacket &dst) const \
        { \
            switch (this->m_fc.alphaMode) { \
            case AlphaMode_AI_AO: \
                this->convertULV##icomponents##Ato##ocomponents##A<InputType, OutputType>(src, \
                                                                                          dst); \
                break; \
            case AlphaMode_AI_O: \
                this->convertULV##icomponents##Ato##ocomponents<InputType, OutputType>(src, \
                                                                                       dst); \
                break; \
            case AlphaMode_I_AO: \
                this->convertULV##icomponents##to##ocomponents##A<InputType, OutputType>(src, \
                                                                                         dst); \
                break; \
            case AlphaMode_I_O: \
                this->convertULV##icomponents##to##ocomponents<InputType, OutputType>(src, \
                                                                                      dst); \
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
        inline void convert(const AkVideoPacket &src,
                            AkVideoPacket &dst)
        {
            if (this->m_scalingMode == AkVideoConverter::ScalingMode_Linear
                && this->m_fc.resizeMode == ResizeMode_Up) {
                switch (this->m_fc.convertType) {
                case ConvertType_Vector:
                    this->convertULVFormat3to3<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertULFormat3to3<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertULFormat3to1<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertULFormat1to3<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertULFormat1to1<InputType, OutputType>(src, dst);
                    break;
                }
            } else if (this->m_scalingMode == AkVideoConverter::ScalingMode_Linear
                       && this->m_fc.resizeMode == ResizeMode_Down) {
                switch (this->m_fc.convertType) {
                case ConvertType_Vector:
                    this->convertDLVFormat3to3<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertDLFormat3to3<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertDLFormat3to1<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertDLFormat1to3<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertDLFormat1to1<InputType, OutputType>(src, dst);
                    break;
                }
            } else {
                switch (this->m_fc.convertType) {
                case ConvertType_Vector:
                    this->convertVFormat3to3<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_3to3:
                    this->convertFormat3to3<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_3to1:
                    this->convertFormat3to1<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_1to3:
                    this->convertFormat1to3<InputType, OutputType>(src, dst);
                    break;
                case ConvertType_1to1:
                    this->convertFormat1to1<InputType, OutputType>(src, dst);
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

AkVideoConverter::AkVideoConverter(const AkVideoConverter &other)
{
    this->d = new AkVideoConverterPrivate();
    this->d->m_outputCaps = other.d->m_outputCaps;
    this->d->m_scalingMode = other.d->m_scalingMode;
    this->d->m_aspectRatioMode = other.d->m_aspectRatioMode;
}

AkVideoConverter::~AkVideoConverter()
{
    delete this->d;
}

AkVideoConverter &AkVideoConverter::operator =(const AkVideoConverter &other)
{
    if (this != &other) {
        this->d->m_outputCaps = other.d->m_outputCaps;
        this->d->m_scalingMode = other.d->m_scalingMode;
        this->d->m_aspectRatioMode = other.d->m_aspectRatioMode;
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

AkVideoConverter::ScalingMode AkVideoConverter::scalingMode() const
{
    return this->d->m_scalingMode;
}

AkVideoConverter::AspectRatioMode AkVideoConverter::aspectRatioMode() const
{
    return this->d->m_aspectRatioMode;
}

AkVideoPacket AkVideoConverter::convert(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    auto &caps = packet.caps();

    if (caps.format() == this->d->m_outputCaps.format()
        && caps.width() == this->d->m_outputCaps.width()
        && caps.height() == this->d->m_outputCaps.height())
        return packet;

    return this->d->convert(packet, this->d->m_outputCaps);
}

AkVideoPacket AkVideoConverter::convert(const QImage &image)
{
    return this->convert(image, {});
}

AkVideoPacket AkVideoConverter::convert(const QImage &image,
                                        const AkVideoPacket &defaultPacket)
{
    if (image.isNull())
        return {};

    if (image.format() != this->d->m_ic.inputImageFormat
        || image.width() != this->d->m_ic.inputImageWidth
        || image.height() != this->d->m_ic.inputImageHeight
        || this->d->m_outputCaps != this->d->m_ic.outputCaps) {
        this->d->m_ic.configure(image, defaultPacket.caps());
        this->d->m_ic.inputImageFormat = image.format();
        this->d->m_ic.inputImageWidth = image.width();
        this->d->m_ic.inputImageHeight = image.height();
        this->d->m_ic.outputCaps = this->d->m_outputCaps;
    }

    this->d->m_ic.inputPacket.copyMetadata(defaultPacket);

    if (this->d->m_ic.directConvertIp) {
        for (int y = 0; y < image.height(); ++y) {
            auto srcLine = image.constScanLine(y);
            auto dstLine = this->d->m_ic.inputPacket.line(0, y);
            memcpy(dstLine, srcLine, this->d->m_ic.lineSizeIp);
        }
    } else {
        auto imgSrc = image.convertToFormat(QImage::Format_ARGB32);
        auto lineSize =
                qMin<size_t>(imgSrc.bytesPerLine(), this->d->m_ic.lineSizeIp);

        for (int y = 0; y < image.height(); ++y) {
            auto srcLine = imgSrc.constScanLine(y);
            auto dstLine = this->d->m_ic.inputPacket.line(0, y);
            memcpy(dstLine, srcLine, lineSize);
        }
    }

    return this->d->convert(this->d->m_ic.inputPacket, this->d->m_outputCaps);
}

QImage AkVideoConverter::convertToImage(const AkVideoPacket &packet,
                                        QImage::Format format)
{
    if (!packet)
        return {};

    if (packet.caps() != this->d->m_ic.inputCaps
        || this->d->m_outputCaps != this->d->m_ic.outputCaps
        || format != this->d->m_ic.outputImageFormat) {
        this->d->m_ic.configure(packet.caps(),
                                this->d->m_outputCaps,
                                format,
                                this->d->m_aspectRatioMode);
        this->d->m_ic.inputCaps = packet.caps();
        this->d->m_ic.outputCaps = this->d->m_outputCaps;
    }

    if (!this->d->m_ic.canConvert)
        return {};

    if (this->d->m_ic.directConvertPi) {
        for (int y = 0; y < this->d->m_ic.outputImage.height(); ++y) {
            auto srcLine = packet.constLine(0, y);
            auto dstLine = this->d->m_ic.outputImage.scanLine(y);
            memcpy(dstLine, srcLine, this->d->m_ic.lineSizePi);
        }
    } else {
        auto src = this->d->convert(packet, this->d->m_ic.outputConvertCaps);

        for (int y = 0; y < this->d->m_ic.outputImage.height(); ++y) {
            auto srcLine = src.constLine(0, y);
            auto dstLine = this->d->m_ic.outputImage.scanLine(y);
            memcpy(dstLine, srcLine, this->d->m_ic.lineSizePi);
        }
    }

    return this->d->m_ic.outputImage;
}

QImage AkVideoConverter::convertToImage(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    if (packet.caps() != this->d->m_ic.inputCaps
        || this->d->m_outputCaps != this->d->m_ic.outputCaps) {
        this->d->m_ic.configure(packet.caps(),
                                this->d->m_outputCaps,
                                this->d->m_aspectRatioMode);
        this->d->m_ic.inputCaps = packet.caps();
        this->d->m_ic.outputCaps = this->d->m_outputCaps;
    }

    if (!this->d->m_ic.canConvert)
        return {};

    if (this->d->m_ic.directConvertPi) {
        for (int y = 0; y < this->d->m_ic.outputImage.height(); ++y) {
            auto srcLine = packet.constLine(0, y);
            auto dstLine = this->d->m_ic.outputImage.scanLine(y);

            memcpy(dstLine, srcLine, this->d->m_ic.lineSizePi);
        }
    } else {
        auto src = this->d->convert(packet, this->d->m_ic.outputConvertCaps);

        for (int y = 0; y < this->d->m_ic.outputImage.height(); ++y) {
            auto srcLine = src.constLine(0, y);
            auto dstLine = this->d->m_ic.outputImage.scanLine(y);
            memcpy(dstLine, srcLine, this->d->m_ic.lineSizePi);
        }
    }

    return this->d->m_ic.outputImage;
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

void AkVideoConverter::resetOutputCaps()
{
    this->setOutputCaps({});
}

void AkVideoConverter::resetScalingMode()
{
    this->setScalingMode(ScalingMode_Fast);
}

void AkVideoConverter::resetAspectRatioMode()
{
    this->setAspectRatioMode(AspectRatioMode_Ignore);
}

void AkVideoConverter::reset()
{
    this->d->m_fc.reset();
    this->d->m_ic.reset();
}

void AkVideoConverter::registerTypes()
{
    qRegisterMetaType<AkVideoConverter>("AkVideoConverter");
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
        this->convert<quint##isize, quint##osize>(packet, \
                                                  this->m_fc.outputFrame); \
        break;

AkVideoPacket AkVideoConverterPrivate::convert(const AkVideoPacket &packet,
                                               const AkVideoCaps &ocaps)
{
    if (packet.caps() != this->m_fc.inputCaps
        || ocaps != this->m_fc.outputCaps
        || this->m_scalingMode != this->m_fc.scalingMode
        || this->m_aspectRatioMode != this->m_fc.aspectRatioMode) {
        this->m_fc.setOutputConvertCaps(packet.caps(),
                                        ocaps,
                                        this->m_aspectRatioMode);
        this->m_fc.configure(packet.caps(),
                             this->m_fc.outputConvertCaps,
                             this->m_colorConvert);
        this->m_fc.configureScaling(packet.caps(),
                                    this->m_fc.outputConvertCaps,
                                    this->m_aspectRatioMode);
        this->m_fc.inputCaps = packet.caps();
        this->m_fc.outputCaps = ocaps;
        this->m_fc.scalingMode = this->m_scalingMode;
        this->m_fc.aspectRatioMode = this->m_aspectRatioMode;
    }

    if (this->m_fc.outputConvertCaps.isSameFormat(packet.caps()))
        return packet;

    switch (this->m_fc.convertDataTypes) {
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

    this->m_fc.outputFrame.copyMetadata(packet);

    return this->m_fc.outputFrame;
}

ColorConvert::ColorConvert()
{
}

ColorConvert::ColorConvert(YuvColorSpace colorSpace,
                           YuvColorSpaceType type):
    colorSpace(colorSpace),
    type(type)
{
}

ColorConvert::ColorConvert(YuvColorSpaceType type):
    type(type)
{
}

void ColorConvert::applyMatrix(qint64 a, qint64 b, qint64 c,
                               qint64 *x, qint64 *y, qint64 *z) const
{
    *x = qBound(this->xmin, (a * this->m00 + b * this->m01 + c * this->m02 + this->m03) >> this->shift, this->xmax);
    *y = qBound(this->ymin, (a * this->m10 + b * this->m11 + c * this->m12 + this->m13) >> this->shift, this->ymax);
    *z = qBound(this->zmin, (a * this->m20 + b * this->m21 + c * this->m22 + this->m23) >> this->shift, this->zmax);
}

void ColorConvert::applyVector(qint64 a, qint64 b, qint64 c,
                               qint64 *x, qint64 *y, qint64 *z) const
{
    *x = (a * this->m00 + this->m03) >> this->shift;
    *y = (b * this->m11 + this->m13) >> this->shift;
    *z = (c * this->m22 + this->m23) >> this->shift;
}

void ColorConvert::applyPoint(qint64 p,
                              qint64 *x, qint64 *y, qint64 *z) const
{
    *x = (p * this->m00 + this->m03) >> this->shift;
    *y = (p * this->m11 + this->m13) >> this->shift;
    *z = (p * this->m22 + this->m23) >> this->shift;
}

void ColorConvert::applyPoint(qint64 a, qint64 b, qint64 c,
                              qint64 *p) const
{
    *p = (a * this->m00 + b * this->m01 + c * this->m02 + this->m03) >> this->shift;
}

void ColorConvert::applyPoint(qint64 p, qint64 *q) const
{
    *q = (p * this->m00 + this->m03) >> this->shift;
}

void ColorConvert::applyAlpha(qint64 x, qint64 y, qint64 z, qint64 a,
                              qint64 *xa, qint64 *ya, qint64 *za) const
{
    qint64 diffA = this->amax - a;
    *xa = (x * a + this->xmin * diffA) / this->amax;
    *ya = (y * a + this->ymin * diffA) / this->amax;
    *za = (z * a + this->zmin * diffA) / this->amax;
}

void ColorConvert::applyAlpha(qint64 a,
                              qint64 *x, qint64 *y, qint64 *z) const
{
    this->applyAlpha(*x, *y, *z, a, x, y, z);
}

void ColorConvert::applyAlpha(qint64 p, qint64 a, qint64 *pa) const
{
    *pa = (p * a + this->xmin * (this->amax - a)) / this->amax;
}

void ColorConvert::applyAlpha(qint64 a, qint64 *p) const
{
    this->applyAlpha(*p, a, p);
}

void ColorConvert::setAlphaBits(int abits)
{
    this->amax = (1L << abits) - 1;
}

void ColorConvert::loadMatrix(ColorMatrix colorMatrix,
                              int ibitsa,
                              int ibitsb,
                              int ibitsc,
                              int obitsx,
                              int obitsy,
                              int obitsz)
{
    switch (colorMatrix) {
    case ColorMatrix_ABC2XYZ:
        this->loadAbc2xyzMatrix(ibitsa,
                                ibitsb,
                                ibitsc,
                                obitsx,
                                obitsy,
                                obitsz);

        break;

    case ColorMatrix_RGB2YUV:
        this->loadRgb2yuvMatrix(this->colorSpace,
                                this->type,
                                ibitsa,
                                ibitsb,
                                ibitsc,
                                obitsx,
                                obitsy,
                                obitsz);

        break;

    case ColorMatrix_YUV2RGB:
        this->loadYuv2rgbMatrix(this->colorSpace,
                                this->type,
                                ibitsa,
                                ibitsb,
                                ibitsc,
                                obitsx,
                                obitsy,
                                obitsz);

        break;

    case ColorMatrix_RGB2GRAY:
        this->loadRgb2grayMatrix(this->colorSpace,
                                 ibitsa,
                                 ibitsb,
                                 ibitsc,
                                 obitsx);

        break;

    case ColorMatrix_GRAY2RGB:
        this->loadGray2rgbMatrix(ibitsa,
                                 obitsx,
                                 obitsy,
                                 obitsz);

        break;

    case ColorMatrix_YUV2GRAY:
        this->loadYuv2grayMatrix(this->type,
                                 ibitsa,
                                 ibitsb,
                                 ibitsc,
                                 obitsx);

        break;

    case ColorMatrix_GRAY2YUV:
        this->loadGray2yuvMatrix(this->type,
                                 ibitsa,
                                 obitsx,
                                 obitsy,
                                 obitsz);


    default:
        break;
    }
}

void ColorConvert::loadMatrix(const AkVideoFormatSpec &from,
                              const AkVideoFormatSpec &to)
{
    ColorMatrix colorMatrix = ColorMatrix_ABC2XYZ;
    int ibitsa = 0;
    int ibitsb = 0;
    int ibitsc = 0;
    int obitsx = 0;
    int obitsy = 0;
    int obitsz = 0;

    if (from.type() == AkVideoFormatSpec::VFT_RGB
        && to.type() == AkVideoFormatSpec::VFT_RGB) {
        colorMatrix = ColorMatrix_ABC2XYZ;
        ibitsa = from.component(AkColorComponent::CT_R).length();
        ibitsb = from.component(AkColorComponent::CT_G).length();
        ibitsc = from.component(AkColorComponent::CT_B).length();
        obitsx = to.component(AkColorComponent::CT_R).length();
        obitsy = to.component(AkColorComponent::CT_G).length();
        obitsz = to.component(AkColorComponent::CT_B).length();
    } else if (from.type() == AkVideoFormatSpec::VFT_RGB
               && to.type() == AkVideoFormatSpec::VFT_YUV) {
        colorMatrix = ColorMatrix_RGB2YUV;
        ibitsa = from.component(AkColorComponent::CT_R).length();
        ibitsb = from.component(AkColorComponent::CT_G).length();
        ibitsc = from.component(AkColorComponent::CT_B).length();
        obitsx = to.component(AkColorComponent::CT_Y).length();
        obitsy = to.component(AkColorComponent::CT_U).length();
        obitsz = to.component(AkColorComponent::CT_V).length();
    } else if (from.type() == AkVideoFormatSpec::VFT_RGB
               && to.type() == AkVideoFormatSpec::VFT_Gray) {
        colorMatrix = ColorMatrix_RGB2GRAY;
        ibitsa = from.component(AkColorComponent::CT_R).length();
        ibitsb = from.component(AkColorComponent::CT_G).length();
        ibitsc = from.component(AkColorComponent::CT_B).length();
        obitsx = to.component(AkColorComponent::CT_Y).length();
        obitsy = obitsx;
        obitsz = obitsx;
    } else if (from.type() == AkVideoFormatSpec::VFT_YUV
               && to.type() == AkVideoFormatSpec::VFT_RGB) {
        colorMatrix = ColorMatrix_YUV2RGB;
        ibitsa = from.component(AkColorComponent::CT_Y).length();
        ibitsb = from.component(AkColorComponent::CT_U).length();
        ibitsc = from.component(AkColorComponent::CT_V).length();
        obitsx = to.component(AkColorComponent::CT_R).length();
        obitsy = to.component(AkColorComponent::CT_G).length();
        obitsz = to.component(AkColorComponent::CT_B).length();
    } else if (from.type() == AkVideoFormatSpec::VFT_YUV
               && to.type() == AkVideoFormatSpec::VFT_YUV) {
        colorMatrix = ColorMatrix_ABC2XYZ;
        ibitsa = from.component(AkColorComponent::CT_Y).length();
        ibitsb = from.component(AkColorComponent::CT_U).length();
        ibitsc = from.component(AkColorComponent::CT_V).length();
        obitsx = to.component(AkColorComponent::CT_Y).length();
        obitsy = to.component(AkColorComponent::CT_U).length();
        obitsz = to.component(AkColorComponent::CT_V).length();
    } else if (from.type() == AkVideoFormatSpec::VFT_YUV
               && to.type() == AkVideoFormatSpec::VFT_Gray) {
        colorMatrix = ColorMatrix_YUV2GRAY;
        ibitsa = from.component(AkColorComponent::CT_Y).length();
        ibitsb = from.component(AkColorComponent::CT_U).length();
        ibitsc = from.component(AkColorComponent::CT_V).length();
        obitsx = to.component(AkColorComponent::CT_Y).length();
        obitsy = obitsx;
        obitsz = obitsx;
    } else if (from.type() == AkVideoFormatSpec::VFT_Gray
               && to.type() == AkVideoFormatSpec::VFT_RGB) {
        colorMatrix = ColorMatrix_GRAY2RGB;
        ibitsa = from.component(AkColorComponent::CT_Y).length();
        ibitsb = ibitsa;
        ibitsc = ibitsa;
        obitsx = to.component(AkColorComponent::CT_R).length();
        obitsy = to.component(AkColorComponent::CT_G).length();
        obitsz = to.component(AkColorComponent::CT_B).length();
    } else if (from.type() == AkVideoFormatSpec::VFT_Gray
               && to.type() == AkVideoFormatSpec::VFT_YUV) {
        colorMatrix = ColorMatrix_GRAY2YUV;
        ibitsa = from.component(AkColorComponent::CT_Y).length();
        ibitsb = ibitsa;
        ibitsc = ibitsa;
        obitsx = to.component(AkColorComponent::CT_Y).length();
        obitsy = to.component(AkColorComponent::CT_U).length();
        obitsz = to.component(AkColorComponent::CT_V).length();
    } else if (from.type() == AkVideoFormatSpec::VFT_Gray
               && to.type() == AkVideoFormatSpec::VFT_Gray) {
        colorMatrix = ColorMatrix_ABC2XYZ;
        ibitsa = from.component(AkColorComponent::CT_Y).length();
        ibitsb = ibitsa;
        ibitsc = ibitsa;
        obitsx = to.component(AkColorComponent::CT_Y).length();
        obitsy = obitsx;
        obitsz = obitsx;
    }

    if (from.contains(AkColorComponent::CT_A))
        this->setAlphaBits(from.component(AkColorComponent::CT_A).length());

    this->loadMatrix(colorMatrix,
                     ibitsa,
                     ibitsb,
                     ibitsc,
                     obitsx,
                     obitsy,
                     obitsz);
}

void ColorConvert::loadMatrix(const AkVideoCaps::PixelFormat &from,
                              const AkVideoCaps::PixelFormat &to)
{
    this->loadMatrix(AkVideoCaps::formatSpecs(from),
                     AkVideoCaps::formatSpecs(to));
}

void ColorConvert::rbConstants(YuvColorSpace colorSpace,
                               qint64 &kr, qint64 &kb, qint64 &div) const
{
    kr = 0;
    kb = 0;
    div = 10000;

    // Coefficients taken from https://en.wikipedia.org/wiki/YUV
    switch (colorSpace) {
    // Same weight for all components
    case YuvColorSpace_AVG:
        kr = 3333;
        kb = 3333;

        break;

    // https://www.itu.int/rec/R-REC-BT.601/en
    case YuvColorSpace_ITUR_BT601:
        kr = 2990;
        kb = 1140;

        break;

    // https://www.itu.int/rec/R-REC-BT.709/en
    case YuvColorSpace_ITUR_BT709:
        kr = 2126;
        kb = 722;

        break;

    // https://www.itu.int/rec/R-REC-BT.2020/en
    case YuvColorSpace_ITUR_BT2020:
        kr = 2627;
        kb = 593;

        break;

    // http://car.france3.mars.free.fr/HD/INA-%2026%20jan%2006/SMPTE%20normes%20et%20confs/s240m.pdf
    case YuvColorSpace_SMPTE_240M:
        kr = 2120;
        kb = 870;

        break;

    default:
        break;
    }
}

qint64 ColorConvert::roundedDiv(qint64 num, qint64 den) const
{
    if ((num < 0 && den > 0) || (num > 0 && den < 0))
        return (2 * num - den) / (2 * den);

    return (2 * num + den) / (2 * den);
}

qint64 ColorConvert::nearestPowOf2(qint64 value) const
{
    qint64 val = value;
    qint64 res = 0;

    while (val >>= 1)
        res++;

    if (abs((1 << (res + 1)) - value) <= abs((1 << res) - value))
        return 1 << (res + 1);

    return 1 << res;
}

void ColorConvert::limitsY(int bits,
                           YuvColorSpaceType type,
                           qint64 &minY,
                           qint64 &maxY) const
{
    if (type == YuvColorSpaceType_FullSwing) {
        minY = 0;
        maxY = (1 << bits) - 1;

        return;
    }

    /* g = 9% is the theoretical maximal overshoot (Gibbs phenomenon)
     *
     * https://en.wikipedia.org/wiki/YUV#Numerical_approximations
     * https://en.wikipedia.org/wiki/Gibbs_phenomenon
     * https://math.stackexchange.com/a/259089
     * https://www.youtube.com/watch?v=Ol0uTeXoKaU
     */
    static const qint64 g = 9;

    qint64 maxValue = (1 << bits) - 1;
    minY = this->nearestPowOf2(roundedDiv(maxValue * g, 2 * g + 100));
    maxY = maxValue * (g + 100) / (2 * g + 100);
}

void ColorConvert::limitsUV(int bits,
                            YuvColorSpaceType type,
                            qint64 &minUV,
                            qint64 &maxUV) const
{
    if (type == YuvColorSpaceType_FullSwing) {
        minUV = 0;
        maxUV = (1 << bits) - 1;

        return;
    }

    static const qint64 g = 9;
    qint64 maxValue = (1 << bits) - 1;
    minUV = this->nearestPowOf2(roundedDiv(maxValue * g, 2 * g + 100));
    maxUV = (1L << bits) - minUV;
}

void ColorConvert::loadAbc2xyzMatrix(int abits,
                                     int bbits,
                                     int cbits,
                                     int xbits,
                                     int ybits,
                                     int zbits)
{
    int shift = std::max(abits, std::max(bbits, cbits));
    qint64 shiftDiv = 1L << shift;
    qint64 rounding = 1L << qAbs(shift - 1);

    qint64 amax = (1L << abits) - 1;
    qint64 bmax = (1L << bbits) - 1;
    qint64 cmax = (1L << cbits) - 1;

    qint64 xmax = (1L << xbits) - 1;
    qint64 ymax = (1L << ybits) - 1;
    qint64 zmax = (1L << zbits) - 1;

    qint64 kx = this->roundedDiv(shiftDiv * xmax, amax);
    qint64 ky = this->roundedDiv(shiftDiv * ymax, bmax);
    qint64 kz = this->roundedDiv(shiftDiv * zmax, cmax);

    this->m00 = kx; this->m01 = 0 ; this->m02 = 0 ; this->m03 = rounding;
    this->m10 = 0 ; this->m11 = ky; this->m12 = 0 ; this->m13 = rounding;
    this->m20 = 0 ; this->m21 = 0 ; this->m22 = kz; this->m23 = rounding;

    this->xmin = 0; this->xmax = xmax;
    this->ymin = 0; this->ymax = ymax;
    this->zmin = 0; this->zmax = zmax;

    this->shift = shift;
}

void ColorConvert::loadRgb2yuvMatrix(YuvColorSpace colorSpace,
                                     YuvColorSpaceType type,
                                     int rbits,
                                     int gbits,
                                     int bbits,
                                     int ybits,
                                     int ubits,
                                     int vbits)
{
    qint64 kyr = 0;
    qint64 kyb = 0;
    qint64 div = 0;
    this->rbConstants(colorSpace, kyr, kyb, div);
    qint64 kyg = div - kyr - kyb;

    qint64 kur = -kyr;
    qint64 kug = -kyg;
    qint64 kub = div - kyb;

    qint64 kvr = div - kyr;
    qint64 kvg = -kyg;
    qint64 kvb = -kyb;

    int shift = std::max(rbits, std::max(gbits, bbits));
    qint64 shiftDiv = 1L << shift;
    qint64 rounding = 1L << (shift - 1);

    qint64 rmax = (1L << rbits) - 1;
    qint64 gmax = (1L << gbits) - 1;
    qint64 bmax = (1L << bbits) - 1;

    qint64 minY = 0;
    qint64 maxY = 0;
    this->limitsY(ybits, type, minY, maxY);
    auto diffY = maxY - minY;

    qint64 kiyr = this->roundedDiv(shiftDiv * diffY * kyr, div * rmax);
    qint64 kiyg = this->roundedDiv(shiftDiv * diffY * kyg, div * gmax);
    qint64 kiyb = this->roundedDiv(shiftDiv * diffY * kyb, div * bmax);

    qint64 minU = 0;
    qint64 maxU = 0;
    this->limitsUV(ubits, type, minU, maxU);
    auto diffU = maxU - minU;

    qint64 kiur = this->roundedDiv(shiftDiv * diffU * kur, 2 * rmax * kub);
    qint64 kiug = this->roundedDiv(shiftDiv * diffU * kug, 2 * gmax * kub);
    qint64 kiub = this->roundedDiv(shiftDiv * diffU      , 2 * bmax);

    qint64 minV = 0;
    qint64 maxV = 0;
    this->limitsUV(vbits, type, minV, maxV);
    auto diffV = maxV - minV;

    qint64 kivr = this->roundedDiv(shiftDiv * diffV      , 2 * rmax);
    qint64 kivg = this->roundedDiv(shiftDiv * diffV * kvg, 2 * gmax * kvr);
    qint64 kivb = this->roundedDiv(shiftDiv * diffV * kvb, 2 * bmax * kvr);

    qint64 ciy = rounding + shiftDiv * minY;
    qint64 ciu = rounding + shiftDiv * (minU + maxU) / 2;
    qint64 civ = rounding + shiftDiv * (minV + maxV) / 2;

    this->m00 = kiyr; this->m01 = kiyg; this->m02 = kiyb; this->m03 = ciy;
    this->m10 = kiur; this->m11 = kiug; this->m12 = kiub; this->m13 = ciu;
    this->m20 = kivr; this->m21 = kivg; this->m22 = kivb; this->m23 = civ;

    this->xmin = minY; this->xmax = maxY;
    this->ymin = minU; this->ymax = maxU;
    this->zmin = minV; this->zmax = maxV;

    this->shift = shift;
}

void ColorConvert::loadYuv2rgbMatrix(YuvColorSpace colorSpace,
                                     YuvColorSpaceType type,
                                     int ybits,
                                     int ubits,
                                     int vbits,
                                     int rbits,
                                     int gbits,
                                     int bbits)
{
    qint64 kyr = 0;
    qint64 kyb = 0;
    qint64 div = 0;
    this->rbConstants(colorSpace, kyr, kyb, div);
    qint64 kyg = div - kyr - kyb;

    qint64 minY = 0;
    qint64 maxY = 0;
    this->limitsY(ybits, type, minY, maxY);
    auto diffY = maxY - minY;

    qint64 minU = 0;
    qint64 maxU = 0;
    this->limitsUV(ubits, type, minU, maxU);
    auto diffU = maxU - minU;

    qint64 minV = 0;
    qint64 maxV = 0;
    this->limitsUV(vbits, type, minV, maxV);
    auto diffV = maxV - minV;

    int shift = std::max(ybits, std::max(ubits, vbits));
    qint64 shiftDiv = 1L << shift;
    qint64 rounding = 1L << (shift - 1);

    qint64 rmax = (1L << rbits) - 1;
    qint64 gmax = (1L << gbits) - 1;
    qint64 bmax = (1L << bbits) - 1;

    qint64 kry = this->roundedDiv(shiftDiv * rmax, diffY);
    qint64 krv = this->roundedDiv(2 * shiftDiv * rmax * (div - kyr), div * diffV);

    qint64 kgy = this->roundedDiv(shiftDiv * gmax, diffY);
    qint64 kgu = this->roundedDiv(2 * shiftDiv * gmax * kyb * (kyb - div), div * kyg * diffU);
    qint64 kgv = this->roundedDiv(2 * shiftDiv * gmax * kyr * (kyr - div), div * kyg * diffV);

    qint64 kby = this->roundedDiv(shiftDiv * bmax, diffY);
    qint64 kbu = this->roundedDiv(2 * shiftDiv * bmax * (div - kyb), div * diffU);

    qint64 cir = rounding - kry * minY - krv * (minV + maxV) / 2;
    qint64 cig = rounding - kgy * minY - (kgu * (minU + maxU) + kgv * (minV + maxV)) / 2;
    qint64 cib = rounding - kby * minY - kbu * (minU + maxU) / 2;

    this->m00 = kry; this->m01 = 0  ; this->m02 = krv; this->m03 = cir;
    this->m10 = kgy; this->m11 = kgu; this->m12 = kgv; this->m13 = cig;
    this->m20 = kby; this->m21 = kbu; this->m22 = 0  ; this->m23 = cib;

    this->xmin = 0; this->xmax = rmax;
    this->ymin = 0; this->ymax = gmax;
    this->zmin = 0; this->zmax = bmax;

    this->shift = shift;
}

void ColorConvert::loadRgb2grayMatrix(YuvColorSpace colorSpace,
                                      int rbits,
                                      int gbits,
                                      int bbits,
                                      int graybits)
{
    YuvColorSpaceType type = YuvColorSpaceType_FullSwing;

    qint64 kyr = 0;
    qint64 kyb = 0;
    qint64 div = 0;
    this->rbConstants(colorSpace, kyr, kyb, div);
    qint64 kyg = div - kyr - kyb;

    int shift = std::max(rbits, std::max(gbits, bbits));
    qint64 shiftDiv = 1L << shift;
    qint64 rounding = 1L << (shift - 1);

    qint64 rmax = (1L << rbits) - 1;
    qint64 gmax = (1L << gbits) - 1;
    qint64 bmax = (1L << bbits) - 1;

    qint64 minY = 0;
    qint64 maxY = 0;
    this->limitsY(graybits, type, minY, maxY);
    auto diffY = maxY - minY;

    qint64 kiyr = this->roundedDiv(shiftDiv * diffY * kyr, div * rmax);
    qint64 kiyg = this->roundedDiv(shiftDiv * diffY * kyg, div * gmax);
    qint64 kiyb = this->roundedDiv(shiftDiv * diffY * kyb, div * bmax);

    qint64 minU = 0;
    qint64 maxU = 0;
    this->limitsUV(graybits, type, minU, maxU);

    qint64 minV = 0;
    qint64 maxV = 0;
    this->limitsUV(graybits, type, minV, maxV);

    qint64 ciy = rounding + shiftDiv * minY;
    qint64 ciu = rounding + shiftDiv * (minU + maxU) / 2;
    qint64 civ = rounding + shiftDiv * (minV + maxV) / 2;

    this->m00 = kiyr; this->m01 = kiyg; this->m02 = kiyb; this->m03 = ciy;
    this->m10 = 0   ; this->m11 = 0   ; this->m12 = 0   ; this->m13 = ciu;
    this->m20 = 0   ; this->m21 = 0   ; this->m22 = 0   ; this->m23 = civ;

    this->xmin = minY; this->xmax = maxY;
    this->ymin = minU; this->ymax = maxU;
    this->zmin = minV; this->zmax = maxV;

    this->shift = shift;
}

void ColorConvert::loadGray2rgbMatrix(int graybits,
                                      int rbits,
                                      int gbits,
                                      int bbits)
{
    int shift = graybits;
    qint64 shiftDiv = 1L << shift;
    qint64 rounding = 1L << (shift - 1);

    qint64 graymax = (1L << graybits) - 1;

    qint64 rmax = (1L << rbits) - 1;
    qint64 gmax = (1L << gbits) - 1;
    qint64 bmax = (1L << bbits) - 1;

    qint64 kr = this->roundedDiv(shiftDiv * rmax, graymax);
    qint64 kg = this->roundedDiv(shiftDiv * gmax, graymax);
    qint64 kb = this->roundedDiv(shiftDiv * bmax, graymax);

    this->m00 = kr; this->m01 = 0; this->m02 = 0; this->m03 = rounding;
    this->m10 = kg; this->m11 = 0; this->m12 = 0; this->m13 = rounding;
    this->m20 = kb; this->m21 = 0; this->m22 = 0; this->m23 = rounding;

    this->xmin = 0; this->xmax = rmax;
    this->ymin = 0; this->ymax = gmax;
    this->zmin = 0; this->zmax = bmax;

    this->shift = shift;
}

void ColorConvert::loadYuv2grayMatrix(YuvColorSpaceType type,
                                      int ybits,
                                      int ubits,
                                      int vbits,
                                      int graybits)
{
    YuvColorSpaceType otype = YuvColorSpaceType_FullSwing;

    int shift = ybits;
    qint64 shiftDiv = 1L << shift;
    qint64 rounding = 1L << (shift - 1);

    qint64 graymax = (1L << graybits) - 1;

    qint64 minY = 0;
    qint64 maxY = 0;
    this->limitsY(ybits, type, minY, maxY);
    auto diffY = maxY - minY;

    qint64 ky = this->roundedDiv(shiftDiv * graymax, diffY);

    qint64 minU = 0;
    qint64 maxU = 0;
    this->limitsUV(graybits, otype, minU, maxU);

    qint64 minV = 0;
    qint64 maxV = 0;
    this->limitsUV(graybits, otype, minV, maxV);

    qint64 ciy = rounding - this->roundedDiv(shiftDiv * minY * graymax, diffY);
    qint64 ciu = rounding + shiftDiv * (minU + maxU) / 2;
    qint64 civ = rounding + shiftDiv * (minV + maxV) / 2;

    this->m00 = ky; this->m01 = 0; this->m02 = 0; this->m03 = ciy;
    this->m10 = 0 ; this->m11 = 0; this->m12 = 0; this->m13 = ciu;
    this->m20 = 0 ; this->m21 = 0; this->m22 = 0; this->m23 = civ;

    this->xmin = 0; this->xmax = graymax;
    this->ymin = 0; this->ymax = graymax;
    this->zmin = 0; this->zmax = graymax;

    this->shift = shift;
}

void ColorConvert::loadGray2yuvMatrix(YuvColorSpaceType type,
                                      int graybits,
                                      int ybits,
                                      int ubits,
                                      int vbits)
{
    int shift = graybits;
    qint64 shiftDiv = 1L << shift;
    qint64 rounding = 1L << (shift - 1);

    qint64 graymax = (1L << graybits) - 1;

    qint64 minY = 0;
    qint64 maxY = 0;
    this->limitsY(ybits, type, minY, maxY);
    auto diffY = maxY - minY;

    qint64 ky = this->roundedDiv(shiftDiv * diffY, graymax);

    qint64 minU = 0;
    qint64 maxU = 0;
    this->limitsUV(ubits, type, minU, maxU);

    qint64 minV = 0;
    qint64 maxV = 0;
    this->limitsUV(ybits, type, minV, maxV);

    qint64 ciy = rounding + shiftDiv * minY;
    qint64 ciu = rounding + shiftDiv * (minU + maxU) / 2;
    qint64 civ = rounding + shiftDiv * (minV + maxV) / 2;

    this->m00 = ky; this->m01 = 0; this->m02 = 0; this->m03 = ciy;
    this->m10 = 0 ; this->m11 = 0; this->m12 = 0; this->m13 = ciu;
    this->m20 = 0 ; this->m21 = 0; this->m22 = 0; this->m23 = civ;

    this->xmin = minY; this->xmax = maxY;
    this->ymin = minU; this->ymax = maxU;
    this->zmin = minV; this->zmax = maxV;

    this->shift = shift;
}

FrameConvertParameters::~FrameConvertParameters()
{
    this->clearBuffers();
    this->clearDlBuffers();
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
    int inputHeight_1 = this->inputHeight + 1;

    if (this->integralImageDataX) {
        for (int y = 0; y < inputHeight_1; ++y)
            delete [] this->integralImageDataX[y];

        delete [] this->integralImageDataX;
        this->integralImageDataX = nullptr;
    }

    if (this->integralImageDataY) {
        for (int y = 0; y < inputHeight_1; ++y)
            delete [] this->integralImageDataY[y];

        delete [] this->integralImageDataY;
        this->integralImageDataY = nullptr;
    }

    if (this->integralImageDataZ) {
        for (int y = 0; y < inputHeight_1; ++y)
            delete [] this->integralImageDataZ[y];

        delete [] this->integralImageDataZ;
        this->integralImageDataZ = nullptr;
    }

    if (this->kdl) {
        for (int y = 0; y < this->inputHeight; ++y)
            delete [] this->kdl[y];

        delete [] this->kdl;
        this->kdl = nullptr;
    }

    if (this->integralImageDataA) {
        for (int y = 0; y < this->inputHeight; ++y)
            delete [] this->integralImageDataA[y];

        delete [] this->integralImageDataA;
        this->integralImageDataA = nullptr;
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

void FrameConvertParameters::allocateBuffers(int width, int height)
{
    this->clearBuffers();

    this->srcWidth = new int [width];
    this->srcWidth_1 = new int [width];
    this->srcWidthOffsetX = new int [width];
    this->srcWidthOffsetY = new int [width];
    this->srcWidthOffsetZ = new int [width];
    this->srcWidthOffsetA = new int [width];
    this->srcHeight = new int [height];

    this->srcWidthOffsetX_1 = new int [width];
    this->srcWidthOffsetY_1 = new int [width];
    this->srcWidthOffsetZ_1 = new int [width];
    this->srcWidthOffsetA_1 = new int [width];
    this->srcHeight_1 = new int [height];

    this->dstWidthOffsetX = new int [width];
    this->dstWidthOffsetY = new int [width];
    this->dstWidthOffsetZ = new int [width];
    this->dstWidthOffsetA = new int [width];

    this->kx = new qint64 [width];
    this->ky = new qint64 [height];
}

void FrameConvertParameters::allocateDlBuffers(int width, int height)
{
    int height_1 = height + 1;

    this->integralImageDataX = new DlSumType *[height_1];
    this->integralImageDataY = new DlSumType *[height_1];
    this->integralImageDataZ = new DlSumType *[height_1];
    this->integralImageDataA = new DlSumType *[height_1];
    this->kdl = new DlSumType *[height];

    int width_1 = width + 1;
    auto lineSize = sizeof(DlSumType) * width_1;

    for (int y = 0; y < height_1; ++y) {
        this->integralImageDataX[y] = new DlSumType [width_1];
        this->integralImageDataY[y] = new DlSumType [width_1];
        this->integralImageDataZ[y] = new DlSumType [width_1];
        this->integralImageDataA[y] = new DlSumType [width_1];
        this->kdl[y] = new DlSumType [width];

        memset(this->integralImageDataX[y], 0, lineSize);
        memset(this->integralImageDataY[y], 0, lineSize);
        memset(this->integralImageDataZ[y], 0, lineSize);
        memset(this->integralImageDataA[y], 0, lineSize);
    }

    this->dlSrcWidthOffsetX = new int [width];
    this->dlSrcWidthOffsetY = new int [width];
    this->dlSrcWidthOffsetZ = new int [width];
    this->dlSrcWidthOffsetA = new int [width];
}

void FrameConvertParameters::setOutputConvertCaps(const AkVideoCaps &icaps,
                                                  const AkVideoCaps &ocaps,
                                                  AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    this->outputConvertCaps = ocaps;

    if (this->outputConvertCaps.format() == AkVideoCaps::Format_none)
        this->outputConvertCaps.setFormat(icaps.format());

    int width = this->outputConvertCaps.width() > 1?
                    this->outputConvertCaps.width():
                    icaps.width();
    int height = this->outputConvertCaps.height() > 1?
                     this->outputConvertCaps.height():
                     icaps.height();

    if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Keep) {
        auto w = height * icaps.width() / icaps.height();
        auto h = width * icaps.height() / icaps.width();

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
}

#define DEFINE_CONVERT_TYPES(isize, osize) \
    if (ispecs.byteLength() == (isize / 8) && ospecs.byteLength() == (osize / 8)) \
        this->convertDataTypes = ConvertDataTypes_##isize##_##osize;

void FrameConvertParameters::configure(const AkVideoCaps &icaps,
                                       const AkVideoCaps &ocaps,
                                       ColorConvert &colorConvert)
{
    this->outputFrame = {ocaps};

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
                                              AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    if (ocaps.width() > icaps.width() || ocaps.height() > icaps.height())
        this->resizeMode = ResizeMode_Up;
    else if (ocaps.width() < icaps.width() || ocaps.height() < icaps.height())
        this->resizeMode = ResizeMode_Down;
    else
        this->resizeMode = ResizeMode_Keep;

    QRect inputRect;

    if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Expanding) {
        auto w = icaps.height() * ocaps.width() / ocaps.height();
        auto h = icaps.width() * ocaps.height() / ocaps.width();

        if (w > icaps.width())
            w = icaps.width();
        else if (h > icaps.height())
            h = icaps.height();

        auto x = (icaps.width() - w) / 2;
        auto y = (icaps.height() - h) / 2;

        inputRect = {x, y, w, h};
    } else {
        inputRect = {0, 0, icaps.width(), icaps.height()};
    }

    this->allocateBuffers(ocaps.width(), ocaps.height());

    int wi_1 = inputRect.width() - 1;
    int wo_1 = ocaps.width() - 1;

    auto xSrcToDst = [&inputRect, &wi_1, &wo_1] (int x) -> int {
        return (x - inputRect.x()) * wo_1 / wi_1;
    };

    auto xDstToSrc = [&inputRect, &wi_1, &wo_1] (int x) -> int {
        return (x * wi_1 + inputRect.x() * wo_1) / wo_1;
    };

    for (int x = 0; x < ocaps.width(); ++x) {
        auto xs = xDstToSrc(x);
        auto xs_1 = xDstToSrc(qMin(x + 1, ocaps.width() - 1));
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

    int hi_1 = inputRect.height() - 1;
    int ho_1 = ocaps.height() - 1;

    auto ySrcToDst = [&inputRect, &hi_1, &ho_1] (int y) -> int {
        return (y - inputRect.y()) * ho_1 / hi_1;
    };

    auto yDstToSrc = [&inputRect, &hi_1, &ho_1] (int y) -> int {
        return (y * hi_1 + inputRect.y() * ho_1) / ho_1;
    };

    for (int y = 0; y < ocaps.height(); ++y) {
        if (this->resizeMode == ResizeMode_Down) {
            this->srcHeight[y] = yDstToSrc(y);
            this->srcHeight_1[y] = qMin(yDstToSrc(y + 1), icaps.height());
        } else {
            auto ys = yDstToSrc(y);
            auto ys_1 = yDstToSrc(qMin(y + 1, ocaps.height() - 1));
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
    this->inputHeight = icaps.height();
    this->outputWidth = ocaps.width();
    this->outputHeight = ocaps.height();

    this->clearDlBuffers();

    if (this->resizeMode == ResizeMode_Down) {
        this->allocateDlBuffers(icaps.width(), icaps.height());

        for (int x = 0; x < icaps.width(); ++x) {
            this->dlSrcWidthOffsetX[x] = (x >> this->compXi.widthDiv()) * this->compXi.step();
            this->dlSrcWidthOffsetY[x] = (x >> this->compYi.widthDiv()) * this->compYi.step();
            this->dlSrcWidthOffsetZ[x] = (x >> this->compZi.widthDiv()) * this->compZi.step();
            this->dlSrcWidthOffsetA[x] = (x >> this->compAi.widthDiv()) * this->compAi.step();
        }

        for (int y = 0; y < this->outputHeight; ++y) {
            auto diffY = this->srcHeight_1[y] - this->srcHeight[y];
            auto line = this->kdl[y];

            for (int x = 0; x < this->outputWidth; ++x) {
                auto diffX = this->srcWidth_1[x] - this->srcWidth[x];
                line[x] = diffX * diffY;
            }
        }
    }
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

size_t ImageConvertParameters::lineSize(const AkVideoCaps &caps) const
{
    static const size_t align = 32;
    auto specs = AkVideoCaps::formatSpecs(caps.format());

    return this->alignUp(specs.plane(0).bitsSize()
                         * caps.width()
                         / 8,
                         align);
}

void ImageConvertParameters::configure(const QImage &image,
                                                      const AkVideoCaps &caps)
{
    this->directConvertIp = imageToAkFormat->contains(image.format());
    this->inputPacket =
        {{imageToAkFormat->value(image.format(), AkVideoCaps::Format_argbpack),
          image.width(),
          image.height(),
          caps.fps()}};

    if (this->directConvertIp)
        this->lineSizeIp =
                qMin<size_t>(image.bytesPerLine(),
                             this->inputPacket.lineSize(0));
    else
        this->lineSizeIp = this->inputPacket.lineSize(0);
}

void ImageConvertParameters::configure(const AkVideoCaps &icaps,
                                                      const AkVideoCaps &ocaps,
                                                      QImage::Format format,
                                                      AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    this->canConvert = icaps && imageToAkFormat->contains(format);
    auto outputPacketFormat = imageToAkFormat->value(format);
    auto _ocaps = ocaps;
    _ocaps.setFormat(outputPacketFormat);

    int width = _ocaps.width() > 1?
                    _ocaps.width():
                    icaps.width();
    int height = _ocaps.height() > 1?
                     _ocaps.height():
                     icaps.height();

    _ocaps.setWidth(width);
    _ocaps.setHeight(height);
    _ocaps.setFps(icaps.fps());
    this->outputConvertCaps = _ocaps;

    if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Keep) {
        auto w = height * icaps.width() / icaps.height();
        auto h = width * icaps.height() / icaps.width();

        if (w > width)
            w = width;
        else if (h > height)
            h = height;

        width = w;
        height = h;
    }

    this->directConvertPi = icaps.format() == _ocaps.format()
                            && icaps.width() == _ocaps.width()
                            && icaps.height() == _ocaps.height();
    this->outputImage = {width, height, format};
    this->outputImageFormat = format;
    this->lineSizePi = qMin<size_t>(this->lineSize(_ocaps),
                                    this->outputImage.bytesPerLine());

    if (this->outputImage.format() == QImage::Format_Indexed8)
        for (int i = 0; i < 256; i++)
            this->outputImage.setColor(i, QRgb(i));
}

void ImageConvertParameters::configure(const AkVideoCaps &icaps,
                                       const AkVideoCaps &ocaps,
                                       AkVideoConverter::AspectRatioMode aspectRatioMode)
{
    this->canConvert = icaps;
    auto outputPacketFormat = imageToAkFormat->values().contains(icaps.format())?
                                  icaps.format():
                                  AkVideoCaps::Format_argbpack;
    auto _ocaps = ocaps;
    _ocaps.setFormat(outputPacketFormat);

    int width = _ocaps.width() > 1?
                    _ocaps.width():
                    icaps.width();
    int height = _ocaps.height() > 1?
                     _ocaps.height():
                     icaps.height();
    _ocaps.setWidth(width);
    _ocaps.setHeight(height);
    _ocaps.setFps(icaps.fps());
    this->outputConvertCaps = _ocaps;

    if (aspectRatioMode == AkVideoConverter::AspectRatioMode_Keep) {
        auto w = height * icaps.width() / icaps.height();
        auto h = width * icaps.height() / icaps.width();

        if (w > width)
            w = width;
        else if (h > height)
            h = height;

        width = w;
        height = h;
    }

    this->directConvertPi = icaps.format() == _ocaps.format()
                            && icaps.width() == _ocaps.width()
                            && icaps.height() == _ocaps.height();
    this->outputImage = {width, height, imageToAkFormat->key(outputPacketFormat)};
    this->outputImageFormat = this->outputImage.format();
    this->lineSizePi =
            qMin<size_t>(this->lineSize(_ocaps),
                         this->outputImage.bytesPerLine());

    if (this->outputImage.format() == QImage::Format_Indexed8)
        for (int i = 0; i < 256; i++)
            this->outputImage.setColor(i, QRgb(i));
}

void ImageConvertParameters::reset()
{
    this->inputCaps = AkVideoCaps();
    this->outputCaps = AkVideoCaps();
    this->outputConvertCaps = AkVideoCaps();
    this->outputImage = {};
    this->outputImageFormat = QImage::Format_Invalid;
    this->inputImageFormat = QImage::Format_Invalid;
    this->inputPacket = AkVideoPacket();
    this->inputImageWidth = 0;
    this->inputImageHeight = 0;
    this->lineSizePi = 0;
    this->lineSizeIp = 0;
    this->canConvert = false;
    this->directConvertPi = false;
    this->directConvertIp = false;
}

#include "moc_akvideoconverter.cpp"
