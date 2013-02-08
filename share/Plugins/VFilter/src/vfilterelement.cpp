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

#include "vfilterelement.h"

VFilterElement::VFilterElement(): QbElement()
{
//    av_register_all();
    avfilter_register_all();

    this->m_oFrameSize = 0;
    this->m_bufferSinkContext = NULL;
    this->m_bufferSrcContext = NULL;
    this->m_filterGraph = NULL;

    this->resetDescription();
    this->resetFormat();
    this->resetTimeBase();
    this->resetPixelAspect();

    this->m_formatToFF["I420"] = PIX_FMT_YUV420P;
    this->m_formatToFF["YUY2"] = PIX_FMT_YUV422P;
    this->m_formatToFF["UYVY"] = PIX_FMT_UYVY422;
    this->m_formatToFF["AYUV"] = PIX_FMT_YUVA420P;
    this->m_formatToFF["RGBx"] = PIX_FMT_RGB0;
    this->m_formatToFF["BGRx"] = PIX_FMT_BGR0;
    this->m_formatToFF["xRGB"] = PIX_FMT_0RGB;
    this->m_formatToFF["xBGR"] = PIX_FMT_0BGR;
    this->m_formatToFF["RGBA"] = PIX_FMT_RGBA;
    this->m_formatToFF["BGRA"] = PIX_FMT_BGRA;
    this->m_formatToFF["ARGB"] = PIX_FMT_ARGB;
    this->m_formatToFF["ABGR"] = PIX_FMT_ABGR;
    this->m_formatToFF["RGB"] = PIX_FMT_RGB24;
    this->m_formatToFF["BGR"] = PIX_FMT_BGR24;
    this->m_formatToFF["Y41B"] = PIX_FMT_YUV411P;
    this->m_formatToFF["Y42B"] = PIX_FMT_YUV422P;
    this->m_formatToFF["YVYU"] = PIX_FMT_UYVY422;
    this->m_formatToFF["Y444"] = PIX_FMT_YUV444P;
    this->m_formatToFF["v210"] = PIX_FMT_YUV422P10LE;
    this->m_formatToFF["v216"] = PIX_FMT_YUV422P16LE;
    this->m_formatToFF["NV12"] = PIX_FMT_NV12;
    this->m_formatToFF["NV21"] = PIX_FMT_NV21;
    this->m_formatToFF["GRAY8"] = PIX_FMT_GRAY8;
    this->m_formatToFF["GRAY16_BE"] = PIX_FMT_GRAY16BE;
    this->m_formatToFF["GRAY16_LE"] = PIX_FMT_GRAY16LE;
    this->m_formatToFF["v308"] = PIX_FMT_YUV444P;
    this->m_formatToFF["RGB16"] = PIX_FMT_RGB565LE;
    this->m_formatToFF["BGR16"] = PIX_FMT_BGR565LE;
    this->m_formatToFF["RGB15"] = PIX_FMT_RGB555LE;
    this->m_formatToFF["BGR15"] = PIX_FMT_BGR555LE;
    this->m_formatToFF["UYVP"] = PIX_FMT_YUV422P12LE;
    this->m_formatToFF["A420"] = PIX_FMT_YUVA420P;
    this->m_formatToFF["RGB8P"] = PIX_FMT_RGB8;
    this->m_formatToFF["IYU1"] = PIX_FMT_YUV411P;
    this->m_formatToFF["I420_10LE"] = PIX_FMT_YUV420P10LE;
    this->m_formatToFF["I420_10BE"] = PIX_FMT_YUV420P10BE;
    this->m_formatToFF["I422_10LE"] = PIX_FMT_YUV422P10LE;
    this->m_formatToFF["I422_10BE"] = PIX_FMT_YUV422P10BE;
}

VFilterElement::~VFilterElement()
{
    this->uninit();
}

