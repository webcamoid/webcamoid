/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef VIDEOENCODERAV1ELEMENT_H
#define VIDEOENCODERAV1ELEMENT_H

#include <iak/akvideoencoder.h>

class VideoEncoderAv1ElementPrivate;

class VideoEncoderAv1Element: public AkVideoEncoder
{
    Q_OBJECT
    Q_FLAGS(ErrorResilientFlag)
    Q_PROPERTY(ErrorResilientFlag errorResilient
               READ errorResilient
               WRITE setErrorResilient
               RESET resetErrorResilient
               NOTIFY errorResilientChanged)
    Q_PROPERTY(int speed
               READ speed
               WRITE setSpeed
               RESET resetSpeed
               NOTIFY speedChanged)
    Q_PROPERTY(Usage usage
               READ usage
               WRITE setUsage
               RESET resetUsage
               NOTIFY usageChanged)
    Q_PROPERTY(bool lossless
               READ lossless
               WRITE setLossless
               RESET resetLossless
               NOTIFY losslessChanged)
    Q_PROPERTY(TuneContent tuneContent
               READ tuneContent
               WRITE setTuneContent
               RESET resetTuneContent
               NOTIFY tuneContentChanged)

    public:
        enum ErrorResilientFlag
        {
            ErrorResilientFlag_NoFlags    = 0x0,
            ErrorResilientFlag_Default    = 0x1,
            ErrorResilientFlag_Partitions = 0x2,
        };
        Q_DECLARE_FLAGS(ErrorResilientFlags, ErrorResilientFlag)
        Q_FLAG(ErrorResilientFlags)
        Q_ENUM(ErrorResilientFlag)

        enum Usage
        {
            Usage_GoodQuality,
            Usage_RealTime,
            Usage_AllIntra,
        };
        Q_ENUM(Usage)

        enum TuneContent
        {
            TuneContent_Default,
            TuneContent_Screen,
            TuneContent_Film,
        };
        Q_ENUM(TuneContent)

        VideoEncoderAv1Element();
        ~VideoEncoderAv1Element();

        Q_INVOKABLE QStringList codecs() const override;
        Q_INVOKABLE AkVideoEncoderCodecID codecID(const QString &codec) const override;
        Q_INVOKABLE QString codecDescription(const QString &codec) const override;
        Q_INVOKABLE AkCompressedVideoCaps outputCaps() const override;
        Q_INVOKABLE AkCompressedPackets headers() const override;
        Q_INVOKABLE qint64 encodedTimePts() const override;
        Q_INVOKABLE ErrorResilientFlag errorResilient() const;
        Q_INVOKABLE int speed() const;
        Q_INVOKABLE Usage usage() const;
        Q_INVOKABLE bool lossless() const;
        Q_INVOKABLE TuneContent tuneContent() const;

    private:
        VideoEncoderAv1ElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void errorResilientChanged(ErrorResilientFlag errorResilient);
        void speedChanged(int speed);
        void usageChanged(Usage usage);
        void losslessChanged(bool lossless);
        void tuneContentChanged(TuneContent tuneContent);

    public slots:
        void setErrorResilient(ErrorResilientFlag errorResilient);
        void setSpeed(int speed);
        void setUsage(Usage usage);
        void setLossless(bool lossless);
        void setTuneContent(TuneContent tuneContent);
        void resetErrorResilient();
        void resetSpeed();
        void resetUsage();
        void resetLossless();
        void resetTuneContent();
        void resetOptions() override;
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(VideoEncoderAv1Element::ErrorResilientFlag)
Q_DECLARE_METATYPE(VideoEncoderAv1Element::Usage)
Q_DECLARE_METATYPE(VideoEncoderAv1Element::TuneContent)

#endif // VIDEOENCODERAV1ELEMENT_H
