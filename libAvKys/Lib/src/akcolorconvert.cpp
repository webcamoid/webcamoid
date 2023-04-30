/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

#include <QtDebug>
#include <QQmlEngine>
#include <limits>

#include "akcolorconvert.h"
#include "akvideoformatspec.h"

class AkColorConvertPrivate
{
    public:
        AkColorConvert *self {nullptr};
        AkColorConvert::YuvColorSpace m_yuvColorSpace {AkColorConvert::YuvColorSpace_ITUR_BT601};
        AkColorConvert::YuvColorSpaceType m_yuvColorSpaceType {AkColorConvert::YuvColorSpaceType_StudioSwing};

        explicit AkColorConvertPrivate(AkColorConvert *self);
        void rbConstants(AkColorConvert::YuvColorSpace colorSpace,
                         qint64 &kr,
                         qint64 &kb,
                         qint64 &div) const;
        qint64 roundedDiv(qint64 num, qint64 den) const;
        qint64 nearestPowOf2(qint64 value) const;
        void limitsY(int bits,
                     AkColorConvert::YuvColorSpaceType type,
                     qint64 &minY,
                     qint64 &maxY) const;
        void limitsUV(int bits,
                      AkColorConvert::YuvColorSpaceType type,
                      qint64 &minUV,
                      qint64 &maxUV) const;
        void loadAbc2xyzMatrix(int abits,
                               int bbits,
                               int cbits,
                               int xbits,
                               int ybits,
                               int zbits);
        void loadRgb2yuvMatrix(AkColorConvert::YuvColorSpace YuvColorSpace,
                               AkColorConvert::YuvColorSpaceType yuvColorSpaceType,
                               int rbits,
                               int gbits,
                               int bbits,
                               int ybits,
                               int ubits,
                               int vbits);
        void loadYuv2rgbMatrix(AkColorConvert::YuvColorSpace YuvColorSpace,
                               AkColorConvert::YuvColorSpaceType yuvColorSpaceType,
                               int ybits,
                               int ubits,
                               int vbits,
                               int rbits,
                               int gbits,
                               int bbits);
        void loadRgb2grayMatrix(AkColorConvert::YuvColorSpace YuvColorSpace,
                                int rbits,
                                int gbits,
                                int bbits,
                                int graybits);
        void loadGray2rgbMatrix(int graybits,
                                int rbits,
                                int gbits,
                                int bbits);
        void loadYuv2grayMatrix(AkColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                int ybits,
                                int ubits,
                                int vbits,
                                int graybits);
        void loadGray2yuvMatrix(AkColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                int graybits,
                                int ybits,
                                int ubits,
                                int vbits);
        void loadAlphaRgbMatrix(int alphaBits);
        void loadAlphaYuvMatrix(AkColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                int alphaBits,
                                int ybits,
                                int ubits,
                                int vbits);
        void loadAlphaGrayMatrix(int alphaBits, int graybits);
};

AkColorConvert::AkColorConvert(QObject *parent):
    QObject(parent)
{
    this->d = new AkColorConvertPrivate(this);
}

AkColorConvert::AkColorConvert(YuvColorSpace yuvColorSpace,
                               YuvColorSpaceType yuvColorSpaceType,
                               QObject *parent):
    QObject(parent)
{
    this->d = new AkColorConvertPrivate(this);
    this->d->m_yuvColorSpace = yuvColorSpace;
    this->d->m_yuvColorSpaceType = yuvColorSpaceType;
}

AkColorConvert::AkColorConvert(YuvColorSpaceType yuvColorSpaceType,
                               QObject *parent):
    QObject(parent)
{
    this->d = new AkColorConvertPrivate(this);
    this->d->m_yuvColorSpaceType = yuvColorSpaceType;
}