QList<QbCaps> VFilterElement::oCaps()
{
    QList<QbCaps> caps;

    if (!this->m_srcs.isEmpty())
    {
        foreach (QbElement *src, this->m_srcs)
            foreach (QbCaps cap, src->oCaps())
                if (cap.mimeType() == "video/x-raw")
                {
                    cap.setProperty("format", this->m_format);
                    caps << cap;

                    return caps;
                }
    }

    return caps;
}

QString VFilterElement::description()
{
    return this->m_description;
}

QString VFilterElement::format()
{
    if (this->m_format.isEmpty())
        return this->m_curInputCaps.property("format").toString();
    else
        return this->m_format;
}

QString VFilterElement::timeBase()
{
    return QString("%1/%2").arg(this->m_timeBaseNum)
                           .arg(this->m_timeBaseDen);
}

QString VFilterElement::pixelAspect()
{
    return QString("%1/%2").arg(this->m_pixelAspectNum)
                           .arg(this->m_pixelAspectDen);
}

bool VFilterElement::init()
{
    AVFilter *buffersrc = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("ffbuffersink");

    this->m_filterGraph = avfilter_graph_alloc();
    int width = this->m_curInputCaps.property("width").toInt();
    int height = this->m_curInputCaps.property("height").toInt();
    QString format = this->m_curInputCaps.property("format").toString();

    // buffer video source: the decoded frames from the decoder will be inserted here.
    QString args = QString("video_size=%1x%2:"
                           "pix_fmt=%3").arg(width)
                                        .arg(height)
                                        .arg(this->m_formatToFF[format]);

    if (this->m_timeBaseNum || this->m_timeBaseDen)
        args.append(QString(":time_base=%1/%2").arg(this->m_timeBaseNum)
                                               .arg(this->m_timeBaseDen));

    if (this->m_pixelAspectNum || this->m_pixelAspectDen)
        args.append(QString(":pixel_aspect=%1/%2").arg(this->m_pixelAspectNum)
                                                  .arg(this->m_pixelAspectDen));

    if (avfilter_graph_create_filter(&this->m_bufferSrcContext,
                                     buffersrc,
                                     "in",
                                     args.toUtf8().constData(),
                                     NULL,
                                     this->m_filterGraph) < 0)
        return false;

    PixelFormat oFormat;

    if (this->format().isEmpty())
    {
        QString format = this->m_curInputCaps.property("format").toString();

        if (format.isEmpty() || !this->m_formatToFF.contains(format))
            return false;

        oFormat = this->m_formatToFF[format];
    }
    else
    {
        if (!this->m_formatToFF.contains(this->format()))
            return false;

        oFormat = this->m_formatToFF[this->format()];
    }

    PixelFormat pixelFormats[2];
    pixelFormats[0] = oFormat;
    pixelFormats[1] = PIX_FMT_NONE;

    // buffer video sink: to terminate the filter chain.
    AVBufferSinkParams *buffersinkParams = av_buffersink_params_alloc();
    buffersinkParams->pixel_fmts = pixelFormats;

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

    av_free(buffersinkParams);

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
                             this->m_description.toUtf8().constData(),
                             &inputs,
                             &outputs,
                             NULL) < 0)
        return false;

    if (avfilter_graph_config(this->m_filterGraph, NULL) < 0)
        return false;

    return true;
}

void VFilterElement::uninit()
{
    if (this->m_filterGraph)
        avfilter_graph_free(&this->m_filterGraph);
}

void VFilterElement::setDescription(QString description)
{
    this->m_description = description;
}

void VFilterElement::setFormat(QString format)
{
    this->m_format = format;
}

void VFilterElement::setTimeBase(QString timeBase)
{
    if (QRegExp("\\s*\\d+\\s*/\\s*\\d+\\s*").exactMatch(timeBase))
    {
        QStringList t = timeBase.split("/", QString::SkipEmptyParts);

        this->m_timeBaseNum = t[0].trimmed().toInt();
        this->m_timeBaseDen = t[1].trimmed().toInt();
    }
    else
    {
        this->m_timeBaseNum = 0;
        this->m_timeBaseDen = 0;
    }
}

