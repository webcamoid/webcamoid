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

    this->resetLoop();
    this->resetFilterStreams();

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(readPackets()));
}

MultiSrcElement::~MultiSrcElement()
{
    this->setState(ElementStateNull);
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
    ElementState preState = this->state();

    if (preState == ElementStateNull)
        this->setState(ElementStateReady);

    foreach (int i, this->m_streams.keys())
        caps[QString("%1").arg(i)] = this->m_streams[i]->caps().toString();

    if (preState == ElementStateNull)
        this->setState(ElementStateNull);

    return caps;
}

QStringList MultiSrcElement::filterStreams() const
{
    return this->m_filterStreams;
}

int MultiSrcElement::defaultStream(QString mimeType)
{
    int stream = -1;
    ElementState preState = this->state();

    if (preState == ElementStateNull)
        this->setState(ElementStateReady);

    foreach (int i, this->m_streams.keys())
        if (this->m_streams[i]->caps().mimeType() == mimeType)
            if (stream < 0 || i < stream)
                stream = i;

    if (preState == ElementStateNull)
        this->setState(ElementStateNull);

    return stream;
}

bool MultiSrcElement::init()
{
    if (this->location().isEmpty())
        return false;

    AVInputFormat *inputFormat = NULL;
    AVDictionary *inputOptions = NULL;

    if (QRegExp("/dev/video\\d*").exactMatch(this->location()))
    {
        inputFormat = av_find_input_format("v4l2");

        //av_dict_set(&inputOptions, "timestamps", "default", 0);
        av_dict_set(&inputOptions, "timestamps", "abs", 0);
        //av_dict_set(&inputOptions, "timestamps", "mono2abs", 0);
    }
    else if (QRegExp(":\\d+\\.\\d+(?:\\+\\d+,\\d+)?").exactMatch(this->location()))
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
    else if (this->location() == "pulse" ||
             QRegExp("hw:\\d+").exactMatch(this->location()))
        inputFormat = av_find_input_format("alsa");
    else if (this->location() == "/dev/dsp")
        inputFormat = av_find_input_format("oss");

    QStringList mmsSchemes;

    mmsSchemes << "mms://" << "mmsh://" << "mmst://";

    QString uri;
    AVFormatContext *inputContext;

    foreach (QString scheme, mmsSchemes)
    {
        uri = this->location();

        foreach (QString schemer, mmsSchemes)
            uri.replace(QRegExp(QString("^%1").arg(schemer)),
                        scheme);

        inputContext = NULL;

        if (avformat_open_input(&inputContext,
                                uri.toStdString().c_str(),
                                inputFormat,
                                &inputOptions) >= 0)
            break;
    }

    if (inputOptions)
        av_dict_free(&inputOptions);

    if (!inputContext)
    {
        emit this->error(QString("Cann't open \"%1\" stream.").arg(this->location()));

        return false;
    }

    this->m_inputContext = FormatContextPtr(inputContext, this->deleteFormatContext);

    if (avformat_find_stream_info(this->m_inputContext.data(), NULL) < 0)
    {
        this->m_inputContext.clear();
        emit this->error(QString("Cann't retrieve information from \"%1\" stream.").arg(this->location()));

        return false;
    }

    av_dump_format(this->m_inputContext.data(), 0, uri.toStdString().c_str(), false);

    this->m_streams.clear();

    for (uint i = 0; i < this->m_inputContext->nb_streams; i++)
    {
        AVMediaType type = AbstractStream::type(this->m_inputContext.data(), i);
        QSharedPointer<AbstractStream> stream;

        if (type == AVMEDIA_TYPE_VIDEO)
            stream = QSharedPointer<AbstractStream>(new VideoStream(this->m_inputContext.data(), i));
        else if (type == AVMEDIA_TYPE_AUDIO)
            stream = QSharedPointer<AbstractStream>(new AudioStream(this->m_inputContext.data(), i));
        else if (type == AVMEDIA_TYPE_SUBTITLE)
            stream = QSharedPointer<AbstractStream>(new SubtitleStream(this->m_inputContext.data(), i));
        else
            stream = QSharedPointer<AbstractStream>(new AbstractStream(this->m_inputContext.data(), i));

        if (stream->isValid())
            this->m_streams[i] = stream;
    }

    return true;
}

void MultiSrcElement::uninit()
{
    this->m_streams.clear();
    this->m_inputContext.clear();
}

void MultiSrcElement::deleteFormatContext(AVFormatContext *context)
{
    avformat_close_input(&context);
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

void MultiSrcElement::setFilterStreams(QStringList filterStreams)
{
    this->m_filterStreams = filterStreams;
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
    this->setFilterStreams(QStringList());
}

void MultiSrcElement::setState(ElementState state)
{
    QbElement::setState(state);

    switch (this->state())
    {
        case ElementStatePaused:
            this->m_timer.stop();
        break;

        case ElementStatePlaying:
            this->m_timer.start();
        break;

        default:
        break;
    }
}

void MultiSrcElement::readPackets()
{
    AVPacket packet;
    av_init_packet(&packet);

    if (av_read_frame(this->m_inputContext.data(), &packet) < 0)
    {
        this->setState(ElementStateNull);

        if (this->loop())
            this->setState(ElementStatePlaying);

        return;
    }

    if (this->m_streams.contains(packet.stream_index) &&
        (this->m_filterStreams.isEmpty() ||
         this->m_filterStreams.contains(QString("%1").arg(packet.stream_index))))
    {
        AbstractStreamPtr stream = this->m_streams[packet.stream_index];

        foreach (QbPacket oPacket, stream->readPackets(&packet))
            if (oPacket.caps().isValid())
                emit this->oStream(oPacket);
    }

    av_free_packet(&packet);
}
