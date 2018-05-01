/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <QDebug>
#include <QDateTime>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include <QtMath>
#include <akutils.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideopacket.h>

extern "C"
{
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
}

#include "videostream.h"
#include "mediawriterffmpeg.h"

struct XRGB
{
    quint8 x;
    quint8 r;
    quint8 g;
    quint8 b;
};

struct BGRX
{
    quint8 b;
    quint8 g;
    quint8 r;
    quint8 x;
};

class VideoStreamPrivate
{
    public:
        AVFrame *m_frame;
        SwsContext *m_scaleContext;
        QMutex m_frameMutex;
        int64_t m_lastPts;
        int64_t m_refPts;
        QWaitCondition m_frameReady;

        VideoStreamPrivate():
            m_frame(nullptr),
            m_scaleContext(nullptr),
            m_lastPts(AV_NOPTS_VALUE),
            m_refPts(AV_NOPTS_VALUE)
        {
        }

        inline QImage swapChannels(const QImage &image) const;
};

VideoStream::VideoStream(const AVFormatContext *formatContext,
                         uint index,
                         int streamIndex,
                         const QVariantMap &configs,
                         const QMap<QString, QVariantMap> &codecOptions,
                         MediaWriterFFmpeg *mediaWriter,
                         QObject *parent):
    AbstractStream(formatContext,
                   index, streamIndex,
                   configs,
                   codecOptions,
                   mediaWriter,
                   parent)
{
    this->d = new VideoStreamPrivate;
    auto codecContext = this->codecContext();
    auto codec = codecContext->codec;
    auto defaultCodecParams = mediaWriter->defaultCodecParams(codec->name);
    codecContext->bit_rate = configs["bitrate"].toInt();

    if (codecContext->bit_rate < 1)
        codecContext->bit_rate = defaultCodecParams["defaultBitRate"].toInt();

    AkVideoCaps videoCaps(configs["caps"].value<AkCaps>());

    QString pixelFormat = AkVideoCaps::pixelFormatToString(videoCaps.format());
    QStringList supportedPixelFormats = defaultCodecParams["supportedPixelFormats"].toStringList();

    if (!supportedPixelFormats.isEmpty() && !supportedPixelFormats.contains(pixelFormat)) {
        QString defaultPixelFormat = defaultCodecParams["defaultPixelFormat"].toString();
        videoCaps.format() = AkVideoCaps::pixelFormatFromString(defaultPixelFormat);
        videoCaps.bpp() = AkVideoCaps::bitsPerPixel(videoCaps.format());
    }

    QVariantList supportedFrameRates = defaultCodecParams["supportedFrameRates"].toList();

    if (!supportedFrameRates.isEmpty()) {
        AkFrac frameRate;
        qreal maxDiff = std::numeric_limits<qreal>::max();

        for (const QVariant &rate: supportedFrameRates) {
            qreal diff = qAbs(videoCaps.fps().value() - rate.value<AkFrac>().value());

            if (diff < maxDiff) {
                frameRate = rate.value<AkFrac>();

                if (qIsNull(diff))
                    break;

                maxDiff = diff;
            }
        }

        videoCaps.fps() = frameRate;
    }

    switch (codec->id) {
    case AV_CODEC_ID_H261:
        videoCaps = mediaWriter->nearestH261Caps(videoCaps);
        break;
    case AV_CODEC_ID_H263:
        videoCaps = mediaWriter->nearestH263Caps(videoCaps);
        break;
    case AV_CODEC_ID_DVVIDEO:
        videoCaps = mediaWriter->nearestDVCaps(videoCaps);
        break;
    case AV_CODEC_ID_DNXHD:
        videoCaps.setProperty("bitrate", configs["bitrate"]);
        videoCaps = mediaWriter->nearestDNxHDCaps(videoCaps);
        codecContext->bit_rate = videoCaps.property("bitrate").toInt();
        videoCaps.setProperty("bitrate", QVariant());
        break;
    case AV_CODEC_ID_ROQ:
        videoCaps.width() = int(qPow(2, qRound(qLn(videoCaps.width()) / qLn(2))));
        videoCaps.height() = int(qPow(2, qRound(qLn(videoCaps.height()) / qLn(2))));
        videoCaps.fps() = AkFrac(qRound(videoCaps.fps().value()), 1);
        break;
    case AV_CODEC_ID_RV10:
        videoCaps.width() = 16 * qRound(videoCaps.width() / 16.);
        videoCaps.height() = 16 * qRound(videoCaps.height() / 16.);
        break;
    case AV_CODEC_ID_AMV:
        videoCaps.height() = 16 * qRound(videoCaps.height() / 16.);
        break;
#ifdef HAVE_EXTRACODECFORMATS
    case AV_CODEC_ID_XFACE:
        videoCaps.width() = 48;
        videoCaps.height() = 48;
        break;
#endif
    default:
        break;
    }

    if (!strcmp(formatContext->oformat->name, "gxf"))
        videoCaps = mediaWriter->nearestGXFCaps(videoCaps);

    QString pixelFormatStr = AkVideoCaps::pixelFormatToString(videoCaps.format());
    codecContext->pix_fmt = av_get_pix_fmt(pixelFormatStr.toStdString().c_str());
    codecContext->width = videoCaps.width();
    codecContext->height = videoCaps.height();

    auto stream = this->stream();
    AkFrac timeBase = videoCaps.fps().invert();
    stream->time_base.num = int(timeBase.num());
    stream->time_base.den = int(timeBase.den());
    codecContext->time_base = stream->time_base;
    codecContext->gop_size = configs["gop"].toInt();

    if (codecContext->gop_size < 1)
        codecContext->gop_size = defaultCodecParams["defaultGOP"].toInt();
}

