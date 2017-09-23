/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef MEDIAWRITERFFMPEG_H
#define MEDIAWRITERFFMPEG_H

#include <QtConcurrent>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <akaudiopacket.h>
#include <akvideopacket.h>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/opt.h>
    #include <libavutil/parseutils.h>
    #include <libavutil/mathematics.h>
}

#include "mediawriter.h"
#include "abstractstream.h"

class MediaWriterFFmpeg: public MediaWriter
{
    Q_OBJECT

    public:
        explicit MediaWriterFFmpeg(QObject *parent=nullptr);
        ~MediaWriterFFmpeg();

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
        QString m_outputFormat;
        QMap<QString, QVariantMap> m_formatOptions;
        QMap<QString, QVariantMap> m_codecOptions;
        QList<QVariantMap> m_streamConfigs;
        AVFormatContext *m_formatContext;
        QThreadPool m_threadPool;
        qint64 m_maxPacketQueueSize;
        bool m_isRecording;
        QMutex m_packetMutex;
        QMutex m_audioMutex;
        QMutex m_videoMutex;
        QMutex m_subtitleMutex;
        QMutex m_writeMutex;
        QMap<int, AbstractStreamPtr> m_streamsMap;

        QString guessFormat();
        QVariantList parseOptions(const AVClass *avClass) const;
        AVDictionary *formatContextOptions(AVFormatContext *formatContext,
                                           const QVariantMap &options);

        AkVideoCaps nearestDVCaps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestDNxHDCaps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestH261Caps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestH263Caps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestGXFCaps(const AkVideoCaps &caps) const;
        AkAudioCaps nearestSWFCaps(const AkAudioCaps &caps) const;

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
        void writePacket(AVPacket *packet);

    friend class VideoStream;
    friend class AudioStream;
};

#endif // MEDIAWRITERFFMPEG_H
