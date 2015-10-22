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

#include <QApplication>
#include <QDesktopWidget>

#include "mediasource.h"
#include "videostream.h"
#include "audiostream.h"
#include "subtitlestream.h"

typedef QMap<AVMediaType, QString> AvMediaTypeStrMap;

inline AvMediaTypeStrMap initAvMediaTypeStrMap()
{
    AvMediaTypeStrMap mediaTypeToStr;
    mediaTypeToStr[AVMEDIA_TYPE_UNKNOWN] = "unknown/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_VIDEO] = "video/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_AUDIO] = "audio/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_DATA] = "data/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_SUBTITLE] = "text/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_ATTACHMENT] = "attachment/x-raw";
    mediaTypeToStr[AVMEDIA_TYPE_NB] = "nb/x-raw";

    return mediaTypeToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(AvMediaTypeStrMap, mediaTypeToStr, (initAvMediaTypeStrMap()))

MediaSource::MediaSource(QObject *parent): QObject(parent)
{
    av_register_all();
    avdevice_register_all();
    avformat_network_init();

    this->m_maxPacketQueueSize = 15 * 1024 * 1024;
    this->m_showLog = false;
    this->m_loop = false;
}

MediaSource::~MediaSource()
{
    this->uninit();
}

QStringList MediaSource::medias() const
{
    QStringList medias;

    if (!this->m_media.isEmpty())
        medias << this->m_media;

    return medias;
}

QString MediaSource::media() const
{
    return this->m_media;
}

QList<int> MediaSource::streams() const
{
    return this->m_streams;
}

QList<int> MediaSource::listTracks(const QString &mimeType)
{
    QList<int> tracks;
    bool clearContext = false;

    if (!this->m_inputContext) {
        if (!this->initContext())
            return tracks;

        clearContext = true;
    }

    for (uint stream = 0; stream < this->m_inputContext->nb_streams; stream++) {
        AVMediaType type = this->m_inputContext->streams[stream]->codec->codec_type;

        if (mimeType.isEmpty()
            || mediaTypeToStr->value(type) == mimeType)
            tracks << stream;
    }

    if (clearContext)
        this->m_inputContext.clear();

    return tracks;
}

QString MediaSource::streamLanguage(int stream)
{
    bool clearContext = false;

    if (!this->m_inputContext) {
        if (!this->initContext())
            return QString();

        clearContext = true;
    }

    AVDictionary *metadata = this->m_inputContext->streams[stream]->metadata;
    AVDictionaryEntry *dicEntry = NULL;
    QString language;

    while ((dicEntry = av_dict_get(metadata, "", dicEntry, AV_DICT_IGNORE_SUFFIX))) {
        QString key(dicEntry->key);
        QString value(dicEntry->value);

        if (key == "language") {
            language = value;

            break;
        }
    }

    if (clearContext)
        this->m_inputContext.clear();

    return language;
}

bool MediaSource::loop() const
{
    return this->m_loop;
}

int MediaSource::defaultStream(const QString &mimeType)
{
    int stream = -1;
    bool clearContext = false;

    if (!this->m_inputContext) {
        if (!this->initContext())
            return stream;

        clearContext = true;
    }

    for (uint i = 0; i < this->m_inputContext->nb_streams; i++) {
        AVMediaType type = this->m_inputContext->streams[i]->codec->codec_type;

        if (mediaTypeToStr->value(type) == mimeType) {
            stream = i;

            break;
        }
    }

    if (clearContext)
        this->m_inputContext.clear();

    return stream;
}

QString MediaSource::description(const QString &media) const
{
    if (this->m_media != media)
        return QString();

    return QFileInfo(media).baseName();
}

QbCaps MediaSource::caps(int stream)
{
    bool clearContext = false;

    if (!this->m_inputContext) {
        if (!this->initContext())
            return QbCaps();

        if (avformat_find_stream_info(this->m_inputContext.data(), NULL) < 0) {
            this->m_inputContext.clear();

            return QbCaps();
        }

        clearContext = true;
    }

    QbCaps caps;

    if (stream >= 0
        && stream < int(this->m_inputContext->nb_streams)) {
        AbstractStreamPtr streamPtr = this->createStream(stream, true);
        caps = streamPtr->caps();
    }

    if (clearContext)
        this->m_inputContext.clear();

    return caps;
}

qint64 MediaSource::maxPacketQueueSize() const
{
    return this->m_maxPacketQueueSize;
}

bool MediaSource::showLog() const
{
    return this->m_showLog;
}

qint64 MediaSource::packetQueueSize()
{
    qint64 size = 0;

    foreach (AbstractStreamPtr stream, this->m_streamsMap.values())
        size += stream->queueSize();

    return size;
}

void MediaSource::deleteFormatContext(AVFormatContext *context)
{
    avformat_close_input(&context);
}

