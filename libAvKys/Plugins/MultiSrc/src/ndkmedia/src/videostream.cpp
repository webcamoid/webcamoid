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
        {COLOR_FormatMonochrome               , AkVideoCaps::Format_monob      },
        {COLOR_Format8bitRGB332               , AkVideoCaps::Format_rgb8       },
        {COLOR_Format12bitRGB444              , AkVideoCaps::Format_rgb444le   },
        {COLOR_Format16bitARGB4444            , AkVideoCaps::Format_argb444le  },
        {COLOR_Format16bitARGB1555            , AkVideoCaps::Format_argb555le  },
        {COLOR_Format16bitRGB565              , AkVideoCaps::Format_rgb565le   },
        {COLOR_Format16bitBGR565              , AkVideoCaps::Format_bgr565le   },
        {COLOR_Format18bitRGB666              , AkVideoCaps::Format_rgb666     },
        {COLOR_Format18bitARGB1665            , AkVideoCaps::Format_argb1665   },
        {COLOR_Format19bitARGB1666            , AkVideoCaps::Format_argb1666   },
        {COLOR_Format24bitRGB888              , AkVideoCaps::Format_rgb24      },
        {COLOR_Format24bitBGR888              , AkVideoCaps::Format_bgr24      },
        {COLOR_Format24bitARGB1887            , AkVideoCaps::Format_argb1887   },
        {COLOR_Format25bitARGB1888            , AkVideoCaps::Format_bgra1888   },
        {COLOR_Format32bitBGRA8888            , AkVideoCaps::Format_bgra       },
        {COLOR_Format32bitARGB8888            , AkVideoCaps::Format_argb       },
        {COLOR_FormatYUV411Planar             , AkVideoCaps::Format_yuv411p    },
        {COLOR_FormatYUV411PackedPlanar       , AkVideoCaps::Format_yuv411p    },
        {COLOR_FormatYUV420Planar             , AkVideoCaps::Format_yuv420p    },
        {COLOR_FormatYUV420PackedPlanar       , AkVideoCaps::Format_yuv420p    },
        {COLOR_FormatYUV420SemiPlanar         , AkVideoCaps::Format_yuv420p    },
        {COLOR_FormatYUV422Planar             , AkVideoCaps::Format_yuv422p    },
        {COLOR_FormatYUV422PackedPlanar       , AkVideoCaps::Format_yuv422p    },
        {COLOR_FormatYUV422SemiPlanar         , AkVideoCaps::Format_yuv422p    },
        {COLOR_FormatYCbYCr                   , AkVideoCaps::Format_yuyv422    },
        {COLOR_FormatYCrYCb                   , AkVideoCaps::Format_yvyu422    },
        {COLOR_FormatCbYCrY                   , AkVideoCaps::Format_uyvy422    },
        {COLOR_FormatCrYCbY                   , AkVideoCaps::Format_vyuy422    },
        {COLOR_FormatYUV444Interleaved        , AkVideoCaps::Format_yuv444     },
        {COLOR_FormatRawBayer8bit             , AkVideoCaps::Format_bayer_rggb8},
//        {COLOR_FormatRawBayer10bit            , AkVideoCaps::Format_           },
//        {COLOR_FormatRawBayer8bitcompressed   , AkVideoCaps::Format_           },
        {COLOR_FormatL2                       , AkVideoCaps::Format_gray2      },
        {COLOR_FormatL4                       , AkVideoCaps::Format_gray4      },
        {COLOR_FormatL8                       , AkVideoCaps::Format_gray       },
        {COLOR_FormatL16                      , AkVideoCaps::Format_gray16le   },
        {COLOR_FormatL24                      , AkVideoCaps::Format_gray24     },
        {COLOR_FormatL32                      , AkVideoCaps::Format_gray32     },
        {COLOR_FormatYUV420PackedSemiPlanar   , AkVideoCaps::Format_yuv420p    },
        {COLOR_FormatYUV422PackedSemiPlanar   , AkVideoCaps::Format_yuv422p    },
        {COLOR_Format18BitBGR666              , AkVideoCaps::Format_bgr666     },
        {COLOR_Format24BitARGB6666            , AkVideoCaps::Format_argb6666   },
        {COLOR_Format24BitABGR6666            , AkVideoCaps::Format_abgr6666   },
