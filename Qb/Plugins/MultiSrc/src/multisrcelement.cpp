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

#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "videostream.h"
#include "audiostream.h"
#include "multisrcelement.h"

MultiSrcElement::MultiSrcElement(): QbElement()
{
    av_register_all();
    avdevice_register_all();
    avformat_network_init();

    this->m_location = "";
    this->resetLoop();
    this->m_state = ElementStateNull;

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(readPackets()));
}

MultiSrcElement::~MultiSrcElement()
{
    this->setState(ElementStateNull);
}

QString MultiSrcElement::location()
{
    return this->m_location;
}

bool MultiSrcElement::loop()
{
    return this->m_loop;
}

QSize MultiSrcElement::size()
{
    ElementState preState = this->state();
    QSize size;

    if (preState == ElementStateNull)
    {
        size = this->webcamSize();

        if (!size.isEmpty())
            return size;

        this->setState(ElementStateReady);
    }

    int index = av_find_best_stream(this->m_inputContext,
                                    AVMEDIA_TYPE_VIDEO,
                                    -1,
                                    -1,
                                    NULL,
                                    0);

    if (index >= 0)
    {
        VideoStream *stream = static_cast<VideoStream *>(this->m_streams[index].data());

        if (stream)
            size = stream->size();
    }

    if (preState == ElementStateNull)
        this->setState(ElementStateNull);

    return size;
}

int MultiSrcElement::streamsCount()
{
    int streamsCount;

    ElementState preState = this->state();

    if (preState == ElementStateNull)
        this->setState(ElementStateReady);

    streamsCount = this->m_streamsCount;

    if (preState == ElementStateNull)
        this->setState(ElementStateNull);

    return streamsCount;
}

MultiSrcElement::StreamType MultiSrcElement::streamType(int streamIndex)
{
    ElementState preState = this->state();
    StreamType streamType;

    if (preState == ElementStateNull)
        this->setState(ElementStateReady);

    AVMediaType type = AbstractStream::type(this->m_inputContext, streamIndex);

    switch (type)
    {
        case AVMEDIA_TYPE_VIDEO:
            streamType = StreamTypeVideo;
        break;

        case AVMEDIA_TYPE_AUDIO:
            streamType = StreamTypeAudio;
        break;

        case AVMEDIA_TYPE_DATA:
            streamType = StreamTypeData;
        break;

        case AVMEDIA_TYPE_SUBTITLE:
            streamType = StreamTypeSubtitle;
        break;

        case AVMEDIA_TYPE_ATTACHMENT:
            streamType = StreamTypeAttachment;
        break;

        case AVMEDIA_TYPE_NB:
            streamType = StreamTypeNb;
        break;

        default:
            streamType = StreamTypeUnknown;
        break;

    }

    if (preState == ElementStateNull)
        this->setState(ElementStateNull);

    return streamType;
}

int MultiSrcElement::defaultIndex(StreamType streamType)
{
    ElementState preState = this->state();

    if (preState == ElementStateNull)
        this->setState(ElementStateReady);

    AVMediaType mediaType;

    switch (streamType)
    {
        case StreamTypeVideo:
            mediaType = AVMEDIA_TYPE_VIDEO;
        break;

        case StreamTypeAudio:
            mediaType = AVMEDIA_TYPE_AUDIO;
        break;

        case StreamTypeData:
            mediaType = AVMEDIA_TYPE_DATA;
        break;

        case StreamTypeSubtitle:
            mediaType = AVMEDIA_TYPE_SUBTITLE;
        break;

        case StreamTypeAttachment:
            mediaType = AVMEDIA_TYPE_ATTACHMENT;
        break;

        case StreamTypeNb:
            mediaType = AVMEDIA_TYPE_NB;
        break;

        default:
            mediaType = AVMEDIA_TYPE_UNKNOWN;
        break;

    }

    int streamIndex = av_find_best_stream(this->m_inputContext, mediaType, -1, -1, NULL, 0);

    if (preState == ElementStateNull)
        this->setState(ElementStateNull);

    return streamIndex;
}

QList<QSize> MultiSrcElement::availableSize()
{
    QList<QSize> availableSize = this->webcamAvailableSize();

    if (availableSize.isEmpty())
        availableSize << this->size();

    return availableSize;
}

