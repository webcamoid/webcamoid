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

#include "filterelement.h"

FilterElement::FilterElement(): QbElement()
{
    avfilter_register_all();

    this->m_bufferSinkContext = NULL;
    this->m_bufferSrcContext = NULL;
    this->m_filterGraph = NULL;

    this->resetDescription();
    this->resetFormat();
    this->resetPixelAspect();
}

FilterElement::~FilterElement()
{
    this->uninit();
}

QString FilterElement::description()
{
    return this->m_description;
}

QString FilterElement::format()
{
    if (this->m_format.isEmpty())
        return this->m_curInputCaps.property("format").toString();
    else
        return this->m_format;
}

QbFrac FilterElement::pixelAspect()
{
    return this->m_pixelAspect;
}

bool FilterElement::initBuffers()
{
    this->m_filterGraph = avfilter_graph_alloc();
    QString args;

    AVFilter *buffersrc;

    if (this->m_curInputCaps.mimeType() == "video/x-raw")
    {
        int width = this->m_curInputCaps.property("width").toInt();
        int height = this->m_curInputCaps.property("height").toInt();
        QString format = this->m_curInputCaps.property("format").toString();

        args = QString("video_size=%1x%2:"
                       "pix_fmt=%3").arg(width)
                                    .arg(height)
                                    .arg(av_get_pix_fmt(format.toStdString().c_str()));

        if (this->m_timeBase.num() || this->m_timeBase.den())
            args.append(QString(":time_base=%1").arg(this->m_timeBase.toString()));

        if (this->m_pixelAspect.num() || this->m_pixelAspect.den())
            args.append(QString(":pixel_aspect=%1").arg(this->m_pixelAspect.toString()));

        buffersrc = avfilter_get_by_name("buffer");
    }
    else if (this->m_curInputCaps.mimeType() == "audio/x-raw")
    {
        QString format = this->m_curInputCaps.property("format").toString();
        int rate = this->m_curInputCaps.property("rate").toInt();
        QString layoutString = this->m_curInputCaps.property("layout").toString();
        uint64_t layout = av_get_channel_layout(layoutString.toStdString().c_str());

        args = QString("sample_fmt=%1:"
                       "sample_rate=%2:"
                       "channel_layout=0x%3").arg(format)
                                             .arg(rate)
                                             .arg(QString::number(layout, 16));

        if (this->m_timeBase.num() || this->m_timeBase.den())
            args.append(QString(":time_base=%1").arg(this->m_timeBase.toString()));

        buffersrc = avfilter_get_by_name("abuffer");
    }
    else
        return false;

    // buffer video source: the decoded frames from the decoder will be inserted here.
    if (avfilter_graph_create_filter(&this->m_bufferSrcContext,
                                     buffersrc,
                                     "in",
                                     args.toStdString().c_str(),
                                     NULL,
                                     this->m_filterGraph) < 0)
        return false;

    // buffer sink: to terminate the filter chain.
    void *buffersinkParams = NULL;
    AVFilter *buffersink;

    if (this->m_curInputCaps.mimeType() == "video/x-raw")
    {
        PixelFormat oFormat;

        if (this->format().isEmpty())
        {
            QString format = this->m_curInputCaps.property("format").toString();

            if (format.isEmpty())
                return false;

            oFormat = av_get_pix_fmt(format.toStdString().c_str());
        }
        else
            oFormat = av_get_pix_fmt(this->format().toStdString().c_str());

        PixelFormat pixelFormats[2];
        pixelFormats[0] = oFormat;
        pixelFormats[1] = PIX_FMT_NONE;

        AVBufferSinkParams *params = av_buffersink_params_alloc();
        params->pixel_fmts = pixelFormats;
        buffersinkParams = params;
        buffersink = avfilter_get_by_name("ffbuffersink");
    }
    else if (this->m_curInputCaps.mimeType() == "audio/x-raw")
    {
        AVSampleFormat oFormat;

        if (this->format().isEmpty())
        {
            QString format = this->m_curInputCaps.property("format").toString();

            if (format.isEmpty())
                return false;

            oFormat = av_get_sample_fmt(format.toStdString().c_str());
        }
        else
            oFormat = av_get_sample_fmt(this->format().toStdString().c_str());

        AVSampleFormat sampleFormats[2];
        sampleFormats[0] = oFormat;
        sampleFormats[1] = AV_SAMPLE_FMT_NONE;

        AVABufferSinkParams *params = av_abuffersink_params_alloc();
        params->sample_fmts = sampleFormats;
        buffersinkParams = params;
        buffersink = avfilter_get_by_name("abuffersink");
    }
    else
        return false;

    if (avfilter_graph_create_filter(&this->m_bufferSinkContext,
                                     buffersink,
                                     "out",
                                     NULL,
                                     buffersinkParams,
                                     this->m_filterGraph) < 0)
    {
        av_free(buffersinkParams);

        return false;
    }

    if (this->m_curInputCaps.mimeType() == "video/x-raw")
        av_free((AVBufferSinkParams *) buffersinkParams);
    else if (this->m_curInputCaps.mimeType() == "audio/x-raw")
        av_free((AVABufferSinkParams *) buffersinkParams);

    // Endpoints for the filter graph.
    AVFilterInOut *outputs = avfilter_inout_alloc();
    outputs->name = av_strdup("in");
    outputs->filter_ctx = this->m_bufferSrcContext;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    AVFilterInOut *inputs = avfilter_inout_alloc();
    inputs->name = av_strdup("out");
    inputs->filter_ctx = this->m_bufferSinkContext;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if (avfilter_graph_parse(this->m_filterGraph,
                             this->m_description.toStdString().c_str(),
                             &inputs,
                             &outputs,
                             NULL) < 0)
        return false;

    if (avfilter_graph_config(this->m_filterGraph, NULL) < 0)
        return false;

    return true;
}

