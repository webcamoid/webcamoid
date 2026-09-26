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

#ifndef  VIDEOENCODERFFMPEGELEMENT_H
#define  VIDEOENCODERFFMPEGELEMENT_H

#include <iak/akvideoencoder.h>

class VideoEncoderFFmpegElementPrivate;

class VideoEncoderFFmpegElement: public AkVideoEncoder
{
    Q_OBJECT
    Q_PROPERTY(bool globalHeaders
               READ globalHeaders
               WRITE setGlobalHeaders
               RESET resetGlobalHeaders
               NOTIFY globalHeadersChanged)
    Q_PROPERTY(size_t frameBufferSize
               READ frameBufferSize
               WRITE setFrameBufferSize
               RESET resetFrameBufferSize
               NOTIFY frameBufferSizeChanged)

    public:
        VideoEncoderFFmpegElement();
        ~VideoEncoderFFmpegElement();

        Q_INVOKABLE QStringList codecs() const override;
        Q_INVOKABLE AkVideoEncoderCodecID codecID(const QString &codec) const override;
        Q_INVOKABLE QString codecDescription(const QString &codec) const override;
        Q_INVOKABLE AkCompressedVideoCaps outputCaps() const override;
        Q_INVOKABLE QByteArray headers() const override;
        Q_INVOKABLE qint64 encodedTimePts() const override;
        Q_INVOKABLE size_t frameBufferSize() const;
        Q_INVOKABLE bool globalHeaders() const;
        Q_INVOKABLE AkPropertyOptions options() const override;
        Q_INVOKABLE bool hasHardwareSupport(const QString &codec) const override;

    private:
        VideoEncoderFFmpegElementPrivate *d;
        bool checkDiscardFrame(const AkVideoPacket &packet) const;
        void applyRegulateFps(const AkVideoPacket &packet);

    protected:
        AkPacket iVideoStream(const AkVideoPacket &packet) override;
        void encodeFrame(const AkVideoPacket &packet) override;

    signals:
        void globalHeadersChanged(bool globalHeaders);
        void frameBufferSizeChanged(size_t frameBufferSize);

    public slots:
        void setGlobalHeaders(bool globalHeaders);
        void setFrameBufferSize(size_t frameBufferSize);
        void resetGlobalHeaders();
        void resetFrameBufferSize();
        bool setState(AkElement::ElementState state) override;

    friend class VideoEncoderFFmpegElementPrivate;
};

#endif // VIDEOENCODERFFMPEGELEMENT_H
