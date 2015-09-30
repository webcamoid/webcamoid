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

#include "outputparams.h"

OutputParams::OutputParams(QObject *parent):
    QObject(parent)
{
    this->m_outputIndex = 0;

    this->m_id = -1;
    this->m_pts = -1;
    this->m_ptsDiff = 0;
    this->m_ptsDrift = 0;

    this->m_resampleContext = NULL;
    this->m_scaleContext = NULL;
}

OutputParams::OutputParams(const CodecContextPtr &codecContext,
                           int outputIndex):
    QObject(NULL),
    m_codecContext(codecContext),
    m_outputIndex(outputIndex),
    m_id(-1),
    m_pts(-1),
    m_ptsDiff(0),
    m_ptsDrift(0),
    m_resampleContext(NULL),
    m_scaleContext(NULL)
{
}

OutputParams::OutputParams(const OutputParams &other):
    QObject(other.parent()),
    m_codecContext(other.m_codecContext),
    m_outputIndex(other.m_outputIndex),
    m_id(other.m_id),
    m_pts(other.m_pts),
    m_ptsDiff(other.m_ptsDiff),
    m_ptsDrift(other.m_ptsDrift),
    m_resampleContext(NULL),
    m_scaleContext(NULL)
{
}

OutputParams::~OutputParams()
{
    if (this->m_resampleContext)
        swr_free(&this->m_resampleContext);

    if (this->m_scaleContext)
        sws_freeContext(this->m_scaleContext);
}

OutputParams &OutputParams::operator =(const OutputParams &other)
{
    if (this != &other) {
        this->m_codecContext = other.m_codecContext;
        this->m_outputIndex = other.m_outputIndex;
        this->m_id = other.m_id;
        this->m_pts = other.m_pts;
        this->m_ptsDiff = other.m_ptsDiff;
        this->m_ptsDrift = other.m_ptsDrift;
    }

    return *this;
}

CodecContextPtr OutputParams::codecContext() const
{
    return this->m_codecContext;
}

CodecContextPtr &OutputParams::codecContext()
{
    return this->m_codecContext;
}

int OutputParams::outputIndex() const
{
    return this->m_outputIndex;
}

int &OutputParams::outputIndex()
{
    return this->m_outputIndex;
}

qint64 OutputParams::nextPts(qint64 pts, qint64 id)
{
    if (this->m_pts < 0 || this->m_id < 0) {
        this->m_ptsDrift = -pts;
        this->m_pts = pts;
        this->m_id = id;

        return 0;
    }

    if (id != this->m_id) {
        this->m_ptsDrift += this->m_pts - pts + this->m_ptsDiff;
        this->m_pts = pts;
        this->m_id = id;

        return pts + this->m_ptsDrift;
    }

    if (pts <= this->m_pts) {
        this->m_ptsDrift += 2 * (this->m_pts - pts) + 1;
        this->m_pts = pts;

        return pts + this->m_ptsDrift;
    }

    this->m_ptsDiff = pts - this->m_pts;
    this->m_pts = pts;

    return pts + this->m_ptsDrift;
}

void OutputParams::setCodecContext(const CodecContextPtr &codecContext)
{
    if (this->m_codecContext == codecContext)
        return;

    this->m_codecContext = codecContext;
    emit this->codecContextChanged(codecContext);
}

void OutputParams::setOutputIndex(int outputIndex)
{
    if (this->m_outputIndex == outputIndex)
        return;

    this->m_outputIndex = outputIndex;
    emit this->outputIndexChanged(outputIndex);
}

void OutputParams::resetCodecContext()
{
    this->setCodecContext(CodecContextPtr());
}

void OutputParams::resetOutputIndex()
{
    this->setOutputIndex(0);
}

bool OutputParams::convert(const QbPacket &packet, AVFrame *frame)
{
    if (packet.caps().mimeType() == "audio/x-raw")
        return this->convertAudio(packet, frame);
    else if (packet.caps().mimeType() == "video/x-raw")
        return this->convertVideo(packet, frame);

    return false;
}

