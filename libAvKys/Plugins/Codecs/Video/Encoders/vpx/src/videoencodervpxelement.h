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

#ifndef VIDEOENCODERVPXELEMENT_H
#define VIDEOENCODERVPXELEMENT_H

#include <iak/akvideoencoder.h>

class VideoEncoderVpxElementPrivate;

class VideoEncoderVpxElement: public AkVideoEncoder
{
    Q_OBJECT
    Q_FLAGS(ErrorResilientFlag)
    Q_PROPERTY(ErrorResilientFlag errorResilient
               READ errorResilient
               WRITE setErrorResilient
               RESET resetErrorResilient
               NOTIFY errorResilientChanged)
    Q_PROPERTY(int deadline
               READ deadline
               WRITE setDeadline
               RESET resetDeadline
               NOTIFY deadlineChanged)
    Q_PROPERTY(int speed
               READ speed
               WRITE setSpeed
               RESET resetSpeed
               NOTIFY speedChanged)
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

        enum Deadline
        {
            Deadline_BestQuality = 0,
            Deadline_Realtime    = 1,
            Deadline_GoodQuality = 1000000,
        };
        Q_ENUM(Deadline)

        enum TuneContent
        {
            TuneContent_Default,
            TuneContent_Screen,
            TuneContent_Film,
        };
        Q_ENUM(TuneContent)

        VideoEncoderVpxElement();
        ~VideoEncoderVpxElement();

        Q_INVOKABLE AkVideoEncoderCodecID codec() const override;
        Q_INVOKABLE AkCompressedVideoCaps outputCaps() const override;
        Q_INVOKABLE AkCompressedPackets headers() const override;
        Q_INVOKABLE ErrorResilientFlag errorResilient() const;
        Q_INVOKABLE int deadline() const;
        Q_INVOKABLE int speed() const;
        Q_INVOKABLE bool lossless() const;
        Q_INVOKABLE TuneContent tuneContent() const;

    private:
        VideoEncoderVpxElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void errorResilientChanged(ErrorResilientFlag errorResilient);
        void deadlineChanged(int deadline);
        void speedChanged(int speed);
        void losslessChanged(bool lossless);
        void tuneContentChanged(TuneContent tuneContent);

    public slots:
        void setErrorResilient(ErrorResilientFlag errorResilient);
        void setDeadline(int deadline);
        void setSpeed(int speed);
        void setLossless(bool lossless);
        void setTuneContent(TuneContent tuneContent);
        void resetErrorResilient();
        void resetDeadline();
        void resetSpeed();
        void resetLossless();
        void resetTuneContent();
        void resetOptions() override;
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(VideoEncoderVpxElement::ErrorResilientFlag)
Q_DECLARE_METATYPE(VideoEncoderVpxElement::Deadline)
Q_DECLARE_METATYPE(VideoEncoderVpxElement::TuneContent)

#endif // VIDEOENCODERVPXELEMENT_H
