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

#include "videostream.h"

typedef QMap<AVPixelFormat, QbVideoCaps::VideoFormat> AVPixelFormatMap;

inline AVPixelFormatMap initAVPixelFormatMap()
{
    AVPixelFormatMap outputFormats;
    outputFormats[AV_PIX_FMT_MONOBLACK] = QbVideoCaps::Format_monob;
    outputFormats[AV_PIX_FMT_BGR0] = QbVideoCaps::Format_bgr0;
    outputFormats[AV_PIX_FMT_BGRA] = QbVideoCaps::Format_bgra;
    outputFormats[AV_PIX_FMT_RGB565LE] = QbVideoCaps::Format_rgb565le;
    outputFormats[AV_PIX_FMT_RGB555LE] = QbVideoCaps::Format_rgb555le;
    outputFormats[AV_PIX_FMT_BGR24] = QbVideoCaps::Format_bgr24;
    outputFormats[AV_PIX_FMT_RGB444LE] = QbVideoCaps::Format_rgb444le;
    outputFormats[AV_PIX_FMT_GRAY8] = QbVideoCaps::Format_gray;

    return outputFormats;
}

Q_GLOBAL_STATIC_WITH_ARGS(AVPixelFormatMap, outputFormats, (initAVPixelFormatMap()))

VideoStream::VideoStream(const AVFormatContext *formatContext,
                         uint index, qint64 id, bool noModify, QObject *parent):
    AbstractStream(formatContext, index, id, noModify, parent)
{
    this->m_scaleContext = NULL;
    this->m_frameBuffer.setMaxSize(3);
}

VideoStream::~VideoStream()
{
    if (this->m_scaleContext)
        sws_freeContext(this->m_scaleContext);
}

QbCaps VideoStream::caps() const
{
    QbVideoCaps::VideoFormat format = outputFormats->contains(this->codecContext()->pix_fmt)?
                               outputFormats->value(this->codecContext()->pix_fmt):
                               QbVideoCaps::Format_bgra;

    QbVideoCaps caps;
    caps.isValid() = true;
    caps.format() = format;
    caps.width() = this->codecContext()->width;
    caps.height() = this->codecContext()->height;
    caps.fps() = this->fps();

    return caps.toCaps();
}

void VideoStream::processPacket(AVPacket *packet)
{
    if (!this->isValid())
        return;

    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    int gotFrame;

    avcodec_decode_video2(this->codecContext(),
                          &iFrame,
                          &gotFrame,
                          packet);

    if (!gotFrame)
        return;

    QbPacket oPacket = this->convert(&iFrame);

    emit this->oStream(oPacket);
}

QbFrac VideoStream::fps() const
{
    QbFrac fps;

    if (this->stream()->avg_frame_rate.num
        && this->stream()->avg_frame_rate.den)
        fps = QbFrac(this->stream()->avg_frame_rate.num,
                     this->stream()->avg_frame_rate.den);
    else
        fps = QbFrac(this->stream()->r_frame_rate.num,
                     this->stream()->r_frame_rate.den);

    return fps;
}

QbPacket VideoStream::convert(AVFrame *iFrame)
{
    AVPicture *oPicture;
    AVPixelFormat oFormat;
    bool delFrame = false;

    if (outputFormats->contains(AVPixelFormat(iFrame->format))) {
        oPicture = (AVPicture *) iFrame;
        oFormat = AVPixelFormat(iFrame->format);
    }
    else {
        oPicture = new AVPicture;
        oFormat = AV_PIX_FMT_BGRA;

        avpicture_alloc(oPicture,
                        oFormat,
                        iFrame->width,
                        iFrame->height);

        this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                    iFrame->width,
                                                    iFrame->height,
                                                    AVPixelFormat(iFrame->format),
                                                    iFrame->width,
                                                    iFrame->height,
                                                    oFormat,
                                                    SWS_FAST_BILINEAR,
                                                    NULL,
                                                    NULL,
                                                    NULL);

        sws_scale(this->m_scaleContext,
                  (uint8_t **) iFrame->data,
                  iFrame->linesize,
                  0,
                  iFrame->height,
                  oPicture->data,
                  oPicture->linesize);

        delFrame = true;
    }

    QbVideoPacket packet;
    packet.caps().isValid() = true;
    packet.caps().format() = outputFormats->value(oFormat);
    packet.caps().width() = iFrame->width;
    packet.caps().height() = iFrame->height;
    packet.caps().fps() = this->fps();

    int frameSize = avpicture_get_size(oFormat,
                                       iFrame->width,
                                       iFrame->height);

    QbBufferPtr oBuffer(new char[frameSize]);

    avpicture_layout(oPicture,
                     oFormat,
                     iFrame->width,
                     iFrame->height,
                     (uint8_t *) oBuffer.data(),
                     frameSize);

    packet.buffer() = oBuffer;
    packet.bufferSize() = frameSize;
    packet.pts() = av_frame_get_best_effort_timestamp(iFrame);
    packet.timeBase() = this->timeBase();
    packet.index() = this->index();
    packet.id() = this->id();

    if (delFrame) {
        avpicture_free(oPicture);
        delete oPicture;
    }

    return packet.toPacket();
}