AkColorConvert::AkColorConvert(const AkColorConvert &other):
    QObject()
{
    this->d = new AkColorConvertPrivate(this);
    this->d->m_yuvColorSpace = other.d->m_yuvColorSpace;
    this->d->m_yuvColorSpaceType = other.d->m_yuvColorSpaceType;
    this->m00 = other.m00; this->m01 = other.m01; this->m02 = other.m02; this->m03 = other.m03;
    this->m10 = other.m10; this->m11 = other.m11; this->m12 = other.m12; this->m13 = other.m13;
    this->m20 = other.m20; this->m21 = other.m21; this->m22 = other.m22; this->m23 = other.m23;
    this->a00 = other.a00; this->a01 = other.a01; this->a02 = other.a02;
    this->a10 = other.a10; this->a11 = other.a11; this->a12 = other.a12;
    this->a20 = other.a20; this->a21 = other.a21; this->a22 = other.a22;
    this->xmin = other.xmin; this->xmax = other.xmax;
    this->ymin = other.ymin; this->ymax = other.ymax;
    this->zmin = other.zmin; this->zmax = other.zmax;
    this->colorShift = other.colorShift;
    this->alphaShift = other.alphaShift;
}

AkColorConvert::~AkColorConvert()
{
    delete this->d;
}

AkColorConvert &AkColorConvert::operator =(const AkColorConvert &other)
{
    if (this != &other) {
        this->d->m_yuvColorSpace = other.d->m_yuvColorSpace;
        this->d->m_yuvColorSpaceType = other.d->m_yuvColorSpaceType;
        this->m00 = other.m00; this->m01 = other.m01; this->m02 = other.m02; this->m03 = other.m03;
        this->m10 = other.m10; this->m11 = other.m11; this->m12 = other.m12; this->m13 = other.m13;
        this->m20 = other.m20; this->m21 = other.m21; this->m22 = other.m22; this->m23 = other.m23;
        this->a00 = other.a00; this->a01 = other.a01; this->a02 = other.a02;
        this->a10 = other.a10; this->a11 = other.a11; this->a12 = other.a12;
        this->a20 = other.a20; this->a21 = other.a21; this->a22 = other.a22;
        this->xmin = other.xmin; this->xmax = other.xmax;
        this->ymin = other.ymin; this->ymax = other.ymax;
        this->zmin = other.zmin; this->zmax = other.zmax;
        this->colorShift = other.colorShift;
        this->alphaShift = other.alphaShift;
    }

    return *this;
}

AkColorConvert::YuvColorSpace AkColorConvert::yuvColorSpace() const
{
    return this->d->m_yuvColorSpace;
}

AkColorConvert::YuvColorSpaceType AkColorConvert::yuvColorSpaceType() const
{
    return this->d->m_yuvColorSpaceType;
}

void AkColorConvert::setYuvColorSpace(YuvColorSpace yuvColorSpace)
{
    if (this->d->m_yuvColorSpace == yuvColorSpace)
        return;

    this->d->m_yuvColorSpace = yuvColorSpace;
    emit this->yuvColorSpaceChanged(yuvColorSpace);
}

void AkColorConvert::setYuvColorSpaceType(YuvColorSpaceType yuvColorSpaceType)
{
    if (this->d->m_yuvColorSpaceType == yuvColorSpaceType)
        return;

    this->d->m_yuvColorSpaceType = yuvColorSpaceType;
    emit this->yuvColorSpaceTypeChanged(yuvColorSpaceType);
}

void AkColorConvert::resetYuvColorSpace()
{
    this->setYuvColorSpace(AkColorConvert::YuvColorSpace_ITUR_BT601);
}

void AkColorConvert::resetYuvColorSpaceType()
{
    this->setYuvColorSpaceType(AkColorConvert::YuvColorSpaceType_StudioSwing);
}

void AkColorConvert::loadColorMatrix(ColorMatrix colorMatrix,
                                     int ibitsa,
                                     int ibitsb,
                                     int ibitsc,
                                     int obitsx,
                                     int obitsy,
                                     int obitsz)
{
    switch (colorMatrix) {
    case ColorMatrix_ABC2XYZ:
        this->d->loadAbc2xyzMatrix(ibitsa,
                                   ibitsb,
                                   ibitsc,
                                   obitsx,
                                   obitsy,
                                   obitsz);

        break;

    case ColorMatrix_RGB2YUV:
        this->d->loadRgb2yuvMatrix(this->d->m_yuvColorSpace,
                                   this->d->m_yuvColorSpaceType,
                                   ibitsa,
                                   ibitsb,
                                   ibitsc,
                                   obitsx,
                                   obitsy,
                                   obitsz);

        break;

    case ColorMatrix_YUV2RGB:
        this->d->loadYuv2rgbMatrix(this->d->m_yuvColorSpace,
                                   this->d->m_yuvColorSpaceType,
                                   ibitsa,
                                   ibitsb,
                                   ibitsc,
                                   obitsx,
                                   obitsy,
                                   obitsz);

        break;

    case ColorMatrix_RGB2GRAY:
        this->d->loadRgb2grayMatrix(this->d->m_yuvColorSpace,
                                    ibitsa,
                                    ibitsb,
                                    ibitsc,
                                    obitsx);

        break;

    case ColorMatrix_GRAY2RGB:
        this->d->loadGray2rgbMatrix(ibitsa,
                                    obitsx,
                                    obitsy,
                                    obitsz);

        break;

    case ColorMatrix_YUV2GRAY:
        this->d->loadYuv2grayMatrix(this->d->m_yuvColorSpaceType,
                                    ibitsa,
                                    ibitsb,
                                    ibitsc,
                                    obitsx);

        break;

    case ColorMatrix_GRAY2YUV:
        this->d->loadGray2yuvMatrix(this->d->m_yuvColorSpaceType,
                                    ibitsa,
                                    obitsx,
                                    obitsy,
                                    obitsz);

    default:
        break;
    }
}

