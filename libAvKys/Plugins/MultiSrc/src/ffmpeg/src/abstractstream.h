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

#ifndef ABSTRACTSTREAM_H
#define ABSTRACTSTREAM_H

#include <QtConcurrent>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <ak.h>

extern "C"
{
    #include <libavdevice/avdevice.h>
    #include <libavutil/imgutils.h>
}

#ifdef PixelFormat
#undef PixelFormat
#endif

#include "clock.h"

typedef QSharedPointer<AVPacket> PacketPtr;
typedef QSharedPointer<AVFrame> FramePtr;
typedef QSharedPointer<AVSubtitle> SubtitlePtr;

class AbstractStream: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool paused
               READ paused
               WRITE setPaused
               RESET resetPaused
               NOTIFY pausedChanged)

    public:
        explicit AbstractStream(const AVFormatContext *formatContext=NULL,
                                uint index=-1, qint64 id=-1,
                                Clock *globalClock=NULL,
                                bool noModify=false,
                                QObject *parent=NULL);
        virtual ~AbstractStream();

        Q_INVOKABLE bool paused() const;
        Q_INVOKABLE bool isValid() const;
        Q_INVOKABLE uint index() const;
        Q_INVOKABLE qint64 id() const;
        Q_INVOKABLE AkFrac timeBase() const;
        Q_INVOKABLE AVMediaType mediaType() const;
        Q_INVOKABLE AVStream *stream() const;
        Q_INVOKABLE AVCodecContext *codecContext() const;
        Q_INVOKABLE AVCodec *codec() const;
        Q_INVOKABLE AVDictionary *codecOptions() const;
        Q_INVOKABLE virtual AkCaps caps() const;
        Q_INVOKABLE void packetEnqueue(AVPacket *packet);
        Q_INVOKABLE void dataEnqueue(AVFrame *frame);
        Q_INVOKABLE void dataEnqueue(AVSubtitle *subtitle);
        Q_INVOKABLE qint64 queueSize();
        Q_INVOKABLE Clock *globalClock();
        Q_INVOKABLE qreal clockDiff() const;

        static AVMediaType type(const AVFormatContext *formatContext,
                                uint index);

    protected:
        bool m_paused;
        bool m_isValid;
        qreal m_clockDiff;
        int m_maxData;

        virtual void processPacket(AVPacket *packet);
        virtual void processData(AVFrame *frame);
        virtual void processData(AVSubtitle *subtitle);

    private:
        uint m_index;
        qint64 m_id;
        AkFrac m_timeBase;
        AVMediaType m_mediaType;
        AVStream *m_stream;
        AVCodecContext *m_codecContext;
        AVCodec *m_codec;
        AVDictionary *m_codecOptions;
        QThreadPool m_threadPool;
        QMutex m_packetMutex;
        QMutex m_dataMutex;
        QWaitCondition m_packetQueueNotEmpty;
        QWaitCondition m_dataQueueNotEmpty;
        QWaitCondition m_dataQueueNotFull;
        QQueue<PacketPtr> m_packets;
        QQueue<FramePtr> m_frames;
        QQueue<SubtitlePtr> m_subtitles;
        qint64 m_packetQueueSize;
        Clock *m_globalClock;
        bool m_runPacketLoop;
        bool m_runDataLoop;
        QFuture<void> m_packetLoopResult;
        QFuture<void> m_dataLoopResult;

        void packetLoop();
        void dataLoop();
        static void deletePacket(AVPacket *packet);
        static void deleteFrame(AVFrame *frame);
        static void deleteSubtitle(AVSubtitle *subtitle);

    signals:
        void pausedChanged(bool paused);
        void oStream(const AkPacket &packet);
        void notify();
        void frameSent();
        void eof();

    public slots:
        void setPaused(bool paused);
        void resetPaused();
        virtual bool init();
        virtual void uninit();
};

#endif // ABSTRACTSTREAM_H
