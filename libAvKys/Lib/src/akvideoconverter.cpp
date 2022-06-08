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
#include "akvideoformatspec.h"

using ImageToPixelFormatMap = QMap<QImage::Format, AkVideoCaps::PixelFormat>;

inline ImageToPixelFormatMap initImageToPixelFormatMap()
{
    ImageToPixelFormatMap imageToFormat {
        {QImage::Format_RGB32     , AkVideoCaps::Format_0rgb    },
        {QImage::Format_ARGB32    , AkVideoCaps::Format_argb    },
        {QImage::Format_RGB16     , AkVideoCaps::Format_rgb565le},
        {QImage::Format_RGB555    , AkVideoCaps::Format_rgb555le},
        {QImage::Format_RGB888    , AkVideoCaps::Format_rgb24   },
        {QImage::Format_RGB444    , AkVideoCaps::Format_rgb444le},
        {QImage::Format_Grayscale8, AkVideoCaps::Format_gray8   }
    };

    return imageToFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImageToPixelFormatMap, AkImageToFormat, (initImageToPixelFormatMap()))

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

class AkVideoConverterPrivate
{
    public:
        QMutex m_mutex;
        ColorConvert m_colorConvert;
        AkVideoCaps m_inputCaps;
        AkVideoCaps m_outputCaps;
        AkVideoConverter::ScalingMode m_scalingMode {AkVideoConverter::ScalingMode_Fast};
        AkVideoConverter::AspectRatioMode m_aspectRatioMode {AkVideoConverter::AspectRatioMode_Ignore};

        QVector<int> m_srcWidthOffsetX;
        QVector<int> m_srcWidthOffsetY;
        QVector<int> m_srcWidthOffsetZ;
        QVector<int> m_srcWidthOffsetA;
        QVector<int> m_srcHeight;

        QVector<int> m_dstWidthOffsetX;
        QVector<int> m_dstWidthOffsetY;
        QVector<int> m_dstWidthOffsetZ;
        QVector<int> m_dstWidthOffsetA;

        int m_planeXi {0};
        int m_planeYi {0};
        int m_planeZi {0};
        int m_planeAi {0};

        AkColorComponent m_compXi;
        AkColorComponent m_compYi;
        AkColorComponent m_compZi;
        AkColorComponent m_compAi;

        int m_planeXo {0};
        int m_planeYo {0};
        int m_planeZo {0};
        int m_planeAo {0};

        AkColorComponent m_compXo;
        AkColorComponent m_compYo;
        AkColorComponent m_compZo;
        AkColorComponent m_compAo;

        quint64 m_maxXi {0};
        quint64 m_maxYi {0};
        quint64 m_maxZi {0};
        quint64 m_maxAi {0};

        quint64 m_maskXo {0};
        quint64 m_maskYo {0};
        quint64 m_maskZo {0};
        quint64 m_maskAo {0};

        bool m_hasAlphaIn {false};
        bool m_hasAlphaOut {false};

        // Configure endianness conversion functions for color components

        template <typename T>
        using EndiannessConvertFunc = std::function<T (T value)>;

        template <typename InputType, typename OutputType>
        inline void configureEndiannessConvertFunc(int iendianness,
                                                   int oendianness,
                                                   EndiannessConvertFunc<InputType> &fromEndianness,
                                                   EndiannessConvertFunc<OutputType> &toEndianness) const
        {

            switch (iendianness) {
            case Q_BIG_ENDIAN:
                fromEndianness = [] (InputType value) -> InputType {
                    return qFromBigEndian(value);
                };

                break;
            case Q_LITTLE_ENDIAN:
                fromEndianness = [] (InputType value) -> InputType {
                    return qFromLittleEndian(value);
                };

                break;
            default:
                fromEndianness = [] (InputType value) -> InputType {
                    return value;
                };

                break;
            }

            switch (oendianness) {
            case Q_BIG_ENDIAN:
                toEndianness = [] (OutputType value) -> OutputType {
                    return qToBigEndian(value);
                };

                break;
            case Q_LITTLE_ENDIAN:
                toEndianness = [] (OutputType value) -> OutputType {
                    return qToLittleEndian(value);
                };

                break;
            default:
                toEndianness = [] (OutputType value) -> OutputType {
                    return value;
                };

                break;
            }
        }

        // Configure color conversion matrix