void AkColorConvert::loadAlphaMatrix(AkVideoFormatSpec::VideoFormatType formatType,
                                     int ibitsAlpha,
                                     int obitsx,
                                     int obitsy,
                                     int obitsz)
{
    switch (formatType) {
    case AkVideoFormatSpec::VFT_RGB:
        this->d->loadAlphaRgbMatrix(ibitsAlpha);

        break;

    case AkVideoFormatSpec::VFT_YUV:
        this->d->loadAlphaYuvMatrix(this->d->m_yuvColorSpaceType,
                                    ibitsAlpha,
                                    obitsx,
                                    obitsy,
                                    obitsz);

        break;

    case AkVideoFormatSpec::VFT_Gray:
        this->d->loadAlphaGrayMatrix(ibitsAlpha, obitsx);

        break;

    default:
        break;
    }
}

void AkColorConvert::loadMatrix(const AkVideoFormatSpec &from,
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

    this->loadColorMatrix(colorMatrix,
                          ibitsa,
                          ibitsb,
                          ibitsc,
                          obitsx,
                          obitsy,
                          obitsz);

    if (from.contains(AkColorComponent::CT_A))
        this->loadAlphaMatrix(to.type(),
                              from.component(AkColorComponent::CT_A).length(),
                              obitsx,
                              obitsy,
                              obitsz);
}

void AkColorConvert::loadMatrix(const AkVideoCaps::PixelFormat &from,
                                const AkVideoCaps::PixelFormat &to)
{
    this->loadMatrix(AkVideoCaps::formatSpecs(from),
                     AkVideoCaps::formatSpecs(to));
}

void AkColorConvert::registerTypes()
{
    qRegisterMetaType<AkColorConvert>("AkColorConvert");

    qRegisterMetaType<YuvColorSpace>("YuvColorSpace");
    QMetaType::registerDebugStreamOperator<AkColorConvert::YuvColorSpace>();
    qRegisterMetaType<YuvColorSpaceType>("YuvColorSpaceType");
    QMetaType::registerDebugStreamOperator<AkColorConvert::YuvColorSpaceType>();
    qRegisterMetaType<ColorMatrix>("ColorMatrix");
    QMetaType::registerDebugStreamOperator<AkColorConvert::ColorMatrix>();

    qmlRegisterSingletonType<AkColorConvert>("Ak", 1, 0, "AkColorConvert",
                                             [] (QQmlEngine *qmlEngine,
                                                QJSEngine *jsEngine) -> QObject * {
                                                 Q_UNUSED(qmlEngine)
                                                 Q_UNUSED(jsEngine)

                                                 return new AkColorConvert();
                                             });
}

QDebug operator <<(QDebug debug, AkColorConvert::YuvColorSpace yuvColorSpace)
{
    AkColorConvert convert;
    int yuvColorSpaceIndex =
        convert.metaObject()->indexOfEnumerator("YuvColorSpace");
    auto colorSpaceEnum = convert.metaObject()->enumerator(yuvColorSpaceIndex);
    QString str(colorSpaceEnum.valueToKey(yuvColorSpace));
    str.remove("YuvColorSpace_");
    debug.nospace() << str.toStdString().c_str();

    return debug.space();
}

