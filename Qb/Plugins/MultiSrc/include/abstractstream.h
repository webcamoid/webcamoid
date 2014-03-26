/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef ABSTRACTSTREAM_H
#define ABSTRACTSTREAM_H

#include <QtCore>
#include <qb.h>

extern "C"
{
    #include <libavdevice/avdevice.h>
    #include <libavutil/imgutils.h>
}

class AbstractStream;

typedef QSharedPointer<AVFormatContext> FormatContextPtr;
typedef QSharedPointer<AbstractStream> AbstractStreamPtr;
typedef QSharedPointer<AVPacket> PacketPtr;
typedef QSharedPointer<QThread> ThreadPtr;

class AbstractStream: public QObject
{
    Q_OBJECT

    public:
        explicit AbstractStream(QObject *parent=NULL);
        AbstractStream(const AVFormatContext *formatContext, uint index);

        Q_INVOKABLE bool isValid() const;
        Q_INVOKABLE uint index() const;
        Q_INVOKABLE QbFrac timeBase() const;
        Q_INVOKABLE AVMediaType mediaType() const;
        Q_INVOKABLE AVStream *stream() const;
        Q_INVOKABLE AVCodecContext *codecContext() const;
        Q_INVOKABLE AVCodec *codec() const;
        Q_INVOKABLE AVDictionary *codecOptions() const;
        Q_INVOKABLE virtual QbCaps caps() const;
        Q_INVOKABLE void enqueue(const AVPacket *packet);
        Q_INVOKABLE qint64 queueSize();
        Q_INVOKABLE QThread *outputThread() const;

        static AVMediaType type(const FormatContextPtr &formatContext,
                                uint index);

    protected:
        bool m_isValid;

        virtual void processPacket(const PacketPtr &packet);

    private:
        uint m_index;
        QbFrac m_timeBase;
        AVMediaType m_mediaType;
        AVStream *m_stream;
        AVCodecContext *m_codecContext;
        AVCodec *m_codec;
        AVDictionary *m_codecOptions;
        QString m_threadName;
        QTimer m_timer;
        QMutex m_mutex;
        ThreadPtr m_outputThreadPtr;
        QThread *m_outputThread;
        QWaitCondition m_queueNotEmpty;
        QQueue<PacketPtr> m_packets;
        qint64 m_queueSize;

        static void deletePacket(AVPacket *packet);
        static void deleteThread(QThread *thread);

    signals:
        void oStream(const QbPacket &packet);
        void notify();
        void exited(uint index);

    public slots:
        void setOutputThread(const QThread *outputThread);
        void resetOutputThread();
        void setPull(bool pull);
        void pull();

        void init();

    private slots:
        void pullFrame();
};

#endif // ABSTRACTSTREAM_H