void VFilterElement::setPixelAspect(QString pixelAspect)
{
    if (QRegExp("\\s*\\d+\\s*/\\s*\\d+\\s*").exactMatch(pixelAspect))
    {
        QStringList p = pixelAspect.split("/", QString::SkipEmptyParts);

        this->m_pixelAspectNum = p[0].trimmed().toInt();
        this->m_pixelAspectDen = p[1].trimmed().toInt();
    }
    else
    {
        this->m_pixelAspectNum = 0;
        this->m_pixelAspectDen = 0;
    }
}

void VFilterElement::resetDescription()
{
    this->setDescription("null");
}

void VFilterElement::resetFormat()
{
    this->setFormat("");
}

void VFilterElement::resetTimeBase()
{
    this->setTimeBase("0/0");
}

void VFilterElement::resetPixelAspect()
{
    this->setPixelAspect("0/0");
}

void VFilterElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    if (packet.caps() != this->m_curInputCaps)
    {
        this->uninit();
        this->m_curInputCaps = packet.caps();
        this->init();
    }

    int iWidth = packet.caps().property("width").toInt();
    int iHeight = packet.caps().property("height").toInt();
    QString iFormat = packet.caps().property("format").toString();

    AVFrame frame;

    avcodec_get_frame_defaults(&frame);

    avpicture_fill((AVPicture *) &frame,
                   (uint8_t *) packet.data(),
                   this->m_formatToFF[iFormat],
                   iWidth,
                   iHeight);

    frame.format = this->m_formatToFF[iFormat],
    frame.width = iWidth,
    frame.height = iHeight;
    frame.type = AVMEDIA_TYPE_VIDEO;

    frame.pkt_pts = packet.pts();
    frame.pkt_dts = packet.dts();
    frame.pkt_duration = packet.duration();
    frame.pts = av_frame_get_best_effort_timestamp(&frame);

    // push the decoded frame into the filtergraph
    if (av_buffersrc_add_frame(this->m_bufferSrcContext, &frame, 0) < 0)
        return;

    AVFilterBufferRef *filterBufferRef = NULL;

    // pull filtered pictures from the filtergraph
    while (true)
    {
        int ret = av_buffersink_get_buffer_ref(this->m_bufferSinkContext,
                                               &filterBufferRef,
                                               0);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0 || !filterBufferRef)
            return;

        int frameSize = avpicture_get_size((PixelFormat) filterBufferRef->format,
                                           filterBufferRef->video->w,
                                           filterBufferRef->video->h);

        if (frameSize != this->m_oFrameSize)
        {
            this->m_oFrame.resize(frameSize);
            this->m_oFrameSize = frameSize;
        }

        AVFrame oFrame;

        avfilter_copy_buf_props(&oFrame, filterBufferRef);

        avpicture_layout((AVPicture *) &oFrame,
                         (PixelFormat) filterBufferRef->format,
                         filterBufferRef->video->w,
                         filterBufferRef->video->h,
                         (uint8_t *) this->m_oFrame.data(),
                         this->m_oFrame.size());

        QString format = this->m_formatToFF.key((PixelFormat) filterBufferRef->format);

        QbPacket oPacket(QString("video/x-raw,"
                                 "format=%1,"
                                 "width=%2,"
                                 "height=%3").arg(format)
                                             .arg(filterBufferRef->video->w)
                                             .arg(filterBufferRef->video->h),
                         this->m_oFrame.constData(),
                         this->m_oFrame.size());

        oPacket.setDts(packet.dts());
        oPacket.setPts(packet.pts());
        oPacket.setDuration(packet.duration());
        oPacket.setTimeBase(packet.timeBase());
        oPacket.setIndex(packet.index());

        emit this->oStream(oPacket);

        avfilter_unref_bufferp(&filterBufferRef);
    }
}
