/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "mediawriter.h"
#include "outputparams.h"

class MediaWriterFFmpeg: public MediaWriter
{
    Q_OBJECT

    public:
        explicit MediaWriterFFmpeg(QObject *parent=NULL);
        ~MediaWriterFFmpeg();

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE QString outputFormat() const;
        Q_INVOKABLE QVariantMap formatOptions() const;
        Q_INVOKABLE QVariantList streams() const;
        Q_INVOKABLE qint64 maxPacketQueueSize() const;

        Q_INVOKABLE QStringList supportedFormats();
        Q_INVOKABLE QStringList fileExtensions(const QString &format);
        Q_INVOKABLE QString formatDescription(const QString &format);
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

    private:
        QString m_location;
        QString m_outputFormat;
        QVariantMap m_formatOptions;
        QList<QVariantMap> m_streamConfigs;
        QList<OutputParams> m_streamParams;
        AVFormatContext *m_formatContext;
        QThreadPool m_threadPool;
        qint64 m_packetQueueSize;
        qint64 m_maxPacketQueueSize;
        bool m_runAudioLoop;
        bool m_runVideoLoop;
        bool m_runSubtitleLoop;
        bool m_isRecording;
        QMutex m_packetMutex;
        QMutex m_audioMutex;
        QMutex m_videoMutex;
        QMutex m_subtitleMutex;
        QMutex m_writeMutex;
        QWaitCondition m_audioQueueNotEmpty;
        QWaitCondition m_videoQueueNotEmpty;
        QWaitCondition m_subtitleQueueNotEmpty;
        QWaitCondition m_packetQueueNotFull;
        QQueue<AkAudioPacket> m_audioPackets;
        QQueue<AkVideoPacket> m_videoPackets;
        QQueue<AkPacket> m_subtitlePackets;
        QFuture<void> m_audioLoopResult;
        QFuture<void> m_videoLoopResult;
        QFuture<void> m_subtitleLoopResult;

        void flushStreams();
        QImage swapChannels(const QImage &image) const;

        AkVideoCaps nearestDVCaps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestDNxHDCaps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestH261Caps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestH263Caps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestGXFCaps(const AkVideoCaps &caps) const;
        AkAudioCaps nearestSWFCaps(const AkAudioCaps &caps) const;

        static void writeAudioLoop(MediaWriterFFmpeg *self);
        static void writeVideoLoop(MediaWriterFFmpeg *self);
        static void writeSubtitleLoop(MediaWriterFFmpeg *self);
        void decreasePacketQueue(int packetSize);

    public slots:
        void setLocation(const QString &location);
        void setOutputFormat(const QString &outputFormat);
        void setFormatOptions(const QVariantMap &formatOptions);
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void resetLocation();
        void resetOutputFormat();
        void resetFormatOptions();
        void resetMaxPacketQueueSize();
        void enqueuePacket(const AkPacket &packet);
        void clearStreams();
        bool init();
        void uninit();

    private slots:
        void writeAudioPacket(const AkAudioPacket &packet);
        void writeVideoPacket(const AkVideoPacket &packet);
        void writeSubtitlePacket(const AkPacket &packet);
        void updateStreams();
};

#endif // MEDIAWRITERFFMPEG_H
