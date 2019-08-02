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

#include <QDateTime>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include <QtMath>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>
#include <media/NdkMediaCodec.h>

#include "videostream.h"
#include "mediawriterndkmedia.h"

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
        {COLOR_FormatYUV420SemiPlanar         , AkVideoCaps::Format_nv12       },
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
        {COLOR_FormatYUV420Flexible           , AkVideoCaps::Format_yuv420p    },
        {COLOR_FormatYUV422Flexible           , AkVideoCaps::Format_yuv422p    },
        {COLOR_FormatYUV444Flexible           , AkVideoCaps::Format_yuv444p    },
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
        AkVideoPacket m_frame;
        QMutex m_frameMutex;
        QWaitCondition m_frameReady;

        explicit VideoStreamPrivate(VideoStream *self);
        AkPacket readPacket(size_t bufferIndex,
                            const AMediaCodecBufferInfo &info);
};

VideoStream::VideoStream(AMediaMuxer *mediaMuxerformatContext,
                         uint index,
                         int streamIndex,
                         const QVariantMap &configs,
                         MediaWriterNDKMedia *mediaWriter,
                         QObject *parent):
    AbstractStream(mediaMuxerformatContext,
                   index, streamIndex,
                   configs,
                   mediaWriter,
                   parent)
{
    this->d = new VideoStreamPrivate(this);
    auto codecName = configs["codec"].toString();
    auto defaultCodecParams = mediaWriter->defaultCodecParams(codecName);
    AkVideoCaps videoCaps(this->caps());
    auto pixelFormat = AkVideoCaps::pixelFormatToString(videoCaps.format());
    auto supportedPixelFormats =
            defaultCodecParams["supportedPixelFormats"].toStringList();

    if (!supportedPixelFormats.isEmpty()
        && !supportedPixelFormats.contains(pixelFormat)) {
        auto defaultPixelFormat = defaultCodecParams["defaultPixelFormat"].toString();
        videoCaps.setFormat(AkVideoCaps::pixelFormatFromString(defaultPixelFormat));
    }

    int32_t interval =
            qRound(configs["gop"].toInt() / videoCaps.fps().value());

    if (interval < 1)
        interval = 1;

    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_COLOR_FORMAT,
                          VideoStream::colorFormatFromPixelFormat(videoCaps.format()));
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_WIDTH,
                          videoCaps.width());
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_HEIGHT,
                          videoCaps.height());
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_FRAME_RATE,
                          qRound(videoCaps.fps().value()));
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_I_FRAME_INTERVAL,
                          interval);
}

VideoStream::~VideoStream()
{
    this->uninit();
    delete this->d;
}

int32_t VideoStream::colorFormatFromPixelFormat(AkVideoCaps::PixelFormat format)
{
    return imageFormatToPixelFormat().key(format);
}

void VideoStream::convertPacket(const AkPacket &packet)
{
    if (!packet)
        return;

    AkVideoPacket videoPacket(packet);
    AkVideoCaps caps = this->caps();
    videoPacket = videoPacket.scaled(caps.width(), caps.height());
    videoPacket = videoPacket.convert(caps.format());

    this->d->m_frameMutex.lock();
    this->d->m_frame = videoPacket;
    this->d->m_frameReady.wakeAll();
    this->d->m_frameMutex.unlock();
}

bool VideoStream::encodeData(bool eos)
{
    ssize_t timeOut = 5000;

    if (eos)  {
        auto bufferIndex =
                AMediaCodec_dequeueInputBuffer(this->codec(), timeOut);

        if (bufferIndex < 0)
            return false;

        AMediaCodec_queueInputBuffer(this->codec(),
                                     size_t(bufferIndex),
                                     0,
                                     0,
                                     0,
                                     AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
    } else {
        auto packet = this->avPacketDequeue();

        if (!packet)
            return false;

        auto bufferIndex =
                AMediaCodec_dequeueInputBuffer(this->codec(), timeOut);

        if (bufferIndex < 0)
            return false;

        size_t buffersize = 0;
        auto buffer = AMediaCodec_getInputBuffer(this->codec(),
                                                 size_t(bufferIndex),
                                                 &buffersize);

        if (!buffer)
            return false;

        buffersize = qMin(size_t(packet.buffer().size()), buffersize);
        memcpy(buffer, packet.buffer().constData(), buffersize);
        auto presentationTimeUs =
                qRound(1e6 * packet.pts() * packet.timeBase().value());
        AMediaCodec_queueInputBuffer(codec(),
                                     size_t(bufferIndex),
                                     0,
                                     buffersize,
                                     uint64_t(presentationTimeUs),
                                     0);
    }

    AMediaCodecBufferInfo info;
    memset(&info, 0, sizeof(AMediaCodecBufferInfo));
    auto bufferIndex =
            AMediaCodec_dequeueOutputBuffer(this->codec(), &info, timeOut);

    if (bufferIndex >= 0) {
        auto packet = this->d->readPacket(size_t(bufferIndex), info);

        AMediaCodec_releaseOutputBuffer(this->codec(),
                                        size_t(bufferIndex),
                                        info.size != 0);
        emit this->packetReady(packet);
    }

    return true;
}

AkPacket VideoStream::avPacketDequeue()
{
    this->d->m_frameMutex.lock();

    if (!this->d->m_frame)
        if (!this->d->m_frameReady.wait(&this->d->m_frameMutex,
                                        THREAD_WAIT_LIMIT)) {
            this->d->m_frameMutex.unlock();

            return nullptr;
        }

    auto frame = this->d->m_frame;
    this->d->m_frame = AkVideoPacket();
    this->d->m_frameMutex.unlock();

    return frame;
}

VideoStreamPrivate::VideoStreamPrivate(VideoStream *self):
    self(self)
{

}

AkPacket VideoStreamPrivate::readPacket(size_t bufferIndex,
                                        const AMediaCodecBufferInfo &info)
{
    size_t bufferSize = 0;
    auto data = AMediaCodec_getOutputBuffer(self->codec(),
                                            bufferIndex,
                                            &bufferSize);
    bufferSize = qMin(bufferSize, size_t(info.size));
    QByteArray oBuffer(int(bufferSize), Qt::Uninitialized);
    memcpy(oBuffer.data(), data + info.offset, bufferSize);

    AkCaps caps("binary/data");
    AkPacket packet(caps);
    packet.setBuffer(oBuffer);
    packet.setPts(info.presentationTimeUs);
    packet.setTimeBase({1, qint64(1e6)});
    packet.setIndex(int(self->index()));
    packet.setId(0);

    return packet;
}

#include "moc_videostream.cpp"
