/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef MEDIAWRITERGSTREAMER_H
#define MEDIAWRITERGSTREAMER_H

#include "mediawriter.h"

class MediaWriterGStreamerPrivate;
class AkAudioPacket;
class AkVideoPacket;

class MediaWriterGStreamer: public MediaWriter
{
    Q_OBJECT

    public:
        MediaWriterGStreamer(QObject *parent=nullptr);
        ~MediaWriterGStreamer();

        Q_INVOKABLE QString outputFormat() const;
        Q_INVOKABLE QVariantList streams() const;

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
        MediaWriterGStreamerPrivate *d;

    public slots:
        void setOutputFormat(const QString &outputFormat);
        void setFormatOptions(const QVariantMap &formatOptions);
        void setCodecOptions(int index, const QVariantMap &codecOptions);
        void resetOutputFormat();
        void resetFormatOptions();
        void resetCodecOptions(int index);
        void enqueuePacket(const AkPacket &packet);
        void clearStreams();
        bool init();
        void uninit();

    private slots:
        void writeAudioPacket(const AkAudioPacket &packet);
        void writeVideoPacket(const AkVideoPacket &packet);
        void writeSubtitlePacket(const AkPacket &packet);

        friend class MediaWriterGStreamerPrivate;
};

#endif // MEDIAWRITERGSTREAMER_H