        void configureConvertParams(const AkVideoCaps &icaps,
                                    const AkVideoCaps &ocaps)
        {
            auto ispecs = AkVideoCaps::formatSpecs(icaps.format());
            auto ospecs = AkVideoCaps::formatSpecs(ocaps.format());

            if (this->m_inputCaps.format() == icaps.format()
                && this->m_outputCaps.format() == ocaps.format())
                return;

            this->m_inputCaps = icaps;
            this->m_outputCaps = ocaps;
            this->m_colorConvert.loadMatrix(ispecs, ospecs);

            switch (ispecs.type()) {
            case AkVideoFormatSpec::VFT_RGB:
                this->m_planeXi = ispecs.componentPlane(AkColorComponent::CT_R);
                this->m_planeYi = ispecs.componentPlane(AkColorComponent::CT_G);
                this->m_planeZi = ispecs.componentPlane(AkColorComponent::CT_B);

                this->m_compXi = ispecs.component(AkColorComponent::CT_R);
                this->m_compYi = ispecs.component(AkColorComponent::CT_G);
                this->m_compZi = ispecs.component(AkColorComponent::CT_B);

                break;

            case AkVideoFormatSpec::VFT_YUV:
                this->m_planeXi = ispecs.componentPlane(AkColorComponent::CT_Y);
                this->m_planeYi = ispecs.componentPlane(AkColorComponent::CT_U);
                this->m_planeZi = ispecs.componentPlane(AkColorComponent::CT_V);

                this->m_compXi = ispecs.component(AkColorComponent::CT_Y);
                this->m_compYi = ispecs.component(AkColorComponent::CT_U);
                this->m_compZi = ispecs.component(AkColorComponent::CT_V);

                break;

            default:
                break;
            }

            this->m_planeAi = ispecs.componentPlane(AkColorComponent::CT_A);
            this->m_compAi = ispecs.component(AkColorComponent::CT_A);

            switch (ospecs.type()) {
            case AkVideoFormatSpec::VFT_RGB:
                this->m_planeXo = ospecs.componentPlane(AkColorComponent::CT_R);
                this->m_planeYo = ospecs.componentPlane(AkColorComponent::CT_G);
                this->m_planeZo = ospecs.componentPlane(AkColorComponent::CT_B);

                this->m_compXo = ospecs.component(AkColorComponent::CT_R);
                this->m_compYo = ospecs.component(AkColorComponent::CT_G);
                this->m_compZo = ospecs.component(AkColorComponent::CT_B);

                break;

            case AkVideoFormatSpec::VFT_YUV:
                this->m_planeXo = ospecs.componentPlane(AkColorComponent::CT_Y);
                this->m_planeYo = ospecs.componentPlane(AkColorComponent::CT_U);
                this->m_planeZo = ospecs.componentPlane(AkColorComponent::CT_V);

                this->m_compXo = ospecs.component(AkColorComponent::CT_Y);
                this->m_compYo = ospecs.component(AkColorComponent::CT_U);
                this->m_compZo = ospecs.component(AkColorComponent::CT_V);

                break;

            default:
                break;
            }

            this->m_planeAo = ospecs.componentPlane(AkColorComponent::CT_A);
            this->m_compAo = ospecs.component(AkColorComponent::CT_A);

            this->m_maxXi = this->m_compXi.max<quint64>();
            this->m_maxYi = this->m_compYi.max<quint64>();
            this->m_maxZi = this->m_compZi.max<quint64>();
            this->m_maxAi = this->m_compAi.max<quint64>();

            this->m_maskXo = ~(this->m_compXo.max<quint64>() << this->m_compXo.shift());
            this->m_maskYo = ~(this->m_compYo.max<quint64>() << this->m_compYo.shift());
            this->m_maskZo = ~(this->m_compZo.max<quint64>() << this->m_compZo.shift());
            this->m_maskAo = ~(this->m_compAo.max<quint64>() << this->m_compAo.shift());

            this->m_colorConvert.setAlphaBits(this->m_compAo.length());
            this->m_hasAlphaIn = ispecs.contains(AkColorComponent::CT_A);
            this->m_hasAlphaOut = ispecs.contains(AkColorComponent::CT_A);
        }

