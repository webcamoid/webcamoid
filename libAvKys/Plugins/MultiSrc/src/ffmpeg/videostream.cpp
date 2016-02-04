/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

// no AV sync correction is done if below the minimum AV sync threshold
#define AV_SYNC_THRESHOLD_MIN 0.01

// AV sync correction is done if above the maximum AV sync threshold
#define AV_SYNC_THRESHOLD_MAX 0.1

// If a frame duration is longer than this, it will not be duplicated to compensate AV sync
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

// no AV correction is done if too big error
#define AV_NOSYNC_THRESHOLD 10.0

typedef QMap<AVPixelFormat, AkVideoCaps::PixelFormat> AVPixelFormatMap;

inline AVPixelFormatMap initAVPixelFormatMap()
{
    AVPixelFormatMap outputFormats;
    outputFormats[AV_PIX_FMT_MONOBLACK] = AkVideoCaps::Format_monob;
    outputFormats[AV_PIX_FMT_BGR0] = AkVideoCaps::Format_bgr0;
    outputFormats[AV_PIX_FMT_BGRA] = AkVideoCaps::Format_bgra;
    outputFormats[AV_PIX_FMT_RGB565LE] = AkVideoCaps::Format_rgb565le;
    outputFormats[AV_PIX_FMT_RGB555LE] = AkVideoCaps::Format_rgb555le;
    outputFormats[AV_PIX_FMT_BGR24] = AkVideoCaps::Format_bgr24;
    outputFormats[AV_PIX_FMT_RGB444LE] = AkVideoCaps::Format_rgb444le;
    outputFormats[AV_PIX_FMT_GRAY8] = AkVideoCaps::Format_gray;

    return outputFormats;
}

Q_GLOBAL_STATIC_WITH_ARGS(AVPixelFormatMap, outputFormats, (initAVPixelFormatMap()))

VideoStream::VideoStream(const AVFormatContext *formatContext,
                         uint index, qint64 id, Clock *globalClock,
                         bool noModify, QObject *parent):
    AbstractStream(formatContext, index, id, globalClock, noModify, parent)
{
    this->m_maxData = 3;
    this->m_scaleContext = NULL;
    this->m_lastPts = 0;
}

VideoStream::~VideoStream()
{
    if (this->m_scaleContext)
        sws_freeContext(this->m_scaleContext);
}

AkCaps VideoStream::caps() const
{
    AkVideoCaps::PixelFormat format =
            outputFormats->value(this->codecContext()->pix_fmt,
                                 AkVideoCaps::Format_bgra);

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = format;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = this->codecContext()->width;
    caps.height() = this->codecContext()->height;
    caps.fps() = this->fps();

    return caps.toCaps();
}

void VideoStream::processPacket(AVPacket *packet)
{
    if (!this->isValid())
        return;

    AVFrame *iFrame = av_frame_alloc();
    int gotFrame;

    avcodec_decode_video2(this->codecContext(),
                          iFrame,
                          &gotFrame,
                          packet);

    if (!gotFrame)
        return;

    this->dataEnqueue(iFrame);
}

void VideoStream::processData(AVFrame *frame)
{
    forever {
        qreal pts = av_frame_get_best_effort_timestamp(frame)
                    * this->timeBase().value();
        qreal diff = pts - this->globalClock()->clock();
        qreal delay = pts - this->m_lastPts;

        // skip or repeat frame. We take into account the
        // delay to compute the threshold. I still don't know
        // if it is the best guess
        double syncThreshold = qBound(AV_SYNC_THRESHOLD_MIN,
                                      delay,
                                      AV_SYNC_THRESHOLD_MAX);

        if (!qIsNaN(diff)
            && qAbs(diff) < AV_NOSYNC_THRESHOLD
            && delay < AV_SYNC_FRAMEDUP_THRESHOLD) {
            // video is backward the external clock.
            if (diff <= -syncThreshold) {
                this->m_lastPts = pts;

                break;
            } else if (diff > syncThreshold) {
                // video is ahead the external clock.
                QThread::usleep(1e6 * (diff - syncThreshold));

                continue;
            }
        } else
            this->globalClock()->setClock(pts);

        this->m_clockDiff = diff;
        AkPacket oPacket = this->convert(frame);
        emit this->oStream(oPacket);
        emit this->frameSent();

        this->m_lastPts = pts;

        break;
    }
}

AkFrac VideoStream::fps() const
{
    AkFrac fps;

    if (this->stream()->avg_frame_rate.num
        && this->stream()->avg_frame_rate.den)
        fps = AkFrac(this->stream()->avg_frame_rate.num,
                     this->stream()->avg_frame_rate.den);
    else
        fps = AkFrac(this->stream()->r_frame_rate.num,
                     this->stream()->r_frame_rate.den);

    return fps;
}

AkPacket VideoStream::convert(AVFrame *iFrame)
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

    AkVideoPacket packet;
    packet.caps().isValid() = true;
    packet.caps().format() = outputFormats->value(oFormat);
    packet.caps().bpp() = AkVideoCaps::bitsPerPixel(packet.caps().format());
    packet.caps().width() = iFrame->width;
    packet.caps().height() = iFrame->height;
    packet.caps().fps() = this->fps();

    int frameSize = avpicture_get_size(oFormat,
                                       iFrame->width,
                                       iFrame->height);

    QByteArray oBuffer(frameSize, Qt::Uninitialized);

    avpicture_layout(oPicture,
                     oFormat,
                     iFrame->width,
                     iFrame->height,
                     (uint8_t *) oBuffer.data(),
                     frameSize);

    packet.buffer() = oBuffer;
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
