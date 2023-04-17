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

#include "akcolorconvert.h"

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
    Q_PROPERTY(AkColorConvert::YuvColorSpace yuvColorSpace
               READ yuvColorSpace
               WRITE setYuvColorSpace
               RESET resetYuvColorSpace
               NOTIFY yuvColorSpaceChanged)
    Q_PROPERTY(AkColorConvert::YuvColorSpaceType yuvColorSpaceType
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
    Q_PROPERTY(QRect inputRect
               READ inputRect
               WRITE setInputRect
               RESET resetInputRect
               NOTIFY inputRectChanged)

    public:
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
        Q_INVOKABLE AkColorConvert::YuvColorSpace yuvColorSpace() const;
        Q_INVOKABLE AkColorConvert::YuvColorSpaceType yuvColorSpaceType() const;
        Q_INVOKABLE AkVideoConverter::ScalingMode scalingMode() const;
        Q_INVOKABLE AkVideoConverter::AspectRatioMode aspectRatioMode() const;
        Q_INVOKABLE QRect inputRect() const;

        Q_INVOKABLE bool begin();
        Q_INVOKABLE void end();
        Q_INVOKABLE AkVideoPacket convert(const AkVideoPacket &packet);

    private:
        AkVideoConverterPrivate *d;

    Q_SIGNALS:
        void outputCapsChanged(const AkVideoCaps &outputCaps);
        void yuvColorSpaceChanged(AkColorConvert::YuvColorSpace yuvColorSpace);
        void yuvColorSpaceTypeChanged(AkColorConvert::YuvColorSpaceType yuvColorSpaceType);
        void scalingModeChanged(AkVideoConverter::ScalingMode scalingMode);
        void aspectRatioModeChanged(AkVideoConverter::AspectRatioMode aspectRatioMode);
        void inputRectChanged(const QRect &inputRect);

    public Q_SLOTS:
        void setCacheIndex(int index);
        void setOutputCaps(const AkVideoCaps &outputCaps);
        void setYuvColorSpace(AkColorConvert::YuvColorSpace yuvColorSpace);
        void setYuvColorSpaceType(AkColorConvert::YuvColorSpaceType yuvColorSpaceType);
        void setScalingMode(AkVideoConverter::ScalingMode scalingMode);
        void setAspectRatioMode(AkVideoConverter::AspectRatioMode aspectRatioMode);
        void setInputRect(const QRect &inputRect);
        void resetOutputCaps();
        void resetYuvColorSpace();
        void resetYuvColorSpaceType();
        void resetScalingMode();
        void resetAspectRatioMode();
        void resetInputRect();
        void reset();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkVideoConverter::ScalingMode mode);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkVideoConverter::AspectRatioMode mode);

Q_DECLARE_METATYPE(AkVideoConverter)
Q_DECLARE_METATYPE(AkVideoConverter::ScalingMode)
Q_DECLARE_METATYPE(AkVideoConverter::AspectRatioMode)

#endif // AKVIDEOCONVERTER_H