QDebug operator <<(QDebug debug, AkColorConvert::YuvColorSpaceType yuvColorSpaceType)
{
    AkColorConvert convert;
    int yuvColorSpaceTypeIndex =
        convert.metaObject()->indexOfEnumerator("YuvColorSpaceType");
    auto colorSpaceTypeEnum = convert.metaObject()->enumerator(yuvColorSpaceTypeIndex);
    QString str(colorSpaceTypeEnum.valueToKey(yuvColorSpaceType));
    str.remove("YuvColorSpaceType_");
    debug.nospace() << str.toStdString().c_str();

    return debug.space();
}

QDebug operator <<(QDebug debug, AkColorConvert::ColorMatrix colorMatrix)
{
    AkColorConvert convert;
    int colorMatrixIndex =
        convert.metaObject()->indexOfEnumerator("ColorMatrix");
    auto colorMatrixEnum = convert.metaObject()->enumerator(colorMatrixIndex);
    QString str(colorMatrixEnum.valueToKey(colorMatrix));
    str.remove("ColorMatrix_");
    debug.nospace() << str.toStdString().c_str();

    return debug.space();
}

AkColorConvertPrivate::AkColorConvertPrivate(AkColorConvert *self):
    self(self)
{

}

void AkColorConvertPrivate::rbConstants(AkColorConvert::YuvColorSpace colorSpace,
                                        qint64 &kr,
                                        qint64 &kb,
                                        qint64 &div) const
{
    kr = 0;
    kb = 0;
    div = 10000;

    // Coefficients taken from https://en.wikipedia.org/wiki/YUV
    switch (colorSpace) {
    // Same weight for all components
    case AkColorConvert::YuvColorSpace_AVG:
        kr = 3333;
        kb = 3333;

        break;

        // https://www.itu.int/rec/R-REC-BT.601/en
    case AkColorConvert::YuvColorSpace_ITUR_BT601:
        kr = 2990;
        kb = 1140;

        break;

        // https://www.itu.int/rec/R-REC-BT.709/en
    case AkColorConvert::YuvColorSpace_ITUR_BT709:
        kr = 2126;
        kb = 722;

        break;

        // https://www.itu.int/rec/R-REC-BT.2020/en
    case AkColorConvert::YuvColorSpace_ITUR_BT2020:
        kr = 2627;
        kb = 593;

        break;

        // http://car.france3.mars.free.fr/HD/INA-%2026%20jan%2006/SMPTE%20normes%20et%20confs/s240m.pdf
    case AkColorConvert::YuvColorSpace_SMPTE_240M:
        kr = 2120;
        kb = 870;

        break;

    default:
        break;
    }
}

qint64 AkColorConvertPrivate::roundedDiv(qint64 num, qint64 den) const
{
    if (den == 0)
        return num < 0?
            std::numeric_limits<qint64>::min():
            std::numeric_limits<qint64>::max();

    if (((num < 0) && (den > 0)) || ((num > 0) && (den < 0)))
        return (2 * num - den) / (2 * den);

    return (2 * num + den) / (2 * den);
}

qint64 AkColorConvertPrivate::nearestPowOf2(qint64 value) const
{
    qint64 val = value;
    qint64 res = 0;

    while (val >>= 1)
        res++;

    if (qAbs((1 << (res + 1)) - value) <= qAbs((1 << res) - value))
        return 1 << (res + 1);

    return 1 << res;
}

