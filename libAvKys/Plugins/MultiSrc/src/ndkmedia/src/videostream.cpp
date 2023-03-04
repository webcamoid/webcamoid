/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#include <QMap>
#include <QThread>
#include <QtDebug>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideopacket.h>
#include <media/NdkMediaExtractor.h>

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

#define COLOR_FormatMonochrome                1
#define COLOR_Format8bitRGB332                2
#define COLOR_Format12bitRGB444               3
#define COLOR_Format16bitARGB4444             4
#define COLOR_Format16bitARGB1555             5
#define COLOR_Format16bitRGB565               6
#define COLOR_Format16bitBGR565               7
#define COLOR_Format18bitRGB666               8
#define COLOR_Format18bitARGB1665             9
#define COLOR_Format19bitARGB1666             10
#define COLOR_Format24bitRGB888               11
#define COLOR_Format24bitBGR888               12
#define COLOR_Format24bitARGB1887             13
#define COLOR_Format25bitARGB1888             14
#define COLOR_Format32bitBGRA8888             15
#define COLOR_Format32bitARGB8888             16
#define COLOR_FormatYUV411Planar              17
#define COLOR_FormatYUV411PackedPlanar        18
#define COLOR_FormatYUV420Planar              19
#define COLOR_FormatYUV420PackedPlanar        20
#define COLOR_FormatYUV420SemiPlanar          21
#define COLOR_FormatYUV422Planar              22
#define COLOR_FormatYUV422PackedPlanar        23
#define COLOR_FormatYUV422SemiPlanar          24
#define COLOR_FormatYCbYCr                    25
#define COLOR_FormatYCrYCb                    26
#define COLOR_FormatCbYCrY                    27
#define COLOR_FormatCrYCbY                    28
#define COLOR_FormatYUV444Interleaved         29
#define COLOR_FormatRawBayer8bit              30
#define COLOR_FormatRawBayer10bit             31
#define COLOR_FormatRawBayer8bitcompressed    32
#define COLOR_FormatL2                        33
#define COLOR_FormatL4                        34
#define COLOR_FormatL8                        35
#define COLOR_FormatL16                       36
#define COLOR_FormatL24                       37
#define COLOR_FormatL32                       38
#define COLOR_FormatYUV420PackedSemiPlanar    39
#define COLOR_FormatYUV422PackedSemiPlanar    40
#define COLOR_Format18BitBGR666               41
#define COLOR_Format24BitARGB6666             42
#define COLOR_Format24BitABGR6666             43
#define COLOR_TI_FormatYUV420PackedSemiPlanar 0x7f000100
#define COLOR_FormatSurface                   0x7f000789
#define COLOR_Format32bitABGR8888             0x7f00a000
#define COLOR_FormatYUV420Flexible            0x7f420888
#define COLOR_FormatYUV422Flexible            0x7f422888
#define COLOR_FormatYUV444Flexible            0x7f444888
#define COLOR_FormatRGBFlexible               0x7f36b888
#define COLOR_FormatRGBAFlexible              0x7f36a888
#define COLOR_QCOM_FormatYUV420SemiPlanar     0x7fa30c00

using ImageFormatToPixelFormatMap = QMap<int32_t, AkVideoCaps::PixelFormat>;

