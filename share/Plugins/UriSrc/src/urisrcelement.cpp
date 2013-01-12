/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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
#include "urisrcelement.h"

UriSrcElement::UriSrcElement(): QbElement()
{
    av_register_all();
    avdevice_register_all();
    avformat_network_init();

    this->m_uri = "";
    this->resetLoop();
    this->m_state = ElementStateNull;

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(readPackets()));
}

UriSrcElement::~UriSrcElement()
{
    this->setState(ElementStateNull);
}

QString UriSrcElement::uri()
{
    return this->m_uri;
}

bool UriSrcElement::loop()
{
    return this->m_loop;
}

QSize UriSrcElement::size()
{
    if (this->state() == ElementStateNull)
    {
    }
    else
    {
    }
}

bool UriSrcElement::initCapture()
{
    AVInputFormat *inputFormat = NULL;
    AVDictionary *inputOptions = NULL;

    if (QRegExp("/dev/video\\d*").exactMatch(this->uri()))
    {
        inputFormat = av_find_input_format("v4l2");
        av_dict_set(&inputOptions,
                    "video_size",
                    QString("%1x%2").arg(640).arg(480).toUtf8().constData(),
                    0);
    }

    QStringList mmsSchemes;

    mmsSchemes << "mms://" << "mmsh://" << "mmst://";

    QString uri;

    foreach (QString scheme, mmsSchemes)
    {
        uri = this->uri();

        foreach (QString schemer, mmsSchemes)
            uri.replace(QRegExp(QString("^%1").arg(schemer)),
                        scheme);

        this->m_inputContext = NULL;

        if (avformat_open_input(&this->m_inputContext,
                                uri.toUtf8().constData(),
                                inputFormat,
                                &inputOptions) >= 0)
            break;
    }

    if (inputOptions)
        av_dict_free(&inputOptions);

    if (!this->m_inputContext)
        return false;

    if (avformat_find_stream_info(this->m_inputContext, NULL) < 0)
    {
        avformat_close_input(&this->m_inputContext);

        return false;
    }

    av_dump_format(this->m_inputContext, 0, uri.toUtf8().constData(), false);

    foreach (AbstractStream *stream, this->m_streams)
        delete stream;

    this->m_streams.clear();

    for (uint i = 0; i < this->m_inputContext->nb_streams; i++)
    {
        AVMediaType type = AbstractStream::type(this->m_inputContext, i);
        AbstractStream *stream;

        if (type == AVMEDIA_TYPE_VIDEO)
            stream = new VideoStream(this->m_inputContext, i);
        else if (type == AVMEDIA_TYPE_AUDIO)
            stream = new AudioStream(this->m_inputContext, i);
        else
            stream = new AbstractStream(this->m_inputContext, i);

        if (stream->isValid())
            this->m_streams[i] = stream;
        else
            delete stream;
    }

    return true;
}

void UriSrcElement::uninitCapture()
{
    foreach (AbstractStream *stream, this->m_streams)
        delete stream;

    this->m_streams.clear();

    if (this->m_inputContext)
        avformat_close_input(&this->m_inputContext);
}

void UriSrcElement::setUri(QString uri)
{
    if (uri == this->uri())
        return;

    ElementState preState = this->state();

    this->setState(ElementStateNull);
    this->m_uri = uri;

    if (!this->uri().isEmpty())
        this->setState(preState);
}

void UriSrcElement::setLoop(bool loop)
{
    this->m_loop = loop;
}

void UriSrcElement::resetUri()
{
    this->setUri("");
}

void UriSrcElement::resetLoop()
{
    this->setLoop(false);
}

void UriSrcElement::iStream(const QbPacket &packet)
{
    Q_UNUSED(packet)
}

void UriSrcElement::setState(ElementState state)
{
    ElementState preState = this->state();

    switch (state)
    {
        case ElementStateNull:
            switch (preState)
            {
                case ElementStatePaused:
                case ElementStatePlaying:
                    this->setState(ElementStateReady);

                    if (this->state() != ElementStateReady)
                        return;

                case ElementStateReady:
                    this->uninitCapture();
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        case ElementStateReady:
            switch (preState)
            {
                case ElementStateNull:
                    if (this->initCapture())
                        this->m_state = state;
                    else
                        this->m_state = ElementStateNull;
                break;

                case ElementStatePlaying:
                    this->setState(ElementStatePaused);

                    if (this->state() != ElementStatePaused)
                        return;

                case ElementStatePaused:
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        case ElementStatePaused:
            switch (preState)
            {
                case ElementStateNull:
                    this->setState(ElementStateReady);

                    if (this->state() != ElementStateReady)
                        return;

                case ElementStateReady:
                    this->m_state = state;
                break;

                case ElementStatePlaying:
                    this->m_timer.stop();
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        case ElementStatePlaying:
            switch (preState)
            {
                case ElementStateNull:
                case ElementStateReady:
                    this->setState(ElementStatePaused);

                    if (this->state() != ElementStatePaused)
                        return;

                case ElementStatePaused:
                    this->m_timer.start();
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        default:
        break;
    }
}

void UriSrcElement::readPackets()
{
    if (av_read_frame(this->m_inputContext, &this->m_packet) < 0)
    {
        this->setState(ElementStateNull);

        if (this->loop())
            this->setState(ElementStatePlaying);

        return;
    }

    AbstractStream *stream = this->m_streams[this->m_packet.stream_index];
    QbPacket oPacket = stream->readPacket(&this->m_packet);

    if (oPacket.caps().isValid())
        emit this->oStream(oPacket);

    av_free_packet(&this->m_packet);
}
