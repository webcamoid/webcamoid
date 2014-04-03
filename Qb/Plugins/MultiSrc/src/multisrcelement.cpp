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

#include "videostream.h"
#include "audiostream.h"
#include "subtitlestream.h"
#include "multisrcelement.h"

MultiSrcElement::MultiSrcElement(): QbElement()
{
    av_register_all();
    avdevice_register_all();
    avformat_network_init();

    this->m_loop = false;
    this->m_audioAlign = false;
    this->resetMaxPacketQueueSize();
    this->m_decodingThread = NULL;

    this->m_avMediaTypeToMimeType[AVMEDIA_TYPE_UNKNOWN] = "unknown/x-raw";
    this->m_avMediaTypeToMimeType[AVMEDIA_TYPE_VIDEO] = "video/x-raw";
    this->m_avMediaTypeToMimeType[AVMEDIA_TYPE_AUDIO] = "audio/x-raw";
    this->m_avMediaTypeToMimeType[AVMEDIA_TYPE_DATA] = "data/x-raw";
    this->m_avMediaTypeToMimeType[AVMEDIA_TYPE_SUBTITLE] = "subtitle/x-raw";
    this->m_avMediaTypeToMimeType[AVMEDIA_TYPE_ATTACHMENT] = "attachment/x-raw";
    this->m_avMediaTypeToMimeType[AVMEDIA_TYPE_NB] = "nb/x-raw";
}

MultiSrcElement::~MultiSrcElement()
{
    this->uninit();
}

QString MultiSrcElement::location() const
{
    return this->m_location;
}

bool MultiSrcElement::loop() const
{
    return this->m_loop;
}

QVariantMap MultiSrcElement::streamCaps()
{
    QVariantMap caps;
    bool clearContext = false;

    if (!this->m_inputContext) {
        if (!this->initContext())
            return caps;

        clearContext = true;
    }

    for (uint i = 0; i < this->m_inputContext->nb_streams; i++) {
        AbstractStreamPtr stream = this->createStream(i, true);

        if (stream)
            caps[QString("%1").arg(i)] = stream->caps().toString();
    }

    if (clearContext)
        this->m_inputContext.clear();

    return caps;
}

QList<int> MultiSrcElement::filterStreams() const
{
    return this->m_filterStreams;
}

bool MultiSrcElement::audioAlign() const
{
    return this->m_audioAlign;
}

qint64 MultiSrcElement::maxPacketQueueSize() const
{
    return this->m_maxPacketQueueSize;
}

int MultiSrcElement::defaultStream(const QString &mimeType)
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

        if (this->m_avMediaTypeToMimeType[type] == mimeType) {
            stream = i;

            break;
        }
    }

    if (clearContext)
        this->m_inputContext.clear();

    return stream;
}

void MultiSrcElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->init();
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        this->uninit();
}

qint64 MultiSrcElement::packetQueueSize()
{
    qint64 size = 0;

    foreach (AbstractStreamPtr stream, this->m_streams.values())
        size += stream->queueSize();

    return size;
}

void MultiSrcElement::deleteFormatContext(AVFormatContext *context)
{
    avformat_close_input(&context);
}

AbstractStreamPtr MultiSrcElement::createStream(int index, bool noModify)
{
    AVMediaType type = AbstractStream::type(this->m_inputContext.data(), index);
    AbstractStreamPtr stream;
    qint64 id = Qb::id();

    if (type == AVMEDIA_TYPE_VIDEO)
        stream = AbstractStreamPtr(new VideoStream(this->m_inputContext.data(),
                                                   index, id, noModify));
    else if (type == AVMEDIA_TYPE_AUDIO)
    {
        stream = AbstractStreamPtr(new AudioStream(this->m_inputContext.data(),
                                                   index, id, noModify));
        stream->setProperty("align", this->m_audioAlign);
    }
    else if (type == AVMEDIA_TYPE_SUBTITLE)
        stream = AbstractStreamPtr(new SubtitleStream(this->m_inputContext.data(),
                                                      index, id, noModify));
    else
        stream = AbstractStreamPtr(new AbstractStream(this->m_inputContext.data(),
                                                      index, id, noModify));

    return stream;
}

void MultiSrcElement::setLocation(const QString &location)
{
    if (location == this->location())
        return;

    ElementState preState = this->state();

    this->setState(ElementStateNull);
    this->m_location = location;

    if (!this->location().isEmpty())
        this->setState(preState);
}

void MultiSrcElement::setLoop(bool loop)
{
    this->m_loop = loop;
}

void MultiSrcElement::setFilterStreams(const QList<int> &filterStreams)
{
    this->m_filterStreams = filterStreams;
}

void MultiSrcElement::setAudioAlign(bool audioAlign)
{
    this->m_audioAlign = audioAlign;
}

void MultiSrcElement::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    this->m_maxPacketQueueSize = maxPacketQueueSize;
}

void MultiSrcElement::resetLocation()
{
    this->setLocation("");
}

void MultiSrcElement::resetLoop()
{
    this->setLoop(false);
}