        inline void configureScaling(const AkVideoCaps &icaps,
                                     const AkVideoCaps &ocaps)
        {
            if (this->m_inputCaps.size() == icaps.size()
                && this->m_outputCaps.size() == ocaps.size())
                return;

            this->m_inputCaps = icaps;
            this->m_outputCaps = ocaps;

            this->m_srcWidthOffsetX.resize(ocaps.width());
            this->m_srcWidthOffsetY.resize(ocaps.width());
            this->m_srcWidthOffsetZ.resize(ocaps.width());
            this->m_srcWidthOffsetA.resize(ocaps.width());
            this->m_srcHeight.resize(ocaps.height());
            this->m_dstWidthOffsetX.resize(ocaps.width());
            this->m_dstWidthOffsetY.resize(ocaps.width());
            this->m_dstWidthOffsetZ.resize(ocaps.width());
            this->m_dstWidthOffsetA.resize(ocaps.width());

            int wi_1 = icaps.width() - 1;
            int wo_1 = ocaps.width() - 1;

            for (int x = 0; x < ocaps.width(); x++) {
                auto xs = x * wi_1 / wo_1;
                this->m_srcWidthOffsetX << (xs >> this->m_compXi.widthDiv()) * this->m_compXi.step();
                this->m_srcWidthOffsetY << (xs >> this->m_compYi.widthDiv()) * this->m_compYi.step();
                this->m_srcWidthOffsetZ << (xs >> this->m_compZi.widthDiv()) * this->m_compZi.step();
                this->m_srcWidthOffsetA << (xs >> this->m_compAi.widthDiv()) * this->m_compAi.step();

                this->m_dstWidthOffsetX << (x >> this->m_compAo.widthDiv()) * this->m_compAo.step();
                this->m_dstWidthOffsetY << (x >> this->m_compZo.widthDiv()) * this->m_compZo.step();
                this->m_dstWidthOffsetZ << (x >> this->m_compYo.widthDiv()) * this->m_compYo.step();
                this->m_dstWidthOffsetA << (x >> this->m_compXo.widthDiv()) * this->m_compXo.step();
            }

            int hi_1 = icaps.height() - 1;
            int ho_1 = ocaps.height() - 1;

            for (int y = 0; y < ocaps.height(); y++)
                this->m_srcHeight << y * hi_1 / ho_1;
        }

