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

#include "abstractstream.h"

AbstractStream::AbstractStream(QObject *parent): QObject(parent)
{
    this->m_isValid = false;
    this->m_index = -1;
    this->m_mediaType = AVMEDIA_TYPE_UNKNOWN;
    this->m_stream = NULL;
    this->m_codecContext = NULL;
    this->m_codec = NULL;
    this->m_codecOptions = NULL;
    this->m_queueSize = 0;

    this->resetOutputThread();
}

AbstractStream::AbstractStream(const AVFormatContext *formatContext, uint index)
{
    this->m_isValid = false;
    this->m_index = index;
    this->m_stream = formatContext->streams[index];
    this->m_mediaType = this->m_stream->codec->codec_type;
    this->m_codecContext = this->m_stream->codec;
    this->m_codec = avcodec_find_decoder(this->m_codecContext->codec_id);
    this->m_codecOptions = NULL;
    this->m_queueSize = 0;

    this->m_timeBase = QbFrac(this->m_stream->time_base.num,
                              this->m_stream->time_base.den);

    if (!this->m_codec)
        return;

    this->m_stream->discard = AVDISCARD_DEFAULT;
    this->m_codecContext->workaround_bugs = 1;
    this->m_codecContext->idct_algo = FF_IDCT_AUTO;
    this->m_codecContext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;

    if (this->m_codec->capabilities & CODEC_CAP_DR1)
        this->m_codecContext->flags |= CODEC_FLAG_EMU_EDGE;

    this->resetOutputThread();

    this->m_isValid = true;
}

bool AbstractStream::isValid() const
{
    return this->m_isValid;
}

uint AbstractStream::index() const
{
    return this->m_index;
}

QbFrac AbstractStream::timeBase() const
{
    return this->m_timeBase;
}

AVMediaType AbstractStream::mediaType() const
{
    return this->m_mediaType;
}

AVStream *AbstractStream::stream() const
{
    return this->m_stream;
}

AVCodecContext *AbstractStream::codecContext() const
{
    return this->m_codecContext;
}

AVCodec *AbstractStream::codec() const
{
    return this->m_codec;
}

AVDictionary *AbstractStream::codecOptions() const
{
    return this->m_codecOptions;
}

QbCaps AbstractStream::caps() const
{
    return QbCaps();
}

void AbstractStream::enqueue(const AVPacket *packet)
{
    this->m_mutex.lock();

    if (packet) {
        this->m_packets.enqueue(PacketPtr(const_cast<AVPacket *>(packet),
                                          this->deletePacket));
        this->m_queueSize += packet->size;
    }
    else
        this->m_packets.insert(0, PacketPtr());

    this->m_queueNotEmpty.wakeAll();
    this->m_mutex.unlock();
}

qint64 AbstractStream::queueSize()
{
    return this->m_queueSize;
}

QThread *AbstractStream::outputThread() const
{
    return this->m_outputThread;
}

AVMediaType AbstractStream::type(const FormatContextPtr &formatContext,
                                 uint index)
{
    return formatContext->streams[index]->codec->codec_type;
}

void AbstractStream::processPacket(const PacketPtr &packet)
{
    Q_UNUSED(packet)
}

void AbstractStream::deletePacket(AVPacket *packet)
{
    av_free_packet(packet);
    delete packet;
}

void AbstractStream::deleteThread(QThread *thread)
{
    thread->quit();
    thread->wait();
    delete thread;
}

void AbstractStream::setOutputThread(const QThread *outputThread)
{
    this->m_outputThread = const_cast<QThread *>(outputThread);
}

void AbstractStream::resetOutputThread()
{
    this->setOutputThread(NULL);
}

void AbstractStream::setPull(bool pull)
{
    this->m_timer.setSingleShot(pull);
}

void AbstractStream::pull()
{
    QMetaObject::invokeMethod(&this->m_timer, "start");
}

void AbstractStream::init()
{
    if (avcodec_open2(this->m_codecContext, this->m_codec,
                      &this->m_codecOptions) < 0)
        return;

    if (this->m_outputThread)
        this->m_timer.moveToThread(const_cast<QThread *>(this->m_outputThread));
    else {
        this->m_outputThreadPtr = ThreadPtr(new QThread(), this->deleteThread);
        this->m_outputThreadPtr->start();
        this->m_timer.moveToThread(this->m_outputThreadPtr.data());
    }

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(pullFrame()),
                     Qt::DirectConnection);

    if (!this->m_timer.isSingleShot())
        QMetaObject::invokeMethod(&this->m_timer, "start");
}

void AbstractStream::pullFrame()
{
    this->m_mutex.lock();

    if (this->m_packets.isEmpty())
        this->m_queueNotEmpty.wait(&this->m_mutex);

    PacketPtr packet = this->m_packets.dequeue();

    if (packet) {
        this->processPacket(packet);
        this->m_queueSize -= packet->size;
        emit this->notify();
    }
    else {
        this->m_timer.stop();

        if (this->m_codecOptions)
            av_dict_free(&this->m_codecOptions);

        if (this->m_codecContext)
            avcodec_close(this->m_codecContext);

        emit this->exited(this->m_index);
    }

    this->m_mutex.unlock();
}