void FilterElement::uninitBuffers()
{
    if (this->m_filterGraph)
        avfilter_graph_free(&this->m_filterGraph);
}

void FilterElement::setDescription(QString description)
{
    this->m_description = description;
}

void FilterElement::setFormat(QString format)
{
    this->m_format = format;
}

void FilterElement::setPixelAspect(QbFrac pixelAspect)
{
    this->m_pixelAspect = pixelAspect;
}

void FilterElement::resetDescription()
{
    this->setDescription("null");
}

void FilterElement::resetFormat()
{
    this->setFormat("");
}

void FilterElement::resetPixelAspect()
{
    this->setPixelAspect(QbFrac(1, 1));
}

void FilterElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        this->state() != ElementStatePlaying)
        return;

    if (packet.caps() != this->m_curInputCaps)
    {
        this->uninitBuffers();
        this->m_curInputCaps = packet.caps();
        this->m_timeBase = packet.timeBase();
        this->initBuffers();
    }

    AVFrame frame;
    avcodec_get_frame_defaults(&frame);

    if (packet.caps().mimeType() == "video/x-raw")
    {
        int width = packet.caps().property("width").toInt();
        int height = packet.caps().property("height").toInt();
        QString format = packet.caps().property("format").toString();

        if (avpicture_fill((AVPicture *) &frame,
                           (uint8_t *) packet.buffer().data(),
                           av_get_pix_fmt(format.toStdString().c_str()),
                           width,
                           height) < 0)
            return;

        frame.format = av_get_pix_fmt(format.toStdString().c_str());
        frame.width = width;
        frame.height = height;
        frame.type = AVMEDIA_TYPE_VIDEO;
    }
    else if (packet.caps().mimeType() == "audio/x-raw")
    {
        QString format = packet.caps().property("format").toString();
        int channels = packet.caps().property("channels").toInt();
        int rate = packet.caps().property("rate").toInt();
        QString layout = packet.caps().property("layout").toString();
        int samples = packet.caps().property("samples").toInt();

        frame.format = av_get_sample_fmt(format.toStdString().c_str());
        frame.channels = channels;
        frame.sample_rate = rate;
        frame.channel_layout = av_get_channel_layout(layout.toStdString().c_str());
        frame.nb_samples = samples;
        frame.type = AVMEDIA_TYPE_AUDIO;

        if (avcodec_fill_audio_frame(&frame,
                                     frame.channels,
                                     (AVSampleFormat) frame.format,
                                     (uint8_t *) packet.buffer().data(),
                                     packet.bufferSize(),
                                     1) < 0)
            return;
    }
    else
        return;

    frame.pkt_pts = packet.pts();
    frame.pkt_dts = packet.pts();
    frame.pkt_duration = packet.duration();
    frame.pts = packet.pts();

    // push the decoded frame into the filtergraph
    if (av_buffersrc_add_frame(this->m_bufferSrcContext, &frame, 0) < 0)
        return;

    AVFilterBufferRef *filterBufferRef = NULL;

    // pull filtered pictures from the filtergraph
    while (true)
    {
        if (av_buffersink_get_buffer_ref(this->m_bufferSinkContext,
                                         &filterBufferRef,
                                         0) < 0)
            return;

        QbPacket oPacket;
        AVFrame oFrame;

        avfilter_copy_buf_props(&oFrame, filterBufferRef);

        QbFrac timeBase(this->m_bufferSinkContext->inputs[0]->time_base.num,
                        this->m_bufferSinkContext->inputs[0]->time_base.den);

        if (this->m_bufferSinkContext->inputs[0]->type == AVMEDIA_TYPE_UNKNOWN)
            return;
        else if (this->m_bufferSinkContext->inputs[0]->type == AVMEDIA_TYPE_VIDEO)
        {
            int frameSize = avpicture_get_size((PixelFormat) oFrame.format,
                                               oFrame.width,
                                               oFrame.height);

            QSharedPointer<uchar> oBuffer(new uchar[frameSize]);

            avpicture_layout((AVPicture *) &oFrame,
                             (PixelFormat) oFrame.format,
                             oFrame.width,
                             oFrame.height,
                             (uint8_t *) oBuffer.data(),
                             frameSize);

            QString format = av_get_pix_fmt_name((PixelFormat) oFrame.format);

            QbCaps caps(packet.caps());
            caps.setProperty("format", format);
            caps.setProperty("width", oFrame.width);
            caps.setProperty("height", oFrame.height);
            caps.setProperty("fps", timeBase.invert().toString());

            oPacket = QbPacket(caps,
                               oBuffer,
                               frameSize);
        }
        else if (this->m_bufferSinkContext->inputs[0]->type == AVMEDIA_TYPE_AUDIO)
        {
            int frameSize = av_samples_get_buffer_size(NULL,
                                                       oFrame.channels,
                                                       oFrame.nb_samples,
                                                       (AVSampleFormat) oFrame.format,
                                                       1);

            uint8_t **oFrameBuffer = NULL;

            int oBufferLineSize;

            int ret = av_samples_alloc(oFrameBuffer,
                                       &oBufferLineSize,
                                       oFrame.channels,
                                       oFrame.nb_samples,
                                       (AVSampleFormat) oFrame.format,
                                       1);

            if (ret < 0)
                return;

            av_samples_copy(oFrameBuffer,
                            oFrame.data,
                            0,
                            0,
                            oFrame.nb_samples,
                            oFrame.channels,
                            (AVSampleFormat) oFrame.format);

            QSharedPointer<uchar> oBuffer(new uchar[frameSize]);
            memcpy(oBuffer.data(), oFrameBuffer[0], frameSize);
            av_freep(&oFrameBuffer[0]);

            const char *format = av_get_sample_fmt_name((AVSampleFormat) oFrame.format);
            char layout[256];

            av_get_channel_layout_string(layout,
                                         sizeof(layout),
                                         oFrame.channels,
                                         oFrame.channel_layout);

            QbCaps caps(packet.caps());
            caps.setProperty("format", format);
            caps.setProperty("channels", oFrame.channels);
            caps.setProperty("rate", oFrame.sample_rate);
            caps.setProperty("layout", layout);
            caps.setProperty("samples", oFrame.nb_samples);

            oPacket = QbPacket(caps,
                               oBuffer,
                               frameSize);
        }
        else if (this->m_bufferSinkContext->inputs[0]->type == AVMEDIA_TYPE_DATA)
            return;
        else if (this->m_bufferSinkContext->inputs[0]->type == AVMEDIA_TYPE_SUBTITLE)
            return;
        else if (this->m_bufferSinkContext->inputs[0]->type == AVMEDIA_TYPE_ATTACHMENT)
            return;
        else if (this->m_bufferSinkContext->inputs[0]->type == AVMEDIA_TYPE_NB)
            return;
        else
            return;

        oPacket.setPts(filterBufferRef->pts);
        oPacket.setDuration((packet.timeBase() == timeBase)? packet.duration(): 1);
        oPacket.setTimeBase(timeBase);
        oPacket.setIndex(packet.index());

        emit this->oStream(oPacket);

        avfilter_unref_bufferp(&filterBufferRef);
    }
}