        // Conversion functions for 3 components to 3 components formats

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert3to3(const AkVideoPacket &src,
                         AkVideoPacket &dst,
                         TransformFuncType1 transformFrom,
                         TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert3to3A(const AkVideoPacket &src,
                          AkVideoPacket &dst,
                          TransformFuncType1 transformFrom,
                          TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();
            quint64 maskAo = this->m_maxAi << this->m_compAo.shift();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();
                auto dst_line_a = dst.line(this->m_planeAo, y) + this->m_compAo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];
                    int xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());
                    *ao = *ao | maskAo;

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);
                    auto aot = transformTo(*ao);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert3Ato3(const AkVideoPacket &src,
                          AkVideoPacket &dst,
                          TransformFuncType1 transformFrom,
                          TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_compAi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];
                    int xs_a = this->m_srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;
                    ai = (transformFrom(ai) >> this->m_compAi.shift()) & this->m_maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);
                    this->m_colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert3Ato3A(const AkVideoPacket &src,
                           AkVideoPacket &dst,
                           TransformFuncType1 transformFrom,
                           TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_compAi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();
                auto dst_line_a = dst.line(this->m_planeAo, y) + this->m_compAo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];
                    int xs_a = this->m_srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;
                    ai = (transformFrom(ai) >> this->m_compAi.shift()) & this->m_maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];
                    int xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());
                    *ao = (*ao & this->m_maskAo) | (ai << this->m_compAo.shift());

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);
                    auto aot = transformTo(*ao);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 3 components to 3 components formats
        // (same color space)

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convertV3to3(const AkVideoPacket &src,
                          AkVideoPacket &dst,
                          TransformFuncType1 transformFrom,
                          TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convertV3to3A(const AkVideoPacket &src,
                           AkVideoPacket &dst,
                           TransformFuncType1 transformFrom,
                           TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();
            quint64 maskAo = this->m_maxAi << this->m_compAo.shift();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();
                auto dst_line_a = dst.line(this->m_planeAo, y) + this->m_compAo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];
                    int xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());
                    *ao = *ao | maskAo;

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);
                    auto aot = transformTo(*ao);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convertV3Ato3(const AkVideoPacket &src,
                           AkVideoPacket &dst,
                           TransformFuncType1 transformFrom,
                           TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_compAi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];
                    int xs_a = this->m_srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;
                    ai = (transformFrom(ai) >> this->m_compAi.shift()) & this->m_maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);
                    this->m_colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convertV3Ato3A(const AkVideoPacket &src,
                            AkVideoPacket &dst,
                            TransformFuncType1 transformFrom,
                            TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_compAi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();
                auto dst_line_a = dst.line(this->m_planeAo, y) + this->m_compAo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];
                    int xs_a = this->m_srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;
                    ai = (transformFrom(ai) >> this->m_compAi.shift()) & this->m_maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];
                    int xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());
                    *ao = (*ao & this->m_maskAo) | (ai << this->m_compAo.shift());

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);
                    auto aot = transformTo(*ao);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 3 components to 1 components formats

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert3to1(const AkVideoPacket &src,
                         AkVideoPacket &dst,
                         TransformFuncType1 transformFrom,
                         TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y);

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;

                    qint64 p = 0;
                    this->m_colorConvert.applyPoint(xi, yi, zi, &p);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = transformTo(p << this->m_compXo.shift());
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert3to1A(const AkVideoPacket &src,
                          AkVideoPacket &dst,
                          TransformFuncType1 transformFrom,
                          TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();
            quint64 maskAo = this->m_maxAi << this->m_compAo.shift();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_a = dst.line(this->m_planeAo, y) + this->m_compAo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;

                    qint64 p = 0;
                    this->m_colorConvert.applyPoint(xi, yi, zi, &p);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_maskXo) | (p << this->m_compXo.shift());
                    *ao = *ao | maskAo;

                    auto xot = transformTo(*xo);
                    auto aot = transformTo(*ao);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert3Ato1(const AkVideoPacket &src,
                          AkVideoPacket &dst,
                          TransformFuncType1 transformFrom,
                          TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_compAi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y);

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];
                    int xs_a = this->m_srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;
                    ai = (transformFrom(ai) >> this->m_compAi.shift()) & this->m_maxAi;

                    qint64 p = 0;
                    this->m_colorConvert.applyPoint(xi, yi, zi, &p);
                    this->m_colorConvert.applyAlpha(ai, &p);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = p << this->m_compXo.shift();
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert3Ato1A(const AkVideoPacket &src,
                           AkVideoPacket &dst,
                           TransformFuncType1 transformFrom,
                           TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_y = src.constLine(this->m_planeYi, ys) + this->m_compYi.offset();
                auto src_line_z = src.constLine(this->m_planeZi, ys) + this->m_compZi.offset();
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_compAi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_a = dst.line(this->m_planeAo, y) + this->m_compAo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_y = this->m_srcWidthOffsetY[x];
                    int xs_z = this->m_srcWidthOffsetZ[x];
                    int xs_a = this->m_srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                    auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    yi = (transformFrom(yi) >> this->m_compYi.shift()) & this->m_maxYi;
                    zi = (transformFrom(zi) >> this->m_compZi.shift()) & this->m_maxZi;
                    ai = (transformFrom(ai) >> this->m_compAi.shift()) & this->m_maxAi;

                    qint64 p = 0;
                    this->m_colorConvert.applyPoint(xi, yi, zi, &p);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_maskXo) | (p << this->m_compXo.shift());
                    *ao = (*ao & this->m_maskAo) | (ai << this->m_compAo.shift());

                    auto xot = transformTo(*xo);
                    auto aot = transformTo(*ao);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 3 components formats

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert1to3(const AkVideoPacket &src,
                         AkVideoPacket &dst,
                         TransformFuncType1 transformFrom,
                         TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys);

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = transformFrom(xi) >> this->m_compXi.shift();

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert1to3A(const AkVideoPacket &src,
                          AkVideoPacket &dst,
                          TransformFuncType1 transformFrom,
                          TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();
            quint64 maskAo = this->m_maxAi << this->m_compAo.shift();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys);

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();
                auto dst_line_a = dst.line(this->m_planeAo, y) + this->m_compAo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = transformFrom(xi) >> this->m_compXi.shift();

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];
                    int xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());
                    *ao = *ao | maskAo;

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);
                    auto aot = transformTo(*ao);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert1Ato3(const AkVideoPacket &src,
                          AkVideoPacket &dst,
                          TransformFuncType1 transformFrom,
                          TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_compAi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_a = this->m_srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    ai = (transformFrom(ai) >> this->m_compAi.shift()) & this->m_maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);
                    this->m_colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert1Ato3A(const AkVideoPacket &src,
                           AkVideoPacket &dst,
                           TransformFuncType1 transformFrom,
                           TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_compAi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_y = dst.line(this->m_planeYo, y) + this->m_compYo.offset();
                auto dst_line_z = dst.line(this->m_planeZo, y) + this->m_compZo.offset();
                auto dst_line_a = dst.line(this->m_planeAo, y) + this->m_compAo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_a = this->m_srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    ai = (transformFrom(ai) >> this->m_compAi.shift()) & this->m_maxAi;

                    qint64 xo_ = 0;
                    qint64 yo_ = 0;
                    qint64 zo_ = 0;
                    this->m_colorConvert.applyPoint(xi, &xo_, &yo_, &zo_);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_y = this->m_dstWidthOffsetY[x];
                    int xd_z = this->m_dstWidthOffsetZ[x];
                    int xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto yo = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                    auto zo = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_maskXo) | (xo_ << this->m_compXo.shift());
                    *yo = (*yo & this->m_maskYo) | (yo_ << this->m_compYo.shift());
                    *zo = (*zo & this->m_maskZo) | (zo_ << this->m_compZo.shift());
                    *ao = (*ao & this->m_maskAo) | (ai << this->m_compAo.shift());

                    auto xot = transformTo(*xo);
                    auto yot = transformTo(*yo);
                    auto zot = transformTo(*zo);
                    auto aot = transformTo(*ao);

                    *xo = xot;
                    *yo = yot;
                    *zo = zot;
                    *ao = aot;
                }
            }
        }

        // Conversion functions for 1 components to 1 components formats

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert1to1(const AkVideoPacket &src,
                         AkVideoPacket &dst,
                         TransformFuncType1 transformFrom,
                         TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys);

                auto dst_line_x = dst.line(this->m_planeXo, y);

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = transformFrom(xi) >> this->m_compXi.shift();

                    qint64 p = 0;
                    this->m_colorConvert.applyPoint(xi, &p);

                    int xd_x = this->m_dstWidthOffsetX[x >> this->m_compXo.widthDiv()];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = transformTo(p << this->m_compXo.shift());
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert1to1A(const AkVideoPacket &src,
                          AkVideoPacket &dst,
                          TransformFuncType1 transformFrom,
                          TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();
            quint64 maskAo = this->m_maxAi << this->m_compAo.shift();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys);

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_a = dst.line(this->m_planeAo, y) + this->m_compAo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    xi = transformFrom(xi) >> this->m_compXi.shift();

                    qint64 p = 0;
                    this->m_colorConvert.applyPoint(xi, &p);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_maskXo) | (p << this->m_compXo.shift());
                    *ao = *ao | maskAo;

                    auto xot = transformTo(*xo);
                    auto aot = transformTo(*ao);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert1Ato1(const AkVideoPacket &src,
                          AkVideoPacket &dst,
                          TransformFuncType1 transformFrom,
                          TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_compAi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_a = this->m_srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    ai = (transformFrom(ai) >> this->m_compAi.shift()) & this->m_maxAi;

                    qint64 p = 0;
                    this->m_colorConvert.applyPoint(xi, &p);
                    this->m_colorConvert.applyAlpha(ai, &p);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    *xo = transformTo(p << this->m_compXo.shift());
                }
            }
        }

        template <typename InputType,
                  typename OutputType,
                  typename TransformFuncType1,
                  typename TransformFuncType2>
        void convert1Ato1A(const AkVideoPacket &src,
                           AkVideoPacket &dst,
                           TransformFuncType1 transformFrom,
                           TransformFuncType2 transformTo) const
        {
            int width = dst.caps().width();
            int height = dst.caps().height();

            for (int y = 0; y < height; ++y) {
                auto ys = this->m_srcHeight[y];
                auto src_line_x = src.constLine(this->m_planeXi, ys) + this->m_compXi.offset();
                auto src_line_a = src.constLine(this->m_planeAi, ys) + this->m_compAi.offset();

                auto dst_line_x = dst.line(this->m_planeXo, y) + this->m_compXo.offset();
                auto dst_line_a = dst.line(this->m_planeAo, y) + this->m_compAo.offset();

                for (int x = 0; x < width; ++x) {
                    int xs_x = this->m_srcWidthOffsetX[x];
                    int xs_a = this->m_srcWidthOffsetA[x];

                    auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                    auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                    xi = (transformFrom(xi) >> this->m_compXi.shift()) & this->m_maxXi;
                    ai = (transformFrom(ai) >> this->m_compAi.shift()) & this->m_maxAi;

                    qint64 p = 0;
                    this->m_colorConvert.applyPoint(xi, &p);

                    int xd_x = this->m_dstWidthOffsetX[x];
                    int xd_a = this->m_dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                    auto ao = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                    *xo = (*xo & this->m_maskXo) | (p << this->m_compXo.shift());
                    *ao = (*ao & this->m_maskAo) | (ai << this->m_compAo.shift());

                    auto xot = transformTo(*xo);
                    auto aot = transformTo(*ao);

                    *xo = xot;
                    *ao = aot;
                }
            }
        }