AbstractStreamPtr MediaSource::createStream(int index, bool noModify)
{
    AVMediaType type = AbstractStream::type(this->m_inputContext.data(), index);
    AbstractStreamPtr stream;
    qint64 id = Qb::id();

    if (type == AVMEDIA_TYPE_VIDEO)
        stream = AbstractStreamPtr(new VideoStream(this->m_inputContext.data(),
                                                   index, id,
                                                   &this->m_globalClock,
                                                   noModify));
    else if (type == AVMEDIA_TYPE_AUDIO)
        stream = AbstractStreamPtr(new AudioStream(this->m_inputContext.data(),
                                                   index, id,
                                                   &this->m_globalClock,
                                                   noModify));
    else if (type == AVMEDIA_TYPE_SUBTITLE)
        stream = AbstractStreamPtr(new SubtitleStream(this->m_inputContext.data(),
                                                      index, id,
                                                      &this->m_globalClock,
                                                      noModify));
    else
        stream = AbstractStreamPtr(new AbstractStream(this->m_inputContext.data(),
                                                      index, id,
                                                      &this->m_globalClock,
                                                      noModify));

    return stream;
}

void MediaSource::readPackets(MediaSource *element)
{
    while (element->m_run) {
        element->m_dataMutex.lock();

        if (element->packetQueueSize() >= element->m_maxPacketQueueSize)
            element->m_packetQueueNotFull.wait(&element->m_dataMutex);

        AVPacket *packet = new AVPacket();
        av_init_packet(packet);
        bool notuse = true;

        int r = av_read_frame(element->m_inputContext.data(), packet);

        if (r >= 0) {
            if (element->m_streamsMap.contains(packet->stream_index)
                && (element->m_streams.isEmpty()
                    || element->m_streams.contains(packet->stream_index))) {
                element->m_streamsMap[packet->stream_index]->enqueue(packet);
                notuse = false;
            }
        }

        if (notuse) {
            av_free_packet(packet);
            delete packet;
        }

        if (r < 0) {
            if (element->loop()) {
                if (element->packetQueueSize() > 0)
                    element->m_packetQueueEmpty.wait(&element->m_dataMutex);

                QMetaObject::invokeMethod(element, "doLoop");
            }

            element->m_dataMutex.unlock();

            return;
        }

        element->m_dataMutex.unlock();
    }
}

void MediaSource::unlockQueue(MediaSource *element)
{
    element->m_dataMutex.tryLock();

    if (element->packetQueueSize() < element->m_maxPacketQueueSize)
        element->m_packetQueueNotFull.wakeAll();

    if (element->packetQueueSize() < 1)
        element->m_packetQueueEmpty.wakeAll();

    element->m_dataMutex.unlock();
}

void MediaSource::setMedia(const QString &media)
{
    if (media == this->m_media)
        return;

    bool isRunning = this->m_run;
    this->uninit();
    this->m_media = media;

    if (isRunning && !this->m_media.isEmpty())
        this->init();

    emit this->mediaChanged(media);
    emit this->mediasChanged(this->medias());
}

void MediaSource::setStreams(const QList<int> &streams)
{
    if (this->m_streams == streams)
        return;

    this->m_streams = streams;
    emit this->streamsChanged(streams);
}

void MediaSource::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void MediaSource::setShowLog(bool showLog)
{
    if (this->m_showLog == showLog)
        return;

    this->m_showLog = showLog;
    emit this->showLogChanged(showLog);
}

void MediaSource::setLoop(bool loop)
{
    if (this->m_loop == loop)
        return;

    this->m_loop = loop;
    emit this->loopChanged(loop);
}

void MediaSource::resetMedia()
{
    this->setMedia("");
}

void MediaSource::resetStreams()
{
    if  (this->m_streams.isEmpty())
        return;

    this->m_streams.clear();
    emit this->streamsChanged(this->m_streams);
}

void MediaSource::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MediaSource::resetShowLog()
{
    this->setShowLog(false);
}

void MediaSource::resetLoop()
{
    this->setLoop(false);
}

bool MediaSource::init()
{
    if (!this->initContext())
        return false;

    if (avformat_find_stream_info(this->m_inputContext.data(), NULL) < 0) {
        this->m_inputContext.clear();

        return false;
    }

    QString uri = this->m_media;
    av_dump_format(this->m_inputContext.data(), 0, uri.toStdString().c_str(),
                   false);

    QList<int> filterStreams;

    if (this->m_streams.isEmpty())
        filterStreams << this->defaultStream("audio/x-raw")
                      << this->defaultStream("video/x-raw");
    else
        filterStreams = this->m_streams;

    foreach (int i, filterStreams) {
        AbstractStreamPtr stream = this->createStream(i);

        if (stream) {
            this->m_streamsMap[i] = stream;

            QObject::connect(stream.data(),
                             &AbstractStream::oStream,
                             this,
                             &MediaSource::oStream,
                             Qt::DirectConnection);

            QObject::connect(stream.data(),
                             &AbstractStream::notify,
                             this,
                             &MediaSource::packetConsumed);

            QObject::connect(stream.data(),
                             &AbstractStream::frameSent,
                             this,
                             &MediaSource::log);

            stream->init();
        }
    }

    this->m_globalClock.setClock(0.);
    this->m_run = true;
    QtConcurrent::run(&this->m_threadPool, this->readPackets, this);

    return true;
}

