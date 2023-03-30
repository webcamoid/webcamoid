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

#ifndef AKVIDEOCONVERTER_H
#define AKVIDEOCONVERTER_H

#include <QObject>

#include "akcommons.h"

class AkVideoConverterPrivate;
class AkVideoCaps;
class AkVideoPacket;

class AKCOMMONS_EXPORT AkVideoConverter: public QObject
{
    Q_OBJECT
    Q_PROPERTY(AkVideoCaps outputCaps
               READ outputCaps
               WRITE setOutputCaps
               RESET resetOutputCaps
               NOTIFY outputCapsChanged)
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
    Q_PROPERTY(AkVideoConverter::ScalingMode scalingMode
               READ scalingMode
               WRITE setScalingMode
               RESET resetScalingMode
               NOTIFY scalingModeChanged)
    Q_PROPERTY(AkVideoConverter::AspectRatioMode aspectRatioMode
               READ aspectRatioMode
               WRITE setAspectRatioMode
               RESET resetAspectRatioMode
               NOTIFY aspectRatioModeChanged)

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

        enum ScalingMode {
            ScalingMode_Fast,
            ScalingMode_Linear
        };
        Q_ENUM(ScalingMode)

        enum AspectRatioMode {
            AspectRatioMode_Ignore,
            AspectRatioMode_Keep,
            AspectRatioMode_Expanding
        };
        Q_ENUM(AspectRatioMode)

        AkVideoConverter(QObject *parent=nullptr);
        AkVideoConverter(const AkVideoCaps &outputCaps,
                         QObject *parent=nullptr);
        AkVideoConverter(const AkVideoConverter &other);
        ~AkVideoConverter();
        AkVideoConverter &operator =(const AkVideoConverter &other);

        Q_INVOKABLE static QObject *create();

        Q_INVOKABLE AkVideoCaps outputCaps() const;
        Q_INVOKABLE YuvColorSpace yuvColorSpace() const;
        Q_INVOKABLE YuvColorSpaceType yuvColorSpaceType() const;
        Q_INVOKABLE AkVideoConverter::ScalingMode scalingMode() const;
        Q_INVOKABLE AkVideoConverter::AspectRatioMode aspectRatioMode() const;
        Q_INVOKABLE bool begin();
        Q_INVOKABLE void end();
        Q_INVOKABLE AkVideoPacket convert(const AkVideoPacket &packet);

    private:
        AkVideoConverterPrivate *d;

    Q_SIGNALS:
        void outputCapsChanged(const AkVideoCaps &outputCaps);
        void yuvColorSpaceChanged(YuvColorSpace yuvColorSpace);
        void yuvColorSpaceTypeChanged(YuvColorSpaceType yuvColorSpaceType);
        void scalingModeChanged(AkVideoConverter::ScalingMode scalingMode);
        void aspectRatioModeChanged(AkVideoConverter::AspectRatioMode aspectRatioMode);

    public Q_SLOTS:
        void setCacheIndex(int index);
        void setOutputCaps(const AkVideoCaps &outputCaps);
        void setYuvColorSpace(YuvColorSpace yuvColorSpace);
        void setYuvColorSpaceType(YuvColorSpaceType yuvColorSpaceType);
        void setScalingMode(AkVideoConverter::ScalingMode scalingMode);
        void setAspectRatioMode(AkVideoConverter::AspectRatioMode aspectRatioMode);
        void resetOutputCaps();
        void resetYuvColorSpace();
        void resetYuvColorSpaceType();
        void resetScalingMode();
        void resetAspectRatioMode();
        void reset();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkVideoConverter::YuvColorSpace yuvColorSpace);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkVideoConverter::YuvColorSpaceType yuvColorSpaceType);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkVideoConverter::ScalingMode mode);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkVideoConverter::AspectRatioMode mode);

Q_DECLARE_METATYPE(AkVideoConverter)
Q_DECLARE_METATYPE(AkVideoConverter::YuvColorSpace)
Q_DECLARE_METATYPE(AkVideoConverter::YuvColorSpaceType)
Q_DECLARE_METATYPE(AkVideoConverter::ScalingMode)
Q_DECLARE_METATYPE(AkVideoConverter::AspectRatioMode)

#endif // AKVIDEOCONVERTER_H