#define CONVERT_FUNC(icomponents, ocomponents) \
        template <typename InputType, \
                  typename OutputType, \
                  typename TransformFuncType1, \
                  typename TransformFuncType2> \
        inline void convertFormat##icomponents##to##ocomponents(const AkVideoPacket &src, \
                                                                AkVideoPacket &dst, \
                                                                TransformFuncType1 fromEndianness, \
                                                                TransformFuncType2 toEndianness) const \
        { \
            if (this->m_hasAlphaIn && this->m_hasAlphaOut) \
                this->convert##icomponents##Ato##ocomponents##A<InputType, OutputType>(src, \
                                                                                       dst, \
                                                                                       fromEndianness, \
                                                                                       toEndianness); \
            else if (this->m_hasAlphaIn && !this->m_hasAlphaOut) \
                this->convert##icomponents##Ato##ocomponents<InputType, OutputType>(src, \
                                                                                    dst, \
                                                                                    fromEndianness, \
                                                                                    toEndianness); \
            else if (!this->m_hasAlphaIn && this->m_hasAlphaOut) \
                this->convert##icomponents##to##ocomponents##A<InputType, OutputType>(src, \
                                                                                      dst, \
                                                                                      fromEndianness, \
                                                                                      toEndianness); \
            else if (!this->m_hasAlphaIn && !this->m_hasAlphaOut) \
                this->convert##icomponents##to##ocomponents<InputType, OutputType>(src, \
                                                                                   dst, \
                                                                                   fromEndianness, \
                                                                                   toEndianness); \
        }

