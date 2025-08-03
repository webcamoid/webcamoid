/* Webcamoid, camera capture application.
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

#ifndef VIDEOMUXERFFMPEGELEMENT_H
#define VIDEOMUXERFFMPEGELEMENT_H

#include <iak/akvideomuxer.h>

class VideoMuxerFFmpegElementPrivate;

class VideoMuxerFFmpegElement: public AkVideoMuxer
{
    Q_OBJECT

    public:
        VideoMuxerFFmpegElement();
        ~VideoMuxerFFmpegElement();

        Q_INVOKABLE QStringList muxers() const override;
        Q_INVOKABLE FormatID formatID(const QString &muxer) const override;
        Q_INVOKABLE QString description(const QString &muxer) const override;
        Q_INVOKABLE QString extension(const QString &muxer) const override;
        Q_INVOKABLE bool gapsAllowed(AkCodecType type) const override;
        Q_INVOKABLE QList<AkCodecID> supportedCodecs(const QString &muxer,
                                                     AkCodecType type) const override;
        Q_INVOKABLE AkCodecID defaultCodec(const QString &muxer,
                                           AkCodecType type) const override;
        Q_INVOKABLE AkPropertyOptions options() const override;

    private:
        VideoMuxerFFmpegElementPrivate *d;

    public slots:
        AkPacket iStream(const AkPacket &packet) override;
        bool setState(AkElement::ElementState state) override;
};

#endif // VIDEOMUXERFFMPEGELEMENT_H
