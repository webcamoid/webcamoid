/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#ifndef MEDIAWRITERNDKMEDIA_H
#define MEDIAWRITERNDKMEDIA_H

#include "mediawriter.h"

class MediaWriterNDKMediaPrivate;
class AkAudioCaps;
class AkVideoCaps;

class MediaWriterNDKMedia: public MediaWriter
{
    Q_OBJECT

    public:
        MediaWriterNDKMedia(QObject *parent=nullptr);
        ~MediaWriterNDKMedia();

        Q_INVOKABLE QString outputFormat() const;
        Q_INVOKABLE QVariantList streams() const;
        Q_INVOKABLE qint64 maxPacketQueueSize() const;

        Q_INVOKABLE QStringList supportedFormats();
        Q_INVOKABLE QStringList fileExtensions(const QString &format);
        Q_INVOKABLE QString formatDescription(const QString &format);
        Q_INVOKABLE QVariantList formatOptions();
        Q_INVOKABLE QStringList supportedCodecs(const QString &format);
        Q_INVOKABLE QStringList supportedCodecs(const QString &format,
                                                const QString &type);
        Q_INVOKABLE QString defaultCodec(const QString &format,
                                         const QString &type);
        Q_INVOKABLE QString codecDescription(const QString &codec);
        Q_INVOKABLE QString codecType(const QString &codec);
        Q_INVOKABLE QVariantMap defaultCodecParams(const QString &codec);
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps);
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps,
                                          const QVariantMap &codecParams);
        Q_INVOKABLE QVariantMap updateStream(int index);
        Q_INVOKABLE QVariantMap updateStream(int index,
                                             const QVariantMap &codecParams);
        Q_INVOKABLE QVariantList codecOptions(int index);

    private:
        MediaWriterNDKMediaPrivate *d;

    public slots:
        void setOutputFormat(const QString &outputFormat);
        void setFormatOptions(const QVariantMap &formatOptions);
        void setCodecOptions(int index, const QVariantMap &codecOptions);
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void resetOutputFormat();
        void resetFormatOptions();
        void resetCodecOptions(int index);
        void resetMaxPacketQueueSize();
        void enqueuePacket(const AkPacket &packet);
        void clearStreams();
        bool init();
        void uninit();

    private slots:
        void writePacket(const AkPacket &packet);

    friend class VideoStream;
    friend class AudioStream;
};

#endif // MEDIAWRITERNDKMEDIA_H
