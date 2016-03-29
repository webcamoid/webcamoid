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

#ifndef CONVERTVIDEO_H
#define CONVERTVIDEO_H

#include <QtConcurrent>
#include <QQueue>
#include <QMutex>
#include <ak.h>
#include <akvideopacket.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/pixdesc.h>
}

#include "clock.h"

typedef QSharedPointer<AVFrame> FramePtr;

class ConvertVideo: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 maxPacketQueueSize
               READ maxPacketQueueSize
               WRITE setMaxPacketQueueSize
               RESET resetMaxPacketQueueSize
               NOTIFY maxPacketQueueSizeChanged)
    Q_PROPERTY(bool showLog
               READ showLog
               WRITE setShowLog
               RESET resetShowLog
               NOTIFY showLogChanged)

    public:
        explicit ConvertVideo(QObject *parent=NULL);
        ~ConvertVideo();

        Q_INVOKABLE qint64 maxPacketQueueSize() const;
        Q_INVOKABLE bool showLog() const;

        Q_INVOKABLE void packetEnqueue(const AkPacket &packet);
        Q_INVOKABLE void dataEnqueue(AVFrame *frame);
        Q_INVOKABLE bool init(const AkCaps &caps);
        Q_INVOKABLE void uninit();

    private:
        SwsContext *m_scaleContext;
        AVCodecContext *m_codecContext;
        qint64 m_maxPacketQueueSize;
        bool m_showLog;
        int m_maxData;
        QThreadPool m_threadPool;
        QMutex m_packetMutex;
        QMutex m_dataMutex;
        QWaitCondition m_packetQueueNotEmpty;
        QWaitCondition m_packetQueueNotFull;
        QWaitCondition m_dataQueueNotEmpty;
        QWaitCondition m_dataQueueNotFull;
        QQueue<AkPacket> m_packets;
        QQueue<FramePtr> m_frames;
        qint64 m_packetQueueSize;
        bool m_runPacketLoop;
        bool m_runDataLoop;
        QFuture<void> m_packetLoopResult;
        QFuture<void> m_dataLoopResult;
        qint64 m_id;
        Clock m_globalClock;

        // Sync properties.
        qreal m_lastPts;

        static void packetLoop(ConvertVideo *stream);
        static void dataLoop(ConvertVideo *stream);
        static void deleteFrame(AVFrame *frame);
        void processData(const FramePtr &frame);
        void convert(const FramePtr &frame);
        void log(qreal diff);

    signals:
        void maxPacketQueueSizeChanged(qint64 maxPacketQueue);
        void showLogChanged(bool showLog);
        void frameReady(const AkPacket &packet);

    public slots:
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void setShowLog(bool showLog);
        void resetMaxPacketQueueSize();
        void resetShowLog();
};

#endif // CONVERTVIDEO_H