void MultiSrcElement::resetFilterStreams()
{
    this->m_filterStreams.clear();
}

void MultiSrcElement::resetAudioAlign()
{
    this->setAudioAlign(false);
}

void MultiSrcElement::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void MultiSrcElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);

    if (this->state() == ElementStatePlaying)
    {
//        QMetaObject::invokeMethod(&this->m_decodingTimer, "start");
    }
    else
    {
//        QMetaObject::invokeMethod(&this->m_decodingTimer, "stop");
    }
}

void MultiSrcElement::doLoop()
{
    this->uninit();
    this->init();
}

void MultiSrcElement::pullData()
{
    while (this->m_run) {
        this->m_dataMutex.lock();

        if (this->packetQueueSize() >= this->m_maxPacketQueueSize)
            this->m_packetQueueNotFull.wait(&this->m_dataMutex);

        AVPacket *packet = new AVPacket();
        av_init_packet(packet);
        bool notuse = true;

        int r = av_read_frame(this->m_inputContext.data(), packet);

        if (r >= 0) {
            if (this->m_streams.contains(packet->stream_index)
                && (this->m_filterStreams.isEmpty()
                    || this->m_filterStreams.contains(packet->stream_index))) {
                this->m_streams[packet->stream_index]->enqueue(packet);
                notuse = false;
            }
        }

        if (notuse) {
            av_free_packet(packet);
            delete packet;
        }

        if (r < 0)
        {
            if (this->m_loop)
                QMetaObject::invokeMethod(this, "doLoop");

            this->m_dataMutex.unlock();

            return;
        }

        QMap<int, qint64> queueSize;

        foreach (int stream, this->m_streams.keys())
            queueSize[stream] = this->m_streams[stream]->queueSize();

        emit this->queueSizeUpdated(queueSize);

        this->m_dataMutex.unlock();
    }
}

void MultiSrcElement::packetConsumed()
{
    QtConcurrent::run(this, &MultiSrcElement::unlockQueue);
}

void MultiSrcElement::unlockQueue()
{
    this->m_dataMutex.lock();

    if (this->packetQueueSize() < this->m_maxPacketQueueSize)
        this->m_packetQueueNotFull.wakeAll();

    this->m_dataMutex.unlock();
}

bool MultiSrcElement::init()
{
    if (!initContext())
        return false;

    if (avformat_find_stream_info(this->m_inputContext.data(), NULL) < 0) {
        this->m_inputContext.clear();

        return false;
    }

    QString uri = this->location();
    av_dump_format(this->m_inputContext.data(), 0, uri.toStdString().c_str(),
                   false);

    QList<int> filterStreams;

    if (this->m_filterStreams.isEmpty())
        for (uint i = 0; i < this->m_inputContext->nb_streams; i++)
            filterStreams << i;
    else
        filterStreams = this->m_filterStreams;

    foreach (int i, filterStreams) {
        AbstractStreamPtr stream = this->createStream(i);

        if (stream) {
            this->m_streams[i] = stream;

            QObject::connect(stream.data(), SIGNAL(oStream(const QbPacket &)), this,
                             SIGNAL(oStream(const QbPacket &)),
                             Qt::DirectConnection);

            QObject::connect(stream.data(), SIGNAL(notify()), this,
                             SLOT(packetConsumed()));

            stream->init();
        }
    }

    this->m_decodingThread = new Thread();

    QObject::connect(this->m_decodingThread,
                     SIGNAL(runTh()),
                     this,
                     SLOT(pullData()),
                     Qt::DirectConnection);

    this->m_run = true;
    this->m_decodingThread->start();

    return true;
}

bool MultiSrcElement::initContext()
{
    QString uri = this->location();

    if (uri.isEmpty()) {
        qDebug() << "URI empty";

        return false;
    }

    AVInputFormat *inputFormat = NULL;
    AVDictionary *inputOptions = NULL;

    if (QRegExp("/dev/video\\d*").exactMatch(uri))
    {
        inputFormat = av_find_input_format("v4l2");

        //av_dict_set(&inputOptions, "timestamps", "default", 0);
        //av_dict_set(&inputOptions, "timestamps", "abs", 0);
        av_dict_set(&inputOptions, "timestamps", "mono2abs", 0);
    }
    else if (QRegExp(":\\d+\\.\\d+(?:\\+\\d+,\\d+)?").exactMatch(uri))
    {
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

    foreach (QString scheme, mmsSchemes)
    {
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

void MultiSrcElement::uninit()
{
    this->m_dataMutex.lock();
    this->m_run = false;
    this->m_packetQueueNotFull.wakeAll();
    this->m_dataMutex.unlock();

    if (this->m_decodingThread) {
        this->m_decodingThread->quit();
        this->m_decodingThread->wait();
        delete this->m_decodingThread;
        this->m_decodingThread = NULL;
    }

    foreach (AbstractStreamPtr stream, this->m_streams)
        stream->uninit();

    this->m_streams.clear();
    this->m_inputContext.clear();
}
