/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef MEDIASOURCE_H
#define MEDIASOURCE_H

#include <QtConcurrent>
#include <gst/gst.h>

#include "stream.h"

class MediaSource: public QObject
{
    Q_OBJECT

    public:
        explicit MediaSource(QObject *parent=NULL);
        ~MediaSource();

        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE QList<int> listTracks(const QString &mimeType);
        Q_INVOKABLE QString streamLanguage(int stream);
        Q_INVOKABLE bool loop() const;

        Q_INVOKABLE int defaultStream(const QString &mimeType);
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE QbCaps caps(int stream);
        Q_INVOKABLE qint64 maxPacketQueueSize() const;
        Q_INVOKABLE bool showLog() const;

    private:
        QString m_media;
        QList<int> m_streams;
        bool m_loop;
        bool m_run;

        qint64 m_maxPacketQueueSize;
        bool m_showLog;
        QThreadPool m_threadPool;
        GstElement *m_pipeline;
        GMainLoop *m_mainLoop;
        guint m_busWatchId;
        qint64 m_audioIndex;
        qint64 m_videoIndex;
        qint64 m_subtitlesIndex;
        qint64 m_audioId;
        qint64 m_videoId;
        qint64 m_subtitlesId;
        QList<Stream> m_streamInfo;

        void waitState(GstState state);

        static gboolean busCallback(GstBus *bus,
                                    GstMessage *message,
                                    gpointer userData);
        static GstFlowReturn audioBufferCallback(GstElement *audioOutput,
                                                 gpointer userData);
        static GstFlowReturn videoBufferCallback(GstElement *videoOutput,
                                                 gpointer userData);
        static GstFlowReturn subtitlesBufferCallback(GstElement *subtitlesOutput,
                                                     gpointer userData);
        static void aboutToFinish(GstElement *object, gpointer userData);
        QStringList languageCodes(const QString &type);
        QStringList languageCodes();

    signals:
        void oStream(const QbPacket &packet);
        void error(const QString &message);
        void maxPacketQueueSizeChanged(qint64 maxPacketQueue);
        void showLogChanged(bool showLog);
        void loopChanged(bool loop);
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);

    public slots:
        void setMedia(const QString &media);
        void setStreams(const QList<int> &streams);
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void setShowLog(bool showLog);
        void setLoop(bool loop);
        void resetMedia();
        void resetStreams();
        void resetMaxPacketQueueSize();
        void resetShowLog();
        void resetLoop();
        bool init(bool paused=false);
        void uninit();

    private slots:
        void updateStreams();
};

#endif // MEDIASOURCE_H