inline const ImageFormatToPixelFormatMap &imageFormatToPixelFormat()
{
    static const ImageFormatToPixelFormatMap imgFmtToPixFmt {
        {COLOR_Format8bitRGB332            , AkVideoCaps::Format_rgb332    },
        {COLOR_Format12bitRGB444           , AkVideoCaps::Format_rgb444le  },
        {COLOR_Format16bitARGB4444         , AkVideoCaps::Format_argb4444le},
        {COLOR_Format16bitARGB1555         , AkVideoCaps::Format_argb1555le},
        {COLOR_Format16bitRGB565           , AkVideoCaps::Format_rgb565le  },
        {COLOR_Format16bitBGR565           , AkVideoCaps::Format_bgr565le  },
        {COLOR_Format24bitRGB888           , AkVideoCaps::Format_rgb24     },
        {COLOR_Format24bitBGR888           , AkVideoCaps::Format_bgr24     },
        {COLOR_Format32bitBGRA8888         , AkVideoCaps::Format_bgra      },
        {COLOR_Format32bitARGB8888         , AkVideoCaps::Format_argb      },
        {COLOR_FormatYUV411Planar          , AkVideoCaps::Format_yuv411p   },
        {COLOR_FormatYUV411PackedPlanar    , AkVideoCaps::Format_yuv411p   },
        {COLOR_FormatYUV420Planar          , AkVideoCaps::Format_yuv420p   },
        {COLOR_FormatYUV420PackedPlanar    , AkVideoCaps::Format_yuv420p   },
        {COLOR_FormatYUV420SemiPlanar      , AkVideoCaps::Format_nv12      },
        {COLOR_FormatYUV422Planar          , AkVideoCaps::Format_yuv422p   },
        {COLOR_FormatYUV422PackedPlanar    , AkVideoCaps::Format_yuv422p   },
        {COLOR_FormatYUV422SemiPlanar      , AkVideoCaps::Format_yuv422p   },
        {COLOR_FormatYCbYCr                , AkVideoCaps::Format_yuyv422   },
        {COLOR_FormatYCrYCb                , AkVideoCaps::Format_yvyu422   },
        {COLOR_FormatCbYCrY                , AkVideoCaps::Format_uyvy422   },
        {COLOR_FormatCrYCbY                , AkVideoCaps::Format_vyuy422   },
        {COLOR_FormatYUV444Interleaved     , AkVideoCaps::Format_yuv444    },
        {COLOR_FormatL8                    , AkVideoCaps::Format_gray8     },
        {COLOR_FormatL16                   , AkVideoCaps::Format_gray16le  },
        {COLOR_FormatL32                   , AkVideoCaps::Format_gray32le  },
        {COLOR_FormatYUV420PackedSemiPlanar, AkVideoCaps::Format_yuv420p   },
        {COLOR_FormatYUV422PackedSemiPlanar, AkVideoCaps::Format_yuv422p   },
        {COLOR_Format32bitABGR8888         , AkVideoCaps::Format_abgr      },
        {COLOR_FormatYUV420Flexible        , AkVideoCaps::Format_yuv420p   },
        {COLOR_FormatYUV422Flexible        , AkVideoCaps::Format_yuv422p   },
        {COLOR_FormatYUV444Flexible        , AkVideoCaps::Format_yuv444p   },
        {COLOR_FormatRGBFlexible           , AkVideoCaps::Format_rgb24p    },
        {COLOR_FormatRGBAFlexible          , AkVideoCaps::Format_rgbap     },
    };

    return imgFmtToPixFmt;
}

#if __ANDROID_API__ < 28
#define AMEDIAFORMAT_KEY_SLICE_HEIGHT "slice-height"
#endif

#if __ANDROID_API__ < 29
#define AMEDIAFORMAT_KEY_FRAME_COUNT "frame-count"
#endif

class VideoStreamPrivate
{
    public:
        VideoStream *self;
        qreal m_lastPts {0.0};
        bool m_eos {false};

        explicit VideoStreamPrivate(VideoStream *self);
        AkPacket readPacket(size_t bufferIndex,
                            const AMediaCodecBufferInfo &info);
};

VideoStream::VideoStream(AMediaExtractor *mediaExtractor,
                         uint index,
                         qint64 id,
                         Clock *globalClock,
                         bool sync,
                         QObject *parent):
    AbstractStream(mediaExtractor,
                   index,
                   id,
                   globalClock,
                   sync,
                   parent)
{
    this->d = new VideoStreamPrivate(this);
    this->m_maxData = 3;
}

VideoStream::~VideoStream()
{
    delete this->d;
}

AkCaps VideoStream::caps() const
{
    int32_t colorFormat = 0;
    AMediaFormat_getInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_COLOR_FORMAT,
                          &colorFormat);
    int32_t width = 0;
    AMediaFormat_getInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_WIDTH,
                          &width);
    int32_t height = 0;
    AMediaFormat_getInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_HEIGHT,
                          &height);
    float frameRate = 0.0f;
    AMediaFormat_getFloat(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_FRAME_RATE,
                          &frameRate);

    if (frameRate < 1.0f) {
        int64_t duration = 0;
        AMediaFormat_getInt64(this->mediaFormat(),
                              AMEDIAFORMAT_KEY_DURATION,
                              &duration);
        int64_t frameCount = 0;
        AMediaFormat_getInt64(this->mediaFormat(),
                              AMEDIAFORMAT_KEY_FRAME_COUNT,
                              &frameCount);
        frameRate = duration > 0.0f?
                        1.0e6f * frameCount / duration:
                        0.0f;
    }

    if (frameRate < 1.0)
        frameRate = DEFAULT_FRAMERATE;

    return AkVideoCaps(imageFormatToPixelFormat().value(colorFormat),
                       width,
                       height,
                       {qRound64(1000 * frameRate), 1000});
}

bool VideoStream::eos() const
{
    return this->d->m_eos;
}