VideoStream::~VideoStream()
{
    this->uninit();
    this->deleteFrame(&this->d->m_frame);
    sws_freeContext(this->d->m_scaleContext);
    delete this->d;
}

QImage VideoStreamPrivate::swapChannels(const QImage &image) const
{
    QImage swapped(image.size(), image.format());

    for (int y = 0; y < image.height(); y++) {
        const XRGB *src = reinterpret_cast<const XRGB *>(image.constScanLine(y));
        BGRX *dst = reinterpret_cast<BGRX *>(swapped.scanLine(y));

        for (int x = 0; x < image.width(); x++) {
            dst[x].x = src[x].x;
            dst[x].r = src[x].r;
            dst[x].g = src[x].g;
            dst[x].b = src[x].b;
        }
    }

    return swapped;
}

void VideoStream::convertPacket(const AkPacket &packet)
{
    if (!packet)
        return;

    auto codecContext = this->codecContext();

#ifdef HAVE_FRAMEALLOC
    auto oFrame = av_frame_alloc();
#else
    auto oFrame = avcodec_alloc_frame();
#endif

    oFrame->format = codecContext->pix_fmt;
    oFrame->width = codecContext->width;
    oFrame->height = codecContext->height;
    oFrame->pts = packet.pts();

    QImage image = AkUtils::packetToImage(packet);
    image = image.convertToFormat(QImage::Format_ARGB32);
    image = this->d->swapChannels(image);
    AkVideoPacket videoPacket(AkUtils::imageToPacket(image, packet));

    QString format = AkVideoCaps::pixelFormatToString(videoPacket.caps().format());
    AVPixelFormat iFormat = av_get_pix_fmt(format.toStdString().c_str());
    int iWidth = videoPacket.caps().width();
    int iHeight = videoPacket.caps().height();

    this->d->m_scaleContext =
            sws_getCachedContext(this->d->m_scaleContext,
                                 iWidth,
                                 iHeight,
                                 iFormat,
                                 oFrame->width,
                                 oFrame->height,
                                 AVPixelFormat(oFrame->format),
                                 SWS_FAST_BILINEAR,
                                 nullptr,
                                 nullptr,
                                 nullptr);

    if (!this->d->m_scaleContext)
        return;

    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    if (av_image_check_size(uint(iWidth),
                            uint(iHeight),
                            0,
                            nullptr) < 0)
        return;

    if (av_image_fill_linesizes(iFrame.linesize,
                                iFormat,
                                iWidth) < 0)
        return;

    if (av_image_fill_pointers(reinterpret_cast<uint8_t **>(iFrame.data),
                               iFormat,
                               iHeight,
                               reinterpret_cast<uint8_t *>(videoPacket.buffer().data()),
                               iFrame.linesize) < 0) {
        return;
    }

    if (av_image_alloc(oFrame->data,
                       oFrame->linesize,
                       oFrame->width,
                       oFrame->height,
                       AVPixelFormat(oFrame->format),
                       4) < 0)
        return;

    sws_scale(this->d->m_scaleContext,
              iFrame.data,
              iFrame.linesize,
              0,
              iHeight,
              oFrame->data,
              oFrame->linesize);

    this->d->m_frameMutex.lock();
    this->deleteFrame(&this->d->m_frame);
    this->d->m_frame = oFrame;
    this->d->m_frameReady.wakeAll();
    this->d->m_frameMutex.unlock();
}