#define CONVERTV_FUNC(icomponents, ocomponents) \
        template <typename InputType, \
                  typename OutputType, \
                  typename TransformFuncType1, \
                  typename TransformFuncType2> \
        inline void convertVFormat##icomponents##to##ocomponents(const AkVideoPacket &src, \
                                                                 AkVideoPacket &dst, \
                                                                 TransformFuncType1 fromEndianness, \
                                                                 TransformFuncType2 toEndianness) const \
        { \
            if (this->m_hasAlphaIn && this->m_hasAlphaOut) \
                this->convertV##icomponents##Ato##ocomponents##A<InputType, OutputType>(src, \
                                                                                        dst, \
                                                                                        fromEndianness, \
                                                                                        toEndianness); \
            else if (this->m_hasAlphaIn && !this->m_hasAlphaOut) \
                this->convertV##icomponents##Ato##ocomponents<InputType, OutputType>(src, \
                                                                                     dst, \
                                                                                     fromEndianness, \
                                                                                     toEndianness); \
            else if (!this->m_hasAlphaIn && this->m_hasAlphaOut) \
                this->convertV##icomponents##to##ocomponents##A<InputType, OutputType>(src, \
                                                                                       dst, \
                                                                                       fromEndianness, \
                                                                                       toEndianness); \
            else if (!this->m_hasAlphaIn && !this->m_hasAlphaOut) \
                this->convertV##icomponents##to##ocomponents<InputType, OutputType>(src, \
                                                                                    dst, \
                                                                                    fromEndianness, \
                                                                                    toEndianness); \
        }

        CONVERTV_FUNC(3, 3)
        CONVERT_FUNC(3, 3)
        CONVERT_FUNC(3, 1)
        CONVERT_FUNC(1, 3)
        CONVERT_FUNC(1, 1)

        template <typename InputType, typename OutputType>
        inline AkVideoPacket convert(const AkVideoFormatSpec &ispecs,
                                     const AkVideoFormatSpec &ospecs,
                                     const AkVideoPacket &packet,
                                     const AkVideoCaps &ocaps)
        {
            AkVideoPacket dst(ocaps);

            this->configureConvertParams(packet.caps(), ocaps);
            this->configureScaling(packet.caps(), ocaps);
            EndiannessConvertFunc<InputType> fromEndianness;
            EndiannessConvertFunc<OutputType> toEndianness;
            this->configureEndiannessConvertFunc<InputType, OutputType>(ispecs.endianness(),
                                                                        ospecs.endianness(),
                                                                        fromEndianness,
                                                                        toEndianness);

            auto icomponents = ispecs.mainComponents();
            auto ocomponents = ospecs.mainComponents();

            if (icomponents == 3 && ispecs.type() == ospecs.type())
                this->convertVFormat3to3<InputType, OutputType>(packet,
                                                                dst,
                                                                fromEndianness,
                                                                toEndianness);
            else if (icomponents == 3 && ocomponents == 3)
                this->convertFormat3to3<InputType, OutputType>(packet,
                                                               dst,
                                                               fromEndianness,
                                                               toEndianness);
            else if (icomponents == 3 && ocomponents == 1)
                this->convertFormat3to1<InputType, OutputType>(packet,
                                                               dst,
                                                               fromEndianness,
                                                               toEndianness);
            else if (icomponents == 1 && ocomponents == 3)
                this->convertFormat1to3<InputType, OutputType>(packet,
                                                               dst,
                                                               fromEndianness,
                                                               toEndianness);
            else if (icomponents == 1 && ocomponents == 1)
                this->convertFormat1to1<InputType, OutputType>(packet,
                                                               dst,
                                                               fromEndianness,
                                                               toEndianness);

            return dst;
        }