bool OutputParams::convertAudio(const QbPacket &packet, AVFrame *frame)
{
    QString layout = packet.caps().property("layout").toString();
    int64_t iLayout = av_get_channel_layout(layout.toStdString().c_str());;
    QString format = packet.caps().property("format").toString();
    AVSampleFormat iFormat = av_get_sample_fmt(format.toStdString().c_str());
    int iSampleRate = packet.caps().property("rate").toInt();

    int64_t oLayout = this->m_codecContext->channel_layout;
    AVSampleFormat oFormat = this->m_codecContext->sample_fmt;
    int oSampleRate = this->m_codecContext->sample_rate;

    this->m_resampleContext = swr_alloc_set_opts(this->m_resampleContext,
                                                 oLayout,
                                                 oFormat,
                                                 oSampleRate,
                                                 iLayout,
                                                 iFormat,
                                                 iSampleRate,
                                                 0,
                                                 NULL);

    if (!this->m_resampleContext)
        return false;

    if (!swr_is_initialized(this->m_resampleContext))
        if (swr_init(this->m_resampleContext) < 0)
            return false;

    static AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    int iChannels = packet.caps().property("channels").toInt();
    int iSamples = packet.caps().property("samples").toInt();

    if (av_samples_fill_arrays(iFrame.data,
                               iFrame.linesize,
                               (const uint8_t *) packet.buffer().data(),
                               iChannels,
                               iSamples,
                               iFormat,
                               1) < 0)
        return false;

    iFrame.channels = iChannels;
    iFrame.channel_layout = iLayout;
    iFrame.format = iFormat;
    iFrame.sample_rate = iSampleRate;
    iFrame.nb_samples = packet.caps().property("samples").toInt();
    iFrame.pts = iFrame.pkt_pts = packet.pts();

    int oNSamples = swr_get_delay(this->m_resampleContext, oSampleRate)
                    + iFrame.nb_samples * (int64_t) oSampleRate / iSampleRate
                    + 3;

    frame->channels = this->m_codecContext->channels;
    frame->channel_layout = oLayout;
    frame->format = oFormat;
    frame->sample_rate = oSampleRate;
    frame->nb_samples = oNSamples;
    frame->pts = frame->pkt_pts = swr_next_pts(this->m_resampleContext,
                                               iFrame.pts);

    av_frame_get_buffer(frame, 0);

    // convert to destination format
    frame->nb_samples = swr_convert(this->m_resampleContext,
                                    frame->data,
                                    frame->nb_samples,
                                    (const uint8_t **) iFrame.data,
                                    iFrame.nb_samples);

    if (frame->nb_samples < 1)
        return false;

    return true;
}

bool OutputParams::convertVideo(const QbPacket &packet, AVFrame *frame)
{
    QString format = packet.caps().property("format").toString();
    AVPixelFormat iFormat = av_get_pix_fmt(format.toStdString().c_str());
    int iWidth = packet.caps().property("width").toInt();
    int iHeight = packet.caps().property("height").toInt();

    AVPixelFormat oFormat = this->m_codecContext->pix_fmt;
    int oWidth = this->m_codecContext->width;
    int oHeight = this->m_codecContext->height;

    this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                iWidth,
                                                iHeight,
                                                iFormat,
                                                oWidth,
                                                oHeight,
                                                oFormat,
                                                SWS_FAST_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);

    if (!this->m_scaleContext)
        return false;

    AVPicture iPicture;
    memset(&iPicture, 0, sizeof(AVPicture));

    avpicture_fill(&iPicture,
                   (const uint8_t *) packet.buffer().data(),
                   iFormat,
                   iWidth,
                   iHeight);

    if (avpicture_alloc((AVPicture *) frame, oFormat, oWidth, oHeight) < 0)
        return false;

    frame->format = oFormat;
    frame->width = oWidth;
    frame->height = oHeight;
    frame->pts = frame->pkt_pts = qRound(packet.pts()
                                         * packet.timeBase().value()
                                         * this->m_codecContext->time_base.den
                                         / this->m_codecContext->time_base.num);

    sws_scale(this->m_scaleContext,
              (uint8_t **) iPicture.data,
              iPicture.linesize,
              0,
              iHeight,
              frame->data,
              frame->linesize);

    return true;
}
