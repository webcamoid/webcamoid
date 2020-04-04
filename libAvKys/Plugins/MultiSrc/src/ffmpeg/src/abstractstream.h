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

#ifndef ABSTRACTSTREAM_H
#define ABSTRACTSTREAM_H

#include <QObject>

extern "C"
{
    #include <libavformat/avformat.h>
}

#ifdef PixelFormat
#undef PixelFormat
#endif

class AbstractStreamPrivate;
class AkFrac;
class AkCaps;
class AkPacket;
class Clock;

class AbstractStream: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool paused
               READ paused
               WRITE setPaused
               RESET resetPaused
               NOTIFY pausedChanged)

    public:
        AbstractStream(const AVFormatContext *formatContext=nullptr,
                       uint index=0, qint64 id=-1,
                       Clock *globalClock=nullptr,
                       bool noModify=false,
                       QObject *parent=nullptr);
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
        Q_INVOKABLE void subtitleEnqueue(AVSubtitle *subtitle);
        Q_INVOKABLE qint64 queueSize();
        Q_INVOKABLE Clock *globalClock();
        Q_INVOKABLE qreal clockDiff() const;
        Q_INVOKABLE qreal &clockDiff();

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
        AbstractStreamPrivate *d;

    signals:
        void pausedChanged(bool paused);
        void oStream(const AkPacket &packet);
        void notify();
        void frameSent();
        void eof();

    public slots:
        void flush();
        void setPaused(bool paused);
        void resetPaused();
        virtual bool init(bool paused=false);
        virtual void uninit();

        friend class AbstractStreamPrivate;
};

#endif // ABSTRACTSTREAM_H