bool MultiSrcElement::init()
{
    if (this->location().isEmpty())
        return false;

    AVInputFormat *inputFormat = NULL;
    AVDictionary *inputOptions = NULL;

    if (QRegExp("/dev/video\\d*").exactMatch(this->location()))
    {
        QSize size = this->webcamSize();
        inputFormat = av_find_input_format("v4l2");

        av_dict_set(&inputOptions,
                    "video_size",
                    QString("%1x%2").arg(size.width())
                                    .arg(size.height())
                                    .toUtf8().constData(),
                    0);
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
                                    .toUtf8().constData(),
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

    foreach (QString scheme, mmsSchemes)
    {
        uri = this->location();

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

    this->m_streamsCount = this->m_inputContext->nb_streams;
    this->m_streams.clear();

    for (uint i = 0; i < this->m_inputContext->nb_streams; i++)
    {
        AVMediaType type = AbstractStream::type(this->m_inputContext, i);
        QSharedPointer<AbstractStream> stream;

        if (type == AVMEDIA_TYPE_VIDEO)
            stream = QSharedPointer<AbstractStream>(new VideoStream(this->m_inputContext, i));
        else if (type == AVMEDIA_TYPE_AUDIO)
            stream = QSharedPointer<AbstractStream>(new AudioStream(this->m_inputContext, i));
        else
            stream = QSharedPointer<AbstractStream>(new AbstractStream(this->m_inputContext, i));

        if (stream->isValid())
            this->m_streams[i] = stream;
    }

    av_init_packet(&this->m_packet);

    return true;
}

void MultiSrcElement::uninit()
{
    this->m_streams.clear();

    if (this->m_inputContext)
        avformat_close_input(&this->m_inputContext);
}

QSize MultiSrcElement::webcamSize()
{
    QSize size;

    if (!QRegExp("/dev/video\\d*").exactMatch(this->location()))
        return size;

    QFile deviceFile(this->location());

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return size;

    v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(deviceFile.handle(), VIDIOC_G_FMT, &fmt) >= 0)
        size = QSize(fmt.fmt.pix.width,
                     fmt.fmt.pix.height);

    deviceFile.close();

    return size;
}

QList<QSize> MultiSrcElement::webcamAvailableSize()
{
    QList<QSize> availableSize;

    if (!QRegExp("/dev/video\\d*").exactMatch(this->location()))
        return availableSize;

    QFile deviceFile(this->location());

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return availableSize;

    QList<v4l2_buf_type> bufType;

    bufType << V4L2_BUF_TYPE_VIDEO_CAPTURE
            << V4L2_BUF_TYPE_VIDEO_OUTPUT
            << V4L2_BUF_TYPE_VIDEO_OVERLAY;

    foreach (v4l2_buf_type type, bufType)
    {
        v4l2_fmtdesc fmt;
        fmt.index = 0;
        fmt.type = type;

        while (ioctl(deviceFile.handle(), VIDIOC_ENUM_FMT, &fmt) >= 0)
        {
            v4l2_frmsizeenum frmSize;
            frmSize.pixel_format = fmt.pixelformat;
            frmSize.index = 0;

            while (ioctl(deviceFile.handle(),
                         VIDIOC_ENUM_FRAMESIZES,
                         &frmSize) >= 0)
            {
                if (frmSize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                    availableSize << QSize(frmSize.discrete.width,
                                           frmSize.discrete.height);

                frmSize.index++;
            }

            fmt.index++;
        }
    }

    deviceFile.close();

    return availableSize;
}

void MultiSrcElement::setLocation(QString location)
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

void MultiSrcElement::setSize(QSize size)
{
    if (!QRegExp("/dev/video\\d*").exactMatch(this->location()))
        return;

    ElementState preState = this->state();

    if (preState != ElementStateNull)
        this->setState(ElementStateNull);

    QFile deviceFile(this->location());

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
        return;

    v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(deviceFile.handle(), VIDIOC_G_FMT, &fmt) == 0)
    {
        fmt.fmt.pix.width = size.width();
        fmt.fmt.pix.height = size.height();

        ioctl(deviceFile.handle(), VIDIOC_S_FMT, &fmt);
    }

    deviceFile.close();

    this->setState(preState);
    this->m_size = size;
}

void MultiSrcElement::resetLocation()
{
    this->setLocation("");
}

void MultiSrcElement::resetLoop()
{
    this->setLoop(false);
}

void MultiSrcElement::resetSize()
{
    this->setSize(this->availableSize()[0]);
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
    if (av_read_frame(this->m_inputContext, &this->m_packet) < 0)
    {
        this->setState(ElementStateNull);

        if (this->loop())
            this->setState(ElementStatePlaying);

        return;
    }

    AbstractStream *stream = this->m_streams[this->m_packet.stream_index].data();
    QbPacket oPacket = stream->readPacket(&this->m_packet);

    if (oPacket.caps().isValid())
        emit this->oStream(oPacket);

    av_free_packet(&this->m_packet);
}
