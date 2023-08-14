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

#ifndef AKCOLORCONVERT_H
#define AKCOLORCONVERT_H

#include "akvideocaps.h"
#include "akvideoformatspec.h"

class AkColorConvertPrivate;

class AKCOMMONS_EXPORT AkColorConvert: public QObject
{
    Q_OBJECT
    Q_PROPERTY(YuvColorSpace yuvColorSpace
               READ yuvColorSpace
               WRITE setYuvColorSpace
               RESET resetYuvColorSpace
               NOTIFY yuvColorSpaceChanged)
    Q_PROPERTY(YuvColorSpaceType yuvColorSpaceType
               READ yuvColorSpaceType
               WRITE setYuvColorSpaceType
               RESET resetYuvColorSpaceType
               NOTIFY yuvColorSpaceTypeChanged)

    public:
        enum YuvColorSpace
        {
            YuvColorSpace_AVG,
            YuvColorSpace_ITUR_BT601,
            YuvColorSpace_ITUR_BT709,
            YuvColorSpace_ITUR_BT2020,
            YuvColorSpace_SMPTE_240M
        };
        Q_ENUM(YuvColorSpace)

        enum YuvColorSpaceType
        {
            YuvColorSpaceType_StudioSwing,
            YuvColorSpaceType_FullSwing
        };
        Q_ENUM(YuvColorSpaceType)

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
        Q_ENUM(ColorMatrix)

        AkColorConvert(QObject *parent=nullptr);
        AkColorConvert(YuvColorSpace yuvColorSpace,
                       YuvColorSpaceType yuvColorSpaceType=YuvColorSpaceType_StudioSwing,
                       QObject *parent=nullptr);
        AkColorConvert(YuvColorSpaceType yuvColorSpaceType,
                       QObject *parent=nullptr);
        AkColorConvert(const AkColorConvert &other);
        ~AkColorConvert();
        AkColorConvert &operator =(const AkColorConvert &other);

        Q_INVOKABLE YuvColorSpace yuvColorSpace() const;
        Q_INVOKABLE YuvColorSpaceType yuvColorSpaceType() const;

        inline void applyMatrix(qint64 a, qint64 b, qint64 c,
                                qint64 *x, qint64 *y, qint64 *z) const
        {
            *x = qBound(this->xmin, (a * this->m00 + b * this->m01 + c * this->m02 + this->m03) >> this->colorShift, this->xmax);
            *y = qBound(this->ymin, (a * this->m10 + b * this->m11 + c * this->m12 + this->m13) >> this->colorShift, this->ymax);
            *z = qBound(this->zmin, (a * this->m20 + b * this->m21 + c * this->m22 + this->m23) >> this->colorShift, this->zmax);
        }

        inline void applyVector(qint64 a, qint64 b, qint64 c,
                                qint64 *x, qint64 *y, qint64 *z) const
        {
            *x = (a * this->m00 + this->m03) >> this->colorShift;
            *y = (b * this->m11 + this->m13) >> this->colorShift;
            *z = (c * this->m22 + this->m23) >> this->colorShift;
        }

        inline void applyPoint(qint64 p,
                               qint64 *x, qint64 *y, qint64 *z) const
        {
            *x = (p * this->m00 + this->m03) >> this->colorShift;
            *y = (p * this->m10 + this->m13) >> this->colorShift;
            *z = (p * this->m20 + this->m23) >> this->colorShift;
        }

        inline void applyPoint(qint64 a, qint64 b, qint64 c,
                               qint64 *p) const
        {
            *p = qBound(this->xmin, (a * this->m00 + b * this->m01 + c * this->m02 + this->m03) >> this->colorShift, this->xmax);
        }

        inline void applyPoint(qint64 p, qint64 *q) const
        {
            *q = (p * this->m00 + this->m03) >> this->colorShift;
        }

        inline void applyAlpha(qint64 x, qint64 y, qint64 z, qint64 a,
                               qint64 *xa, qint64 *ya, qint64 *za) const
        {
            *xa = qBound<qint64>(this->xmin, (a * (x * this->a00 + this->a01) + this->a02) >> this->alphaShift, this->xmax);
            *ya = qBound<qint64>(this->ymin, (a * (y * this->a10 + this->a11) + this->a12) >> this->alphaShift, this->ymax);
            *za = qBound<qint64>(this->zmin, (a * (z * this->a20 + this->a21) + this->a22) >> this->alphaShift, this->zmax);
        }

        inline void applyAlpha(qint64 a,
                               qint64 *x, qint64 *y, qint64 *z) const
        {
            this->applyAlpha(*x, *y, *z, a, x, y, z);
        }

        inline void applyAlpha(qint64 p, qint64 a, qint64 *pa) const
        {
            *pa = qBound<qint64>(this->ymin, (a * (p * this->a00 + this->a01) + this->a02) >> this->alphaShift, this->ymax);
        }

        inline void applyAlpha(qint64 a, qint64 *p) const
        {
            this->applyAlpha(*p, a, p);
        }

    private:
        AkColorConvertPrivate *d;

    protected:
        // Color matrix
        qint64 m00 {0}, m01 {0}, m02 {0}, m03 {0};
        qint64 m10 {0}, m11 {0}, m12 {0}, m13 {0};
        qint64 m20 {0}, m21 {0}, m22 {0}, m23 {0};

        // Alpha matrix
        qint64 a00 {0}, a01 {0}, a02 {0};
        qint64 a10 {0}, a11 {0}, a12 {0};
        qint64 a20 {0}, a21 {0}, a22 {0};

        qint64 xmin {0}, xmax {0};
        qint64 ymin {0}, ymax {0};
        qint64 zmin {0}, zmax {0};

        qint64 colorShift {0};
        qint64 alphaShift {0};

    Q_SIGNALS:
        void yuvColorSpaceChanged(YuvColorSpace yuvColorSpace);
        void yuvColorSpaceTypeChanged(YuvColorSpaceType yuvColorSpaceType);

    public Q_SLOTS:
        void setYuvColorSpace(YuvColorSpace yuvColorSpace);
        void setYuvColorSpaceType(YuvColorSpaceType yuvColorSpaceType);
        void resetYuvColorSpace();
        void resetYuvColorSpaceType();
        void loadColorMatrix(ColorMatrix colorMatrix,
                             int ibitsa,
                             int ibitsb,
                             int ibitsc,
                             int obitsx,
                             int obitsy,
                             int obitsz);
        void loadAlphaMatrix(AkVideoFormatSpec::VideoFormatType formatType,
                             int ibitsAlpha,
                             int obitsx,
                             int obitsy,
                             int obitsz);
        void loadMatrix(const AkVideoFormatSpec &from,
                        const AkVideoFormatSpec &to);
        void loadMatrix(const AkVideoCaps::PixelFormat &from,
                        const AkVideoCaps::PixelFormat &to);
        static void registerTypes();

    friend class AkColorConvertPrivate;
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkColorConvert::YuvColorSpace yuvColorSpace);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkColorConvert::YuvColorSpaceType yuvColorSpaceType);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkColorConvert::ColorMatrix colorMatrix);

Q_DECLARE_METATYPE(AkColorConvert)
Q_DECLARE_METATYPE(AkColorConvert::YuvColorSpace)
Q_DECLARE_METATYPE(AkColorConvert::YuvColorSpaceType)
Q_DECLARE_METATYPE(AkColorConvert::ColorMatrix)

#endif // AKCOLORCONVERT_H
