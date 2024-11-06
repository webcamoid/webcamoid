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

#ifndef VIDEOMUXERWEBMELEMENT_H
#define VIDEOMUXERWEBMELEMENT_H

#include <iak/akvideomuxer.h>

class VideoMuxerWebmElementPrivate;

class VideoMuxerWebmElement: public AkVideoMuxer
{
    Q_OBJECT

    public:
        VideoMuxerWebmElement();
        ~VideoMuxerWebmElement();

        Q_INVOKABLE FormatID formatID() const override;
        Q_INVOKABLE QString extension() const override;
        Q_INVOKABLE QList<AkCodecID> supportedCodecs(AkCompressedCaps::CapsType type) const override;
        Q_INVOKABLE AkCodecID defaultCodec(AkCompressedCaps::CapsType type) const override;

    private:
        VideoMuxerWebmElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iCompressedAudioStream(const AkCompressedAudioPacket &packet) override;
        AkPacket iCompressedVideoStream(const AkCompressedVideoPacket &packet) override;

    signals:

    public slots:
        void resetOptions() override;
        bool setState(AkElement::ElementState state) override;
};

#endif // VIDEOMUXERWEBMELEMENT_H