AbstractStream::EnqueueResult VideoStream::decodeData()
{
    if (!this->isValid())
        return EnqueueFailed;

    AMediaCodecBufferInfo info;
    memset(&info, 0, sizeof(AMediaCodecBufferInfo));
    ssize_t timeOut = 5000;
    auto bufferIndex =
            AMediaCodec_dequeueOutputBuffer(this->codec(), &info, timeOut);
    AkPacket packet;

    if (bufferIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
        return EnqueueAgain;
    } else if (bufferIndex >= 0) {
        packet = this->d->readPacket(size_t(bufferIndex), info);
        AMediaCodec_releaseOutputBuffer(this->codec(),
                                        size_t(bufferIndex),
                                        info.size != 0);

        if (this->m_buffersQueued > 0) {
            this->m_bufferQueueSize = this->m_bufferQueueSize *
                                      (this->m_buffersQueued - 1)
                                      / this->m_buffersQueued;
            this->m_buffersQueued--;
        }
    }

    EnqueueResult result = EnqueueFailed;

    if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
        while (this->running()) {
            result = this->dataEnqueue({});

            if (result != EnqueueAgain)
                break;
        }

        this->d->m_eos = true;
    } else if (packet) {
        while (this->running()) {
            result = this->dataEnqueue(packet);

            if (result != EnqueueAgain)
                break;
        }
    }

    return result;
}

void VideoStream::processData(const AkPacket &packet)
{
    if (!this->sync()) {
        emit this->oStream(packet);

        return;
    }

    forever {
        qreal pts = packet.pts() * packet.timeBase().value();
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
        } else {
            this->globalClock()->setClock(pts);
        }

        this->m_clockDiff = diff;
        emit this->oStream(packet);
        this->d->m_lastPts = pts;

        break;
    }
}

VideoStreamPrivate::VideoStreamPrivate(VideoStream *self):
    self(self)
{

}

AkPacket VideoStreamPrivate::readPacket(size_t bufferIndex,
                                        const AMediaCodecBufferInfo &info)
{
    auto format = AMediaCodec_getOutputFormat(self->codec());
    int32_t colorFormat = 0;
    AMediaFormat_getInt32(format,
                          AMEDIAFORMAT_KEY_COLOR_FORMAT,
                          &colorFormat);
    int32_t width = 0;
    AMediaFormat_getInt32(format,
                          AMEDIAFORMAT_KEY_WIDTH,
                          &width);
    int32_t height = 0;
    AMediaFormat_getInt32(format,
                          AMEDIAFORMAT_KEY_HEIGHT,
                          &height);
    float frameRate = 0.0f;
    AMediaFormat_getFloat(format,
                          AMEDIAFORMAT_KEY_FRAME_RATE,
                          &frameRate);

    if (frameRate < 1.0f) {
        int64_t duration = 0;
        AMediaFormat_getInt64(format,
                              AMEDIAFORMAT_KEY_DURATION,
                              &duration);
        int64_t frameCount = 0;
        AMediaFormat_getInt64(format,
                              AMEDIAFORMAT_KEY_FRAME_COUNT,
                              &frameCount);
        frameRate = duration > 0.0f?
                        1.0e6f * frameCount / duration:
                        0.0f;
    }

    if (frameRate < 1.0)
        frameRate = DEFAULT_FRAMERATE;

    int32_t stride = 0;
    AMediaFormat_getInt32(format,
                          AMEDIAFORMAT_KEY_STRIDE,
                          &stride);
    int32_t sliceHeight = 0;
    AMediaFormat_getInt32(format,
                          AMEDIAFORMAT_KEY_SLICE_HEIGHT,
                          &sliceHeight);

    if (sliceHeight < height)
        sliceHeight = height;

    AMediaFormat_delete(format);

    size_t bufferSize = 0;
    auto data = AMediaCodec_getOutputBuffer(self->codec(),
                                            bufferIndex,
                                            &bufferSize);
    AkVideoPacket packet({imageFormatToPixelFormat().value(colorFormat),
                          width,
                          height,
                          {qRound64(1000 * frameRate), 1000}});
    auto iData = data + info.offset;

    for (int plane = 0; plane < packet.planes(); ++plane) {
        auto iLineSize = packet.planes() > 1?
                             stride >> packet.widthDiv(plane):
                             stride;
        auto oLineSize = packet.lineSize(plane);
        auto lineSize = qMin<size_t>(iLineSize, oLineSize);
        auto heightDiv = packet.heightDiv(plane);

        for (int y = 0; y < packet.caps().height(); ++y) {
            int ys = y >> heightDiv;
            memcpy(packet.line(plane, y),
                   iData + ys * iLineSize,
                   lineSize);
        }

        iData += (iLineSize * sliceHeight) >> heightDiv;
    }

    packet.setPts(info.presentationTimeUs);
    packet.setTimeBase({1, qint64(1e6)});
    packet.setIndex(int(self->index()));
    packet.setId(self->id());

    return packet;
}

#include "moc_videostream.cpp"