//        {COLOR_TI_FormatYUV420PackedSemiPlanar, AkVideoCaps::Format_           },
//        {COLOR_FormatSurface                  , AkVideoCaps::Format_           },
        {COLOR_Format32bitABGR8888            , AkVideoCaps::Format_abgr       },
//        {COLOR_FormatYUV420Flexible           , AkVideoCaps::Format_           },
//        {COLOR_FormatYUV422Flexible           , AkVideoCaps::Format_           },
//        {COLOR_FormatYUV444Flexible           , AkVideoCaps::Format_           },
        {COLOR_FormatRGBFlexible              , AkVideoCaps::Format_rgbp       },
        {COLOR_FormatRGBAFlexible             , AkVideoCaps::Format_rgbap      },
//        {COLOR_QCOM_FormatYUV420SemiPlanar    , AkVideoCaps::Format_           },
    };

    return imgFmtToPixFmt;
}

class VideoStreamPrivate
{
    public:
        VideoStream *self;
        qreal m_lastPts {0.0};

        explicit VideoStreamPrivate(VideoStream *self);
        AkPacket readPacket(size_t bufferIndex,
                            const AMediaCodecBufferInfo &info);
};

VideoStream::VideoStream(AMediaExtractor *mediaExtractor,
                         uint index, qint64 id, Clock *globalClock,
                         QObject *parent):
    AbstractStream(mediaExtractor, index, id, globalClock, parent)
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
    int32_t width = 0;
    AMediaFormat_getInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_WIDTH,
                          &width);
    int32_t height = 0;
    AMediaFormat_getInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_HEIGHT,
                          &height);
    int32_t frameRate;
    AMediaFormat_getInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_FRAME_RATE,
                          &frameRate);

    return AkVideoCaps(AkVideoCaps::Format_rgb24,
                       width,
                       height,
                       AkFrac(frameRate, 1));
}

bool VideoStream::decodeData()
{
    if (!this->isValid())
        return false;

    AMediaCodecBufferInfo info;
    memset(&info, 0, sizeof(AMediaCodecBufferInfo));
    ssize_t timeOut = 5000;
    auto bufferIndex =
            AMediaCodec_dequeueOutputBuffer(this->codec(), &info, timeOut);

    if (bufferIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
        return true;
    else if (bufferIndex >= 0) {
        auto packet = this->d->readPacket(size_t(bufferIndex), info);

        if (packet)
            this->dataEnqueue(packet);

        AMediaCodec_releaseOutputBuffer(this->codec(),
                                        size_t(bufferIndex),
                                        info.size != 0);
    }

    if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
        this->dataEnqueue({});

        return false;
    }

    return true;
}

void VideoStream::processData(const AkPacket &packet)
{
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
        auto oPacket = AkVideoPacket(packet).convert(AkVideoCaps::Format_rgb24);
        emit this->oStream(oPacket);
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
    int32_t frameRate;
    AMediaFormat_getInt32(format,
                          AMEDIAFORMAT_KEY_FRAME_RATE,
                          &frameRate);
    AMediaFormat_delete(format);

    size_t bufferSize = 0;
    auto data = AMediaCodec_getOutputBuffer(self->codec(),
                                            bufferIndex,
                                            &bufferSize);
    bufferSize = qMin(bufferSize, size_t(info.size));
    QByteArray oBuffer(int(bufferSize), Qt::Uninitialized);
    memcpy(oBuffer.data(), data + info.offset, bufferSize);

    AkVideoPacket packet;
    packet.setCaps({imageFormatToPixelFormat().value(colorFormat),
                    width,
                    height,
                    {AkFrac(frameRate, 1)}});
    packet.setBuffer(oBuffer);
    packet.setPts(info.presentationTimeUs);
    packet.setTimeBase({1, qint64(1e6)});
    packet.setIndex(int(self->index()));
    packet.setId(self->id());

    return packet;
}

#include "moc_videostream.cpp"
