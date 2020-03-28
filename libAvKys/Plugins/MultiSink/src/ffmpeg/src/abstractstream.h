/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#include <QVariantMap>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}

#define CODEC_COMPLIANCE FF_COMPLIANCE_VERY_STRICT
//#define CODEC_COMPLIANCE FF_COMPLIANCE_EXPERIMENTAL
#define THREAD_WAIT_LIMIT 500

class AbstractStreamPrivate;
class MediaWriterFFmpeg;
class AbstractStream;
class AkPacket;

using AbstractStreamPtr = QSharedPointer<AbstractStream>;

class AbstractStream: public QObject
{
    Q_OBJECT

    public:
        AbstractStream(const AVFormatContext *formatContext=nullptr,
                       uint index=0, int streamIndex=-1,
                       const QVariantMap &configs={},
                       const QMap<QString, QVariantMap> &codecOptions={},
                       MediaWriterFFmpeg *mediaWriter=nullptr,
                       QObject *parent=nullptr);
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

        virtual void convertPacket(const AkPacket &packet);
        virtual int encodeData(AVFrame *frame);
        virtual AVFrame *dequeueFrame();
        void rescaleTS(AVPacket *pkt, AVRational src, AVRational dst);
        void deleteFrame(AVFrame **frame);

    private:
        AbstractStreamPrivate *d;

    signals:
        void packetReady(AVPacket *packet);

    public slots:
        virtual bool init();
        virtual void uninit();

        friend class AbstractStreamPrivate;
};

#endif // ABSTRACTSTREAM_H