void MediaSource::uninit()
{
    this->m_run = false;
    this->m_dataMutex.lock();
    this->m_packetQueueNotFull.wakeAll();
    this->m_packetQueueEmpty.wakeAll();
    this->m_dataMutex.unlock();

    this->m_threadPool.waitForDone();

    foreach (AbstractStreamPtr stream, this->m_streamsMap)
        stream->uninit();

    this->m_streamsMap.clear();
    this->m_inputContext.clear();
}

void MediaSource::doLoop()
{
    if (!this->m_run)
        return;

    this->uninit();
    this->init();
}

void MediaSource::packetConsumed()
{
    QtConcurrent::run(&this->m_threadPool, this->unlockQueue, this);
}

bool MediaSource::initContext()
{
    QString uri = this->m_media;

    if (uri.isEmpty()) {
        qDebug() << "URI empty";

        return false;
    }

    AVInputFormat *inputFormat = NULL;
    AVDictionary *inputOptions = NULL;

    if (QRegExp("/dev/video\\d*").exactMatch(uri))
        inputFormat = av_find_input_format("v4l2");
    else if (QRegExp(":\\d+\\.\\d+(?:\\+\\d+,\\d+)?").exactMatch(uri)) {
        inputFormat = av_find_input_format("x11grab");

        int width = this->roundDown(QApplication::desktop()->width(), 4);
        int height = this->roundDown(QApplication::desktop()->height(), 4);

        av_dict_set(&inputOptions,
                    "video_size",
                    QString("%1x%2").arg(width)
                                    .arg(height)
                                    .toStdString().c_str(),
                    0);

        // draw_mouse (int)
    }
    else if (uri == "pulse" ||
             QRegExp("hw:\\d+").exactMatch(uri))
        inputFormat = av_find_input_format("alsa");
    else if (uri == "/dev/dsp")
        inputFormat = av_find_input_format("oss");

    QStringList mmsSchemes;
    mmsSchemes << "mms://" << "mmsh://" << "mmst://";

    AVFormatContext *inputContext = NULL;

    foreach (QString scheme, mmsSchemes) {
        QString uriCopy = uri;

        foreach (QString schemer, mmsSchemes)
            uriCopy.replace(QRegExp(QString("^%1").arg(schemer)),
                            scheme);

        inputContext = NULL;

        if (avformat_open_input(&inputContext,
                                uriCopy.toStdString().c_str(),
                                inputFormat,
                                &inputOptions) >= 0)
            break;
    }

    if (inputOptions)
        av_dict_free(&inputOptions);

    if (!inputContext) {
        emit this->error(QString("Cann't open \"%1\" stream.").arg(uri));

        return false;
    }

    this->m_inputContext = FormatContextPtr(inputContext, this->deleteFormatContext);

    return true;
}

void MediaSource::log()
{
    if (!this->m_showLog)
        return;

    AbstractStreamPtr audioStream;
    AbstractStreamPtr videoStream;

    foreach (int streamId, this->m_streamsMap.keys()) {
        AVMediaType mediaType = this->m_streamsMap[streamId]->mediaType();

        if (mediaType == AVMEDIA_TYPE_AUDIO && !audioStream)
            audioStream = this->m_streamsMap[streamId];

        if (mediaType == AVMEDIA_TYPE_VIDEO && !videoStream)
            videoStream = this->m_streamsMap[streamId];

        if (audioStream && videoStream)
            break;
    }

    QString diffType;
    qreal diff;

    if (audioStream && videoStream) {
        diffType = "A-V";
        diff = audioStream->clockDiff() - videoStream->clockDiff();
    } else if (audioStream) {
        diffType = "M-A";
        diff = -audioStream->clockDiff();
    } else if (videoStream) {
        diffType = "M-V";
        diff = -videoStream->clockDiff();
    } else
        return;

    QString logFmt("%1 %2: %3 aq=%4KB vq=%5KB");

    QString log = logFmt.arg(this->m_globalClock.clock(), 7, 'f', 2)
                        .arg(diffType)
                        .arg(diff, 7, 'f', 3)
                        .arg(audioStream->queueSize() / 1024, 5)
                        .arg(videoStream->queueSize() / 1024, 5);

    qDebug() << log.toStdString().c_str();
}
