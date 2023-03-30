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
#include <akvideoconverter.h>
#include <akvideoformatspec.h>
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

const QVector<QSize> &h263SupportedSize()
{
    static const QVector<QSize> supportedSize {
        QSize(1408, 1152),
        QSize( 704,  576),
        QSize( 352,  288),
        QSize( 176,  144),
        QSize( 128,   96)
    };

    return supportedSize;
}

#if __ANDROID_API__ < 28
const char *AMEDIAFORMAT_KEY_SLICE_HEIGHT = "slice-height";
#endif

class VideoStreamPrivate
{
    public:
        VideoStream *self;
        AkVideoPacket m_frame;
        QMutex m_frameMutex;
        QWaitCondition m_frameReady;
        AkVideoCaps m_caps;
        AkVideoConverter m_videoConverter;

        explicit VideoStreamPrivate(VideoStream *self);
        static AkVideoCaps nearestH263Caps(const AkVideoCaps &caps);
};

VideoStream::VideoStream(AMediaMuxer *mediaMuxerformatContext,
                         uint index,
                         int streamIndex,
                         const QVariantMap &configs,
                         const QMap<QString, QVariantMap> &codecOptions,
                         MediaWriterNDKMedia *mediaWriter,
                         QObject *parent):
    AbstractStream(mediaMuxerformatContext,
                   index, streamIndex,
                   configs,
                   codecOptions,
                   mediaWriter,
                   parent)
{
    this->d = new VideoStreamPrivate(this);
    this->d->m_caps = configs["caps"].value<AkCaps>();
    auto codecName = configs["codec"].toString();
    auto defaultCodecParams = mediaWriter->defaultCodecParams(codecName);
    auto pixelFormat = this->d->m_caps.format();
    auto supportedPixelFormats =
            defaultCodecParams["supportedPixelFormats"].toList();

    if (!supportedPixelFormats.contains(int(pixelFormat))) {
        auto defaultPixelFormat =
                AkVideoCaps::PixelFormat(defaultCodecParams["defaultPixelFormat"].toInt());
        this->d->m_caps.setFormat(defaultPixelFormat);
    }

    if (codecName == "video/3gpp")
        this->d->m_caps = VideoStreamPrivate::nearestH263Caps(this->d->m_caps);

    int32_t interval =
            qRound(configs["gop"].toInt() / this->d->m_caps.fps().value());

    if (interval < 1)
        interval = 1;

    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_COLOR_FORMAT,
                          VideoStream::colorFormatFromPixelFormat(this->d->m_caps.format()));
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_WIDTH,
                          this->d->m_caps.width());
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_HEIGHT,
                          this->d->m_caps.height());
    AMediaFormat_setFloat(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_FRAME_RATE,
                          float(this->d->m_caps.fps().value()));
    auto specs = AkVideoCaps::formatSpecs(this->d->m_caps.format());
    auto &plane = specs.plane(0);
    size_t stride = this->d->m_caps.width() & 0x1?
                        plane.bitsSize() * (this->d->m_caps.width() + 1) / 8:
                        plane.bitsSize() * this->d->m_caps.width() / 8;
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_STRIDE,
                          stride);
#if __ANDROID_API__ >= 28
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_SLICE_HEIGHT,
                          this->d->m_caps.height());
#endif
    AMediaFormat_setInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_I_FRAME_INTERVAL,
                          interval);
    this->d->m_videoConverter.setOutputCaps(this->d->m_caps);
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

    this->d->m_frameMutex.lock();

    this->d->m_videoConverter.begin();
    this->d->m_frame = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    this->d->m_frameReady.wakeAll();
    this->d->m_frameMutex.unlock();
}

void VideoStream::encode(const AkPacket &packet,
                         uint8_t *buffer,
                         size_t bufferSize)
{
    Q_UNUSED(bufferSize)
    AkVideoPacket videoPacket(packet);

    int32_t stride = 0;
    AMediaFormat_getInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_STRIDE,
                          &stride);
    int32_t sliceHeight = 0;
    AMediaFormat_getInt32(this->mediaFormat(),
                          AMEDIAFORMAT_KEY_SLICE_HEIGHT,
                          &sliceHeight);

    if (sliceHeight < videoPacket.caps().height())
        sliceHeight = videoPacket.caps().height();

    auto oData = buffer;

    for (int plane = 0; plane < videoPacket.planes(); ++plane) {
        auto iLineSize = videoPacket.lineSize(plane);
        auto oLineSize = videoPacket.planes() > 1?
                             stride >> videoPacket.widthDiv(plane):
                             stride;
        auto lineSize = qMin<size_t>(iLineSize, oLineSize);
        auto heightDiv = videoPacket.heightDiv(plane);

        for (int y = 0; y < videoPacket.caps().height(); ++y) {
            int ys = y >> heightDiv;
            memcpy(oData + ys * oLineSize,
                   videoPacket.constLine(plane, y),
                   lineSize);
        }

        oData += oLineSize * (sliceHeight >> heightDiv);
    }
}

AkPacket VideoStream::avPacketDequeue(size_t bufferSize)
{
    Q_UNUSED(bufferSize)

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

AkVideoCaps VideoStreamPrivate::nearestH263Caps(const AkVideoCaps &caps)
{
    QSize nearestSize;
    auto q = std::numeric_limits<qreal>::max();

    for (auto &size: h263SupportedSize()) {
        qreal dw = size.width() - caps.width();
        qreal dh = size.height() - caps.height();
        qreal k = dw * dw + dh * dh;

        if (k < q) {
            nearestSize = size;
            q = k;

            if (k == 0.)
                break;
        }
    }

    AkVideoCaps nearestCaps(caps);
    nearestCaps.setWidth(nearestSize.width());
    nearestCaps.setHeight(nearestSize.height());

    return nearestCaps;
}

#include "moc_videostream.cpp"
