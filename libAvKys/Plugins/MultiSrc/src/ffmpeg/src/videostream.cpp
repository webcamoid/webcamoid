/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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
 * Web-Site: http://webcamoid.github.io/
 */

#include <QThread>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideopacket.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
}

#include "videostream.h"
#include "clock.h"

// no AV sync correction is done if below the minimum AV sync threshold
#define AV_SYNC_THRESHOLD_MIN 0.04

// AV sync correction is done if above the maximum AV sync threshold
#define AV_SYNC_THRESHOLD_MAX 0.1

// If a frame duration is longer than this, it will not be duplicated to compensate AV sync
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

// no AV correction is done if too big error
#define AV_NOSYNC_THRESHOLD 10.0

class VideoStreamPrivate
{
    public:
        VideoStream *self;
        SwsContext *m_scaleContext {nullptr};
        qreal m_lastPts {0.0};

        explicit VideoStreamPrivate(VideoStream *self);
        AkFrac fps() const;
        AkPacket convert(AVFrame *iFrame);
        AVFrame *copyFrame(AVFrame *frame) const;

        template<typename R, typename S>
        inline static R align(R value, S align)
        {
            return (value + (align >> 1)) & ~(align - 1);
        }
};

VideoStream::VideoStream(const AVFormatContext *formatContext,
                         uint index,
                         qint64 id,
                         Clock *globalClock,
                         bool sync,
                         bool noModify,
                         QObject *parent):
    AbstractStream(formatContext,
                   index,
                   id,
                   globalClock,
                   sync,
                   noModify,
                   parent)
{
    this->d = new VideoStreamPrivate(this);
    this->m_maxData = 3;
}

VideoStream::~VideoStream()
{
    if (this->d->m_scaleContext)
        sws_freeContext(this->d->m_scaleContext);

    delete this->d;
}

AkCaps VideoStream::caps() const
{
    return AkVideoCaps(AkVideoCaps::Format_rgb24,
                       this->codecContext()->width,
                       this->codecContext()->height,
                       this->d->fps());
}

bool VideoStream::decodeData()
{
    if (!this->isValid())
        return false;

    bool result = false;

    forever {
        auto iFrame = av_frame_alloc();
        int r = avcodec_receive_frame(this->codecContext(), iFrame);

        if (r >= 0) {
            this->dataEnqueue(this->d->copyFrame(iFrame));
            result = true;
        }

        av_frame_free(&iFrame);

        if (r < 0)
            break;
    }

    return result;
}

void VideoStream::processPacket(AVPacket *packet)
{
    if (!this->isValid())
        return;

    if (!packet) {
        this->dataEnqueue(nullptr);

        return;
    }

    avcodec_send_packet(this->codecContext(), packet);
}

void VideoStream::processData(AVFrame *frame)
{
    if (!this->sync()) {
        auto oPacket = this->d->convert(frame);
        emit this->oStream(oPacket);

        return;
    }

    forever {
        qreal pts = frame->pts * this->timeBase().value();
        qreal diff = pts - this->globalClock()->clock();
        qreal delay = pts - this->d->m_lastPts;

        // Skip or repeat frame. We take into account the
        // delay to compute the threshold. I still don't know
        // if it is the best guess.
        qreal syncThreshold = qBound(AV_SYNC_THRESHOLD_MIN,
                                     delay,
                                     AV_SYNC_THRESHOLD_MAX);

        if (!qIsNaN(diff)
            && qAbs(diff) < AV_NOSYNC_THRESHOLD
            && delay < AV_SYNC_FRAMEDUP_THRESHOLD) {
            // Video is backward the external clock.
            if (diff <= -syncThreshold) {
                // Drop frame.
                this->d->m_lastPts = pts;

                break;
            }

            if (diff > syncThreshold) {
                // Video is ahead the external clock.
                QThread::usleep(ulong(1e6 * (diff - syncThreshold)));

                continue;
            }
        } else
            this->globalClock()->setClock(pts);

        this->m_clockDiff = diff;
        auto oPacket = this->d->convert(frame);
        emit this->oStream(oPacket);
        this->d->m_lastPts = pts;

        break;
    }
}

VideoStreamPrivate::VideoStreamPrivate(VideoStream *self):
    self(self)
{
}

AkFrac VideoStreamPrivate::fps() const
{
    AkFrac fps;

    if (self->stream()->avg_frame_rate.num
        && self->stream()->avg_frame_rate.den)
        fps = AkFrac(self->stream()->avg_frame_rate.num,
                     self->stream()->avg_frame_rate.den);
    else
        fps = AkFrac(self->stream()->r_frame_rate.num,
                     self->stream()->r_frame_rate.den);

    return fps;
}

AkPacket VideoStreamPrivate::convert(AVFrame *iFrame)
{
    static const AVPixelFormat outPixFormat = AV_PIX_FMT_RGB24;

    // Initialize rescaling context.
    this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                iFrame->width,
                                                iFrame->height,
                                                AVPixelFormat(iFrame->format),
                                                iFrame->width,
                                                iFrame->height,
                                                outPixFormat,
                                                SWS_FAST_BILINEAR,
                                                nullptr,
                                                nullptr,
                                                nullptr);

    if (!this->m_scaleContext)
        return {};

    // Create oPicture
    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));

    if (av_image_alloc(oFrame.data,
                       oFrame.linesize,
                       iFrame->width,
                       iFrame->height,
                       outPixFormat,
                       1) < 1)
        return {};

    // Convert picture format
    sws_scale(this->m_scaleContext,
              iFrame->data,
              iFrame->linesize,
              0,
              iFrame->height,
              oFrame.data,
              oFrame.linesize);

    // Create packet
    auto nPlanes = av_pix_fmt_count_planes(AVPixelFormat(iFrame->format));
    AkVideoCaps caps(AkVideoCaps::Format_rgb24,
                     iFrame->width,
                     iFrame->height,
                     this->fps());
    AkVideoPacket oPacket(caps);

    for (int plane = 0; plane < nPlanes; ++plane) {
        auto planeData = oFrame.data[plane];
        auto oLineSize = oFrame.linesize[plane];
        auto lineSize = qMin<size_t>(oPacket.lineSize(plane), oLineSize);
        auto heightDiv = oPacket.heightDiv(plane);

        for (int y = 0; y < iFrame->height; ++y) {
            auto ys = y >> heightDiv;
            memcpy(oPacket.line(plane, y),
                   planeData + ys * oLineSize,
                   lineSize);
        }
    }

    oPacket.setId(self->id());
    oPacket.setPts(iFrame->pts);
    oPacket.setTimeBase(self->timeBase());
    oPacket.setIndex(int(self->index()));
    av_freep(&oFrame.data[0]);

    return oPacket;
}

AVFrame *VideoStreamPrivate::copyFrame(AVFrame *frame) const
{
    auto oFrame = av_frame_alloc();
    oFrame->width = frame->width;
    oFrame->height = frame->height;
    oFrame->format = frame->format;
    oFrame->pts = frame->best_effort_timestamp;

    av_image_alloc(oFrame->data,
                   oFrame->linesize,
                   oFrame->width,
                   oFrame->height,
                   AVPixelFormat(oFrame->format),
                   1);
    av_image_copy(oFrame->data,
                  oFrame->linesize,
                  const_cast<const uint8_t **>(frame->data),
                  frame->linesize,
                  AVPixelFormat(oFrame->format),
                  oFrame->width,
                  oFrame->height);

    return oFrame;
}

#include "moc_videostream.cpp"