void AkColorConvertPrivate::limitsY(int bits,
                                    AkColorConvert::YuvColorSpaceType type,
                                    qint64 &minY,
                                    qint64 &maxY) const
{
    if (type == AkColorConvert::YuvColorSpaceType_FullSwing) {
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
    minY = this->nearestPowOf2(this->roundedDiv(maxValue * g, 2 * g + 100));
    maxY = maxValue * (g + 100) / (2 * g + 100);
}

void AkColorConvertPrivate::limitsUV(int bits,
                                     AkColorConvert::YuvColorSpaceType type,
                                     qint64 &minUV,
                                     qint64 &maxUV) const
{
    if (type == AkColorConvert::YuvColorSpaceType_FullSwing) {
        minUV = 0;
        maxUV = (1 << bits) - 1;

        return;
    }

    static const qint64 g = 9;
    qint64 maxValue = (1 << bits) - 1;
    minUV = this->nearestPowOf2(this->roundedDiv(maxValue * g, 2 * g + 100));
    maxUV = (1L << bits) - minUV;
}

void AkColorConvertPrivate::loadAbc2xyzMatrix(int abits,
                                              int bbits,
                                              int cbits,
                                              int xbits,
                                              int ybits,
                                              int zbits)
{
    int shift = qMax(abits, qMax(bbits, cbits));
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

    self->m00 = kx; self->m01 = 0 ; self->m02 = 0 ; self->m03 = rounding;
    self->m10 = 0 ; self->m11 = ky; self->m12 = 0 ; self->m13 = rounding;
    self->m20 = 0 ; self->m21 = 0 ; self->m22 = kz; self->m23 = rounding;

    self->xmin = 0; self->xmax = xmax;
    self->ymin = 0; self->ymax = ymax;
    self->zmin = 0; self->zmax = zmax;

    self->colorShift = shift;
}

void AkColorConvertPrivate::loadRgb2yuvMatrix(AkColorConvert::YuvColorSpace yuvColorSpace,
                                              AkColorConvert::YuvColorSpaceType yuvColorSpaceType,
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
    this->rbConstants(yuvColorSpace, kyr, kyb, div);
    qint64 kyg = div - kyr - kyb;

    qint64 kur = -kyr;
    qint64 kug = -kyg;
    qint64 kub = div - kyb;

    qint64 kvr = div - kyr;
    qint64 kvg = -kyg;
    qint64 kvb = -kyb;

    int shift = qMax(rbits, qMax(gbits, bbits));
    qint64 shiftDiv = 1L << shift;
    qint64 rounding = 1L << (shift - 1);

    qint64 rmax = (1L << rbits) - 1;
    qint64 gmax = (1L << gbits) - 1;
    qint64 bmax = (1L << bbits) - 1;

    qint64 minY = 0;
    qint64 maxY = 0;
    this->limitsY(ybits, yuvColorSpaceType, minY, maxY);
    auto diffY = maxY - minY;

    qint64 kiyr = this->roundedDiv(shiftDiv * diffY * kyr, div * rmax);
    qint64 kiyg = this->roundedDiv(shiftDiv * diffY * kyg, div * gmax);
    qint64 kiyb = this->roundedDiv(shiftDiv * diffY * kyb, div * bmax);

    qint64 minU = 0;
    qint64 maxU = 0;
    this->limitsUV(ubits, yuvColorSpaceType, minU, maxU);
    auto diffU = maxU - minU;

    qint64 kiur = this->roundedDiv(shiftDiv * diffU * kur, 2 * rmax * kub);
    qint64 kiug = this->roundedDiv(shiftDiv * diffU * kug, 2 * gmax * kub);
    qint64 kiub = this->roundedDiv(shiftDiv * diffU      , 2 * bmax);

    qint64 minV = 0;
    qint64 maxV = 0;
    this->limitsUV(vbits, yuvColorSpaceType, minV, maxV);
    auto diffV = maxV - minV;

    qint64 kivr = this->roundedDiv(shiftDiv * diffV      , 2 * rmax);
    qint64 kivg = this->roundedDiv(shiftDiv * diffV * kvg, 2 * gmax * kvr);
    qint64 kivb = this->roundedDiv(shiftDiv * diffV * kvb, 2 * bmax * kvr);

    qint64 ciy = rounding + shiftDiv * minY;
    qint64 ciu = rounding + shiftDiv * (minU + maxU) / 2;
    qint64 civ = rounding + shiftDiv * (minV + maxV) / 2;

    self->m00 = kiyr; self->m01 = kiyg; self->m02 = kiyb; self->m03 = ciy;
    self->m10 = kiur; self->m11 = kiug; self->m12 = kiub; self->m13 = ciu;
    self->m20 = kivr; self->m21 = kivg; self->m22 = kivb; self->m23 = civ;

    self->xmin = minY; self->xmax = maxY;
    self->ymin = minU; self->ymax = maxU;
    self->zmin = minV; self->zmax = maxV;

    self->colorShift = shift;
}

void AkColorConvertPrivate::loadYuv2rgbMatrix(AkColorConvert::YuvColorSpace yuvColorSpace,
                                              AkColorConvert::YuvColorSpaceType yuvColorSpaceType,
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
    this->rbConstants(yuvColorSpace, kyr, kyb, div);
    qint64 kyg = div - kyr - kyb;

    qint64 minY = 0;
    qint64 maxY = 0;
    this->limitsY(ybits, yuvColorSpaceType, minY, maxY);
    auto diffY = maxY - minY;

    qint64 minU = 0;
    qint64 maxU = 0;
    this->limitsUV(ubits, yuvColorSpaceType, minU, maxU);
    auto diffU = maxU - minU;

    qint64 minV = 0;
    qint64 maxV = 0;
    this->limitsUV(vbits, yuvColorSpaceType, minV, maxV);
    auto diffV = maxV - minV;

    int shift = qMax(ybits, qMax(ubits, vbits));
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

    self->m00 = kry; self->m01 = 0  ; self->m02 = krv; self->m03 = cir;
    self->m10 = kgy; self->m11 = kgu; self->m12 = kgv; self->m13 = cig;
    self->m20 = kby; self->m21 = kbu; self->m22 = 0  ; self->m23 = cib;

    self->xmin = 0; self->xmax = rmax;
    self->ymin = 0; self->ymax = gmax;
    self->zmin = 0; self->zmax = bmax;

    self->colorShift = shift;
}

void AkColorConvertPrivate::loadRgb2grayMatrix(AkColorConvert::YuvColorSpace yuvColorSpace,
                                               int rbits,
                                               int gbits,
                                               int bbits,
                                               int graybits)
{
    AkColorConvert::YuvColorSpaceType type = AkColorConvert::YuvColorSpaceType_FullSwing;

    qint64 kyr = 0;
    qint64 kyb = 0;
    qint64 div = 0;
    this->rbConstants(yuvColorSpace, kyr, kyb, div);
    qint64 kyg = div - kyr - kyb;

    int shift = qMax(rbits, qMax(gbits, bbits));
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

    self->m00 = kiyr; self->m01 = kiyg; self->m02 = kiyb; self->m03 = ciy;
    self->m10 = 0   ; self->m11 = 0   ; self->m12 = 0   ; self->m13 = ciu;
    self->m20 = 0   ; self->m21 = 0   ; self->m22 = 0   ; self->m23 = civ;

    self->xmin = minY; self->xmax = maxY;
    self->ymin = minU; self->ymax = maxU;
    self->zmin = minV; self->zmax = maxV;

    self->colorShift = shift;
}

void AkColorConvertPrivate::loadGray2rgbMatrix(int graybits,
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

    self->m00 = kr; self->m01 = 0; self->m02 = 0; self->m03 = rounding;
    self->m10 = kg; self->m11 = 0; self->m12 = 0; self->m13 = rounding;
    self->m20 = kb; self->m21 = 0; self->m22 = 0; self->m23 = rounding;

    self->xmin = 0; self->xmax = rmax;
    self->ymin = 0; self->ymax = gmax;
    self->zmin = 0; self->zmax = bmax;

    self->colorShift = shift;
}

void AkColorConvertPrivate::loadYuv2grayMatrix(AkColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                               int ybits,
                                               int ubits,
                                               int vbits,
                                               int graybits)
{
    AkColorConvert::YuvColorSpaceType otype = AkColorConvert::YuvColorSpaceType_FullSwing;

    int shift = ybits;
    qint64 shiftDiv = 1L << shift;
    qint64 rounding = 1L << (shift - 1);

    qint64 graymax = (1L << graybits) - 1;

    qint64 minY = 0;
    qint64 maxY = 0;
    this->limitsY(ybits, yuvColorSpaceType, minY, maxY);
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

    self->m00 = ky; self->m01 = 0; self->m02 = 0; self->m03 = ciy;
    self->m10 = 0 ; self->m11 = 0; self->m12 = 0; self->m13 = ciu;
    self->m20 = 0 ; self->m21 = 0; self->m22 = 0; self->m23 = civ;

    self->xmin = 0; self->xmax = graymax;
    self->ymin = 0; self->ymax = graymax;
    self->zmin = 0; self->zmax = graymax;

    self->colorShift = shift;
}

void AkColorConvertPrivate::loadGray2yuvMatrix(AkColorConvert::YuvColorSpaceType yuvColorSpaceType,
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
    this->limitsY(ybits, yuvColorSpaceType, minY, maxY);
    auto diffY = maxY - minY;

    qint64 ky = this->roundedDiv(shiftDiv * diffY, graymax);

    qint64 minU = 0;
    qint64 maxU = 0;
    this->limitsUV(ubits, yuvColorSpaceType, minU, maxU);

    qint64 minV = 0;
    qint64 maxV = 0;
    this->limitsUV(vbits, yuvColorSpaceType, minV, maxV);

    qint64 ciy = rounding + shiftDiv * minY;
    qint64 ciu = rounding + shiftDiv * (minU + maxU) / 2;
    qint64 civ = rounding + shiftDiv * (minV + maxV) / 2;

    self->m00 = ky; self->m01 = 0; self->m02 = 0; self->m03 = ciy;
    self->m10 = 0 ; self->m11 = 0; self->m12 = 0; self->m13 = ciu;
    self->m20 = 0 ; self->m21 = 0; self->m22 = 0; self->m23 = civ;

    self->xmin = minY; self->xmax = maxY;
    self->ymin = minU; self->ymax = maxU;
    self->zmin = minV; self->zmax = maxV;

    self->colorShift = shift;
}

void AkColorConvertPrivate::loadAlphaRgbMatrix(int alphaBits)
{
    qint64 amax = (1L << alphaBits) - 1;
    self->alphaShift = alphaBits;
    qint64 shiftDiv = 1L << self->alphaShift;
    qint64 rounding = 1L << (self->alphaShift - 1);

    auto k = this->roundedDiv(shiftDiv, amax);

    self->a00 = k; self->a01 = 0; self->a02 = rounding;
    self->a10 = k; self->a11 = 0; self->a12 = rounding;
    self->a20 = k; self->a21 = 0; self->a22 = rounding;

}

void AkColorConvertPrivate::loadAlphaYuvMatrix(AkColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                               int alphaBits,
                                               int ybits,
                                               int ubits,
                                               int vbits)
{
    qint64 amax = (1L << alphaBits) - 1;
    self->alphaShift = alphaBits;
    qint64 shiftDiv = 1L << self->alphaShift;
    qint64 rounding = 1L << (self->alphaShift - 1);

    quint64 k = shiftDiv / amax;

    qint64 minY = 0;
    qint64 maxY = 0;
    this->limitsY(ybits, yuvColorSpaceType, minY, maxY);
    qint64 ky = -this->roundedDiv(shiftDiv * minY, amax);

    qint64 minU = 0;
    qint64 maxU = 0;
    this->limitsUV(ubits, yuvColorSpaceType, minU, maxU);
    qint64 ku = -this->roundedDiv(shiftDiv * (minU + maxU), 2 * amax);

    qint64 minV = 0;
    qint64 maxV = 0;
    this->limitsUV(vbits, yuvColorSpaceType, minV, maxV);
    qint64 kv = -this->roundedDiv(shiftDiv * (minV + maxV), 2 * amax);

    qint64 ciy = rounding + shiftDiv * minY;
    qint64 ciu = rounding + shiftDiv * (minU + maxU) / 2;
    qint64 civ = rounding + shiftDiv * (minV + maxV) / 2;

    self->a00 = k; self->a01 = ky; self->a02 = ciy;
    self->a10 = k; self->a11 = ku; self->a12 = ciu;
    self->a20 = k; self->a21 = kv; self->a22 = civ;
}

void AkColorConvertPrivate::loadAlphaGrayMatrix(int alphaBits, int graybits)
{
    AkColorConvert::YuvColorSpaceType otype = AkColorConvert::YuvColorSpaceType_FullSwing;

    qint64 amax = (1L << alphaBits) - 1;
    self->alphaShift = alphaBits;
    qint64 shiftDiv = 1L << self->alphaShift;
    qint64 rounding = 1L << (self->alphaShift - 1);

    auto k = this->roundedDiv(self->alphaShift, amax);

    qint64 minU = 0;
    qint64 maxU = 0;
    this->limitsUV(graybits, otype, minU, maxU);

    qint64 minV = 0;
    qint64 maxV = 0;
    this->limitsUV(graybits, otype, minV, maxV);

    qint64 ciu = rounding + shiftDiv * (minU + maxU) / 2;
    qint64 civ = rounding + shiftDiv * (minV + maxV) / 2;

    self->a00 = k; self->a01 = 0; self->a02 = rounding;
    self->a10 = 0; self->a11 = 0; self->a12 = ciu;
    self->a20 = 0; self->a21 = 0; self->a22 = civ;
}

#include "moc_akcolorconvert.cpp"
