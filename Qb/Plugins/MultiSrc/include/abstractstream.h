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

#ifndef ABSTRACTSTREAM_H
#define ABSTRACTSTREAM_H

#include <QtConcurrent>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <qb.h>

extern "C"
{
    #include <libavdevice/avdevice.h>
    #include <libavutil/imgutils.h>
}

class AbstractStream: public QObject
{
    Q_OBJECT

    public:
        explicit AbstractStream(const AVFormatContext *formatContext=NULL,
                                uint index=-1, qint64 id=-1, bool noModify=false,
                                QObject *parent=NULL);

        Q_INVOKABLE bool isValid() const;
        Q_INVOKABLE uint index() const;
        Q_INVOKABLE qint64 id() const;
        Q_INVOKABLE QbFrac timeBase() const;
        Q_INVOKABLE AVMediaType mediaType() const;
        Q_INVOKABLE AVStream *stream() const;
        Q_INVOKABLE AVCodecContext *codecContext() const;
        Q_INVOKABLE AVCodec *codec() const;
        Q_INVOKABLE AVDictionary *codecOptions() const;
        Q_INVOKABLE virtual QbCaps caps() const;
        Q_INVOKABLE void enqueue(AVPacket *packet);
        Q_INVOKABLE qint64 queueSize();

        static AVMediaType type(const AVFormatContext *formatContext,
                                uint index);

    protected:
        bool m_isValid;

        virtual void processPacket(AVPacket *packet);

    private:
        uint m_index;
        qint64 m_id;
        QbFrac m_timeBase;
        AVMediaType m_mediaType;
        AVStream *m_stream;
        AVCodecContext *m_codecContext;
        AVCodec *m_codec;
        AVDictionary *m_codecOptions;
        bool m_run;
        QThreadPool m_threadPool;
        QMutex m_mutex;
        QWaitCondition m_queueNotEmpty;
        QQueue<AVPacket *> m_packets;
        qint64 m_queueSize;

        static void decodeFrame(AbstractStream *stream);

    signals:
        void oStream(const QbPacket &packet);
        void notify();

    public slots:
        bool open();
        void close();
        virtual void init();
        virtual void uninit();
};

#endif // ABSTRACTSTREAM_H