int VideoStream::encodeData(AVFrame *frame)
{
    auto formatContext = this->formatContext();

#ifdef AVFMT_RAWPICTURE
    if (!frame && formatContext->oformat->flags & AVFMT_RAWPICTURE)
        return AVERROR_EOF;
#endif

    auto codecContext = this->codecContext();

    AkFrac outTimeBase(codecContext->time_base.num,
                       codecContext->time_base.den);

    if (frame) {
        qint64 pts = qRound64(QDateTime::currentMSecsSinceEpoch()
                              / outTimeBase.value()
                              / 1000);

        if (this->d->m_refPts == AV_NOPTS_VALUE)
            this->d->m_lastPts = this->d->m_refPts = pts;
        else if (this->d->m_lastPts != pts)
            this->d->m_lastPts = pts;
        else
            return AVERROR(EAGAIN);

        frame->pts = this->d->m_lastPts - this->d->m_refPts;
    } else {
        this->d->m_lastPts++;
    }

    auto stream = this->stream();

#ifdef AVFMT_RAWPICTURE
    if (formatContext->oformat->flags & AVFMT_RAWPICTURE) {
        // Raw video case - directly store the picture in the packet
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.data = frame? frame->data[0]: nullptr;
        pkt.size = sizeof(AVPicture);
        pkt.pts = frame? frame->pts: this->d->m_lastPts;
        pkt.stream_index = this->streamIndex();

        this->rescaleTS(&pkt, codecContext->time_base, stream->time_base);
        emit this->packetReady(&pkt);

        return 0;
    }
#endif

    // encode the image
#ifdef HAVE_SENDRECV
    auto result = avcodec_send_frame(codecContext, frame);

    if (result == AVERROR_EOF || result == AVERROR(EAGAIN))
        return result;
    else if (result < 0) {
        char errorStr[1024];
        av_strerror(AVERROR(result), errorStr, 1024);
        qDebug() << "Error encoding packets: " << errorStr;

        return result;
    }

    forever {
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = nullptr; // packet data will be allocated by the encoder
        pkt.size = 0;
        result = avcodec_receive_packet(codecContext, &pkt);

        if (result < 0)
            break;

        pkt.stream_index = this->streamIndex();
        this->rescaleTS(&pkt,
                        codecContext->time_base,
                        stream->time_base);

        // Write the compressed frame to the media file.
        emit this->packetReady(&pkt);
    }

    return result;
#else
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr; // packet data will be allocated by the encoder
    pkt.size = 0;

    int gotPacket;
    int result = avcodec_encode_video2(codecContext,
                                       &pkt,
                                       frame,
                                       &gotPacket);

    if (result < 0)
        return result;

    // If size is zero, it means the image was buffered.
    if (gotPacket) {
        pkt.stream_index = this->streamIndex();
        this->rescaleTS(&pkt,
                        codecContext->time_base,
                        stream->time_base);

        // Write the compressed frame to the media file.
        emit this->packetReady(&pkt);
    }

    return 0;
#endif
}

AVFrame *VideoStream::dequeueFrame()
{
    this->d->m_frameMutex.lock();

    if (!this->d->m_frame)
        if (!this->d->m_frameReady.wait(&this->d->m_frameMutex,
                                        THREAD_WAIT_LIMIT)) {
            this->d->m_frameMutex.unlock();

            return nullptr;
        }

    auto frame = this->d->m_frame;
    this->d->m_frame = nullptr;
    this->d->m_frameMutex.unlock();

    return frame;
}

#include "moc_videostream.cpp"
