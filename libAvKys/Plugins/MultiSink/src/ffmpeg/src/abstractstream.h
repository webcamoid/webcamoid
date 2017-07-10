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

#ifndef ABSTRACTSTREAM_H
#define ABSTRACTSTREAM_H

#include <QQueue>
#include <QWaitCondition>
#include <QtConcurrentRun>
#include <akpacket.h>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/frame.h>
}

#define CODEC_COMPLIANCE FF_COMPLIANCE_VERY_STRICT
//#define CODEC_COMPLIANCE FF_COMPLIANCE_EXPERIMENTAL
#define THREAD_WAIT_LIMIT 500

class MediaWriterFFmpeg;

class AbstractStream: public QObject
{
    Q_OBJECT

    public:
        explicit AbstractStream(const AVFormatContext *formatContext=NULL,
                                uint index=0, int streamIndex=-1,
                                const QVariantMap &configs={},
                                const QMap<QString, QVariantMap> &codecOptions={},
                                MediaWriterFFmpeg *mediaWriter=NULL,
                                QObject *parent=NULL);
        virtual ~AbstractStream();

        Q_INVOKABLE uint index() const;
        Q_INVOKABLE int streamIndex() const;
        Q_INVOKABLE AVMediaType mediaType() const;
        Q_INVOKABLE AVStream *stream() const;
        Q_INVOKABLE AVFormatContext *formatContext() const;
        Q_INVOKABLE AVCodecContext *codecContext() const;
        Q_INVOKABLE void packetEnqueue(const AkPacket &packet);

    protected:
        int m_maxPacketQueueSize;
        int m_maxFrameQueueSize;
        qint64 m_frameQueueSize;
        int m_frameSize;

        virtual void convertPacket(const AkPacket &packet);
        virtual void encodeData(AVFrame *frame);
        virtual AVFrame *dequeueFrame();
        void rescaleTS(AVPacket *pkt, AVRational src, AVRational dst);

    private:
        uint m_index;
        int m_streamIndex;
        AVMediaType m_mediaType;
        AVFormatContext *m_formatContext;
        AVCodecContext *m_codecContext;
        AVStream *m_stream;
        QThreadPool m_threadPool;
        AVDictionary *m_codecOptions;

        // Packet queue and convert loop.
        QQueue<AkPacket> m_packetQueue;
        QMutex m_convertMutex;
        QWaitCondition m_packetQueueNotFull;
        QWaitCondition m_packetQueueNotEmpty;
        QFuture<void> m_convertLoopResult;
        bool m_runConvertLoop;

        // Frame queue and encoding loop.
        QMutex m_encodeMutex;
        QWaitCondition m_frameQueueNotFull;
        QWaitCondition m_frameQueueNotEmpty;
        QFuture<void> m_encodeLoopResult;
        bool m_runEncodeLoop;

        void convertLoop();
        void encodeLoop();
        void deleteFrame(AVFrame *frame);

    signals:
        void packetReady(const AVPacket *packet);

    public slots:
        virtual bool init();
        virtual void uninit();
};

#endif // ABSTRACTSTREAM_H
