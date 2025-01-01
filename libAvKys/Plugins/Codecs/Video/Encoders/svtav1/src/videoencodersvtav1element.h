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

#ifndef VIDEOENCODERSVTAV1ELEMENT_H
#define VIDEOENCODERSVTAV1ELEMENT_H

#include <iak/akvideoencoder.h>

class VideoEncoderSvtAv1ElementPrivate;

class VideoEncoderSvtAv1Element: public AkVideoEncoder
{
    Q_OBJECT
    Q_PROPERTY(int speed
               READ speed
               WRITE setSpeed
               RESET resetSpeed
               NOTIFY speedChanged)
    Q_PROPERTY(TuneContent tuneContent
               READ tuneContent
               WRITE setTuneContent
               RESET resetTuneContent
               NOTIFY tuneContentChanged)

    public:
        enum TuneContent
        {
            TuneContent_VQ,
            TuneContent_PSNR,
            TuneContent_SSIM,
        };
        Q_ENUM(TuneContent)

        VideoEncoderSvtAv1Element();
        ~VideoEncoderSvtAv1Element();

        Q_INVOKABLE QStringList codecs() const override;
        Q_INVOKABLE AkVideoEncoderCodecID codecID(const QString &codec) const override;
        Q_INVOKABLE QString codecDescription(const QString &codec) const override;
        Q_INVOKABLE AkCompressedVideoCaps outputCaps() const override;
        Q_INVOKABLE AkCompressedPackets headers() const override;
        Q_INVOKABLE qint64 encodedTimePts() const override;
        Q_INVOKABLE int speed() const;
        Q_INVOKABLE TuneContent tuneContent() const;

    private:
        VideoEncoderSvtAv1ElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void speedChanged(int speed);
        void tuneContentChanged(TuneContent tuneContent);

    public slots:
        void setSpeed(int speed);
        void setTuneContent(TuneContent tuneContent);
        void resetSpeed();
        void resetTuneContent();
        void resetOptions() override;
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(VideoEncoderSvtAv1Element::TuneContent)

#endif // VIDEOENCODERSVTAV1ELEMENT_H