#define DEFINE_CONVERT_FUNC(isize, itype, osize, otype) \
        if (ispecs.byteLength() == isize && ospecs.byteLength() == osize) \
            return this->convert<itype, otype>(ispecs, \
                                               ospecs, \
                                               packet, \
                                               ocaps);

        AkVideoPacket convert(const AkVideoPacket &packet,
                              const AkVideoCaps &ocaps)
        {
            auto ispecs = AkVideoCaps::formatSpecs(packet.caps().format());
            auto ospecs = AkVideoCaps::formatSpecs(ocaps.format());

            DEFINE_CONVERT_FUNC(1, quint8 , 1, quint8 )
            DEFINE_CONVERT_FUNC(1, quint8 , 2, quint16)
            DEFINE_CONVERT_FUNC(1, quint8 , 4, quint32)
            DEFINE_CONVERT_FUNC(2, quint16, 1, quint8 )
            DEFINE_CONVERT_FUNC(2, quint16, 2, quint16)
            DEFINE_CONVERT_FUNC(2, quint16, 4, quint32)
            DEFINE_CONVERT_FUNC(4, quint32, 1, quint8 )
            DEFINE_CONVERT_FUNC(4, quint32, 2, quint16)
            DEFINE_CONVERT_FUNC(4, quint32, 4, quint32)

            return {};
        }
};

AkVideoConverter::AkVideoConverter(const AkVideoCaps &outputCaps, QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoConverterPrivate();
    this->d->m_outputCaps = outputCaps;
}

