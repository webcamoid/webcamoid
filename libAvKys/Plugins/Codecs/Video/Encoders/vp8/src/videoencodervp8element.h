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

#ifndef VIDEOENCODERVP8ELEMENT_H
#define VIDEOENCODERVP8ELEMENT_H

#include <iak/akvideoencoder.h>

class VideoEncoderVp8ElementPrivate;

class VideoEncoderVp8Element: public AkVideoEncoder
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

        VideoEncoderVp8Element();
        ~VideoEncoderVp8Element();

        Q_INVOKABLE AkVideoEncoderCodecID codec() const override;
        Q_INVOKABLE ErrorResilientFlag errorResilient() const;
        Q_INVOKABLE int deadline() const;

    private:
        VideoEncoderVp8ElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void errorResilientChanged(ErrorResilientFlag errorResilient);
        void deadlineChanged(int deadline);

    public slots:
        void setErrorResilient(ErrorResilientFlag errorResilient);
        void setDeadline(int deadline);
        void resetErrorResilient();
        void resetDeadline();
        void resetOptions() override;
        bool setState(AkElement::ElementState state) override;
};

Q_DECLARE_METATYPE(VideoEncoderVp8Element::ErrorResilientFlag)
Q_DECLARE_METATYPE(VideoEncoderVp8Element::Deadline)

#endif // VIDEOENCODERVP8ELEMENT_H
