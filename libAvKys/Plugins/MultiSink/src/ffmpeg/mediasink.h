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

#ifndef MEDIASINK_H
#define MEDIASINK_H

#include <QtConcurrent>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

#include "outputparams.h"

class MediaSink: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString location
               READ location
               WRITE setLocation
               RESET resetLocation
               NOTIFY locationChanged)
    Q_PROPERTY(QString outputFormat
               READ outputFormat
               WRITE setOutputFormat
               RESET resetOutputFormat
               NOTIFY outputFormatChanged)
    Q_PROPERTY(QVariantMap formatOptions
               READ formatOptions
               WRITE setFormatOptions
               RESET resetFormatOptions
               NOTIFY formatOptionsChanged)
    Q_PROPERTY(QVariantList streams
               READ streams
               NOTIFY streamsChanged)
    Q_PROPERTY(qint64 maxPacketQueueSize
               READ maxPacketQueueSize
               WRITE setMaxPacketQueueSize
               RESET resetMaxPacketQueueSize
               NOTIFY maxPacketQueueSizeChanged)

    public:
        explicit MediaSink(QObject *parent=NULL);
        ~MediaSink();

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE QString outputFormat() const;
        Q_INVOKABLE QVariantMap formatOptions() const;
        Q_INVOKABLE QVariantList streams() const;
        Q_INVOKABLE qint64 maxPacketQueueSize() const;

        Q_INVOKABLE QStringList supportedFormats();
        Q_INVOKABLE QStringList fileExtensions(const QString &format);
        Q_INVOKABLE QString formatDescription(const QString &format);
        Q_INVOKABLE QStringList supportedCodecs(const QString &format,
                                                const QString &type="");
        Q_INVOKABLE QString defaultCodec(const QString &format,
                                         const QString &type);
        Q_INVOKABLE QString codecDescription(const QString &codec);
        Q_INVOKABLE QString codecType(const QString &codec);
        Q_INVOKABLE QVariantMap defaultCodecParams(const QString &codec);
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps,
                                          const QVariantMap &codecParams=QVariantMap());
        Q_INVOKABLE QVariantMap updateStream(int index,
                                             const QVariantMap &codecParams=QVariantMap());

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
        AkVideoCaps nearestDVCaps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestDNxHDCaps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestH261Caps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestH263Caps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestGXFCaps(const AkVideoCaps &caps) const;
        AkAudioCaps nearestSWFCaps(const AkAudioCaps &caps) const;

        static void writeAudioLoop(MediaSink *self);
        static void writeVideoLoop(MediaSink *self);
        static void writeSubtitleLoop(MediaSink *self);
        void decreasePacketQueue(int packetSize);

    signals:
        void locationChanged(const QString &location);
        void outputFormatChanged(const QString &outputFormat);
        void formatOptionsChanged(const QVariantMap &formatOptions);
        void streamsChanged(const QVariantList &streams);
        void maxPacketQueueSizeChanged(qint64 maxPacketQueueSize);
        void streamUpdated(int index);

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

#endif // MEDIASINK_H