AkVideoConverter::AkVideoConverter(const AkVideoConverter &other)
{
    this->d = new AkVideoConverterPrivate();
    this->d->m_outputCaps = other.d->m_outputCaps;
    this->d->m_inputCaps = other.d->m_inputCaps;
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
        this->d->m_inputCaps = other.d->m_inputCaps;
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

bool AkVideoConverter::canConvert(AkVideoCaps::PixelFormat input,
                                  AkVideoCaps::PixelFormat output)
{
    if (input == output)
        return true;

    for (auto &convert: *videoConvert)
        if (convert.from == input
            && convert.to == output) {
            return true;
        }

    auto values = AkImageToFormat->values();

    if (values.contains(input) && values.contains(output))
        return true;

    return false;
}

AkVideoPacket AkVideoConverter::convert(const AkVideoPacket &packet) const
{
    if (packet.caps().format() == format) {
        if (packet.caps() != align)
            return packet.realign(align);

        return packet;
    }

    for (auto &convert: *videoConvert)
        if (convert.from == packet.caps().format()
            && convert.to == format) {
            return convert.convert(&packet, align);
        }

    if (!AkImageToFormat->values().contains(format))
        return {};

    auto frame = this->convertToImage(packet);

    if (frame.isNull())
        return packet;

    auto convertedFrame = frame.convertToFormat(AkImageToFormat->key(format));
    //return AkVideoPacket::fromImage(this->toImage().scaled(width, height),
    //                                *this);

    return this->convert(convertedFrame, packet);
}

AkVideoPacket AkVideoConverter::convert(const QImage &image,
                                        const AkVideoPacket &defaultPacket) const
{
    if (!AkImageToFormat->contains(image.format()))
        return AkVideoPacket();

    auto imageSize = image.bytesPerLine() * image.height();
    QByteArray oBuffer(imageSize, Qt::Uninitialized);
    memcpy(oBuffer.data(), image.constBits(), size_t(imageSize));

    AkVideoPacket packet;
    packet.caps() = {AkImageToFormat->value(image.format()),
                     image.width(),
                     image.height(),
                     defaultPacket.caps().fps()};
    packet.buffer() = oBuffer;
    packet.copyMetadata(defaultPacket);

    return packet;
}

QImage AkVideoConverter::convertToImage(const AkVideoPacket &packet,
                                        QImage::Format format) const
{
}

QImage AkVideoConverter::convertToImage(const AkVideoPacket &packet) const
{
    if (!packet.caps())
        return {};

    if (!AkImageToFormat->values().contains(packet.caps().format()))
        return {};

    QImage image(packet.caps().width(),
                 packet.caps().height(),
                 AkImageToFormat->key(packet.caps().format()));
    auto size = qMin(size_t(packet.buffer().size()),
                     size_t(image.bytesPerLine()) * size_t(image.height()));

    if (size > 0)
        memcpy(image.bits(), packet.buffer().constData(), size);

    if (packet.caps().format() == AkVideoCaps::Format_gray8)
        for (int i = 0; i < 256; i++)
            image.setColor(i, QRgb(i));

    return image;
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
    this->d->m_mutex.lock();
    this->d->m_inputCaps = AkVideoCaps();
    this->d->m_mutex.unlock();
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
    *x = qBound(this->xmin, (a * this->m00 + this->m03) >> this->shift, this->xmax);
    *y = qBound(this->ymin, (b * this->m11 + this->m13) >> this->shift, this->ymax);
    *z = qBound(this->zmin, (c * this->m22 + this->m23) >> this->shift, this->zmax);
}

void ColorConvert::applyPoint(qint64 p,
                              qint64 *x, qint64 *y, qint64 *z) const
{
    *x = qBound(this->xmin, (p * this->m00 + this->m03) >> this->shift, this->xmax);
    *y = qBound(this->ymin, (p * this->m11 + this->m13) >> this->shift, this->ymax);
    *z = qBound(this->zmin, (p * this->m22 + this->m23) >> this->shift, this->zmax);
}

void ColorConvert::applyPoint(qint64 a, qint64 b, qint64 c,
                              qint64 *p) const
{
    *p = qBound(this->xmin, (a * this->m00 + b * this->m01 + c * this->m02 + this->m03) >> this->shift, this->xmax);
}

void ColorConvert::applyPoint(qint64 p, qint64 *q) const
{
    *q = qBound(this->xmin, (p * this->m00 + this->m03) >> this->shift, this->xmax);
}

void ColorConvert::applyAlpha(qint64 x, qint64 y, qint64 z, qint64 a,
                              qint64 *xa, qint64 *ya, qint64 *za) const
{
    qint64 diffA = this->amax - a;
    *xa = (x * a + this->xmin * diffA) / this->amax;
    *ya = (y * a + this->xmin * diffA) / this->amax;
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

    if (to.contains(AkColorComponent::CT_A))
        this->setAlphaBits(to.component(AkColorComponent::CT_A).length());

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
    qint64 rounding = 1L << (shift - 1);

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

#include "moc_akvideoconverter.cpp"
