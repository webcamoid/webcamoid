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

#include <QMetaEnum>
#include <QtConcurrent>
#include <QQueue>
#include <QMutex>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akvideocaps.h>
#include <akpacket.h>
#include <akvideopacket.h>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/mem.h>
}

#include "convertvideoffmpeg.h"
#include "clock.h"

#define THREAD_WAIT_LIMIT 500

// no AV sync correction is done if below the minimum AV sync threshold
#define AV_SYNC_THRESHOLD_MIN 0.01

// AV sync correction is done if above the maximum AV sync threshold
#define AV_SYNC_THRESHOLD_MAX 0.1

// If a frame duration is longer than this, it will not be duplicated to compensate AV sync
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

// no AV correction is done if too big error
#define AV_NOSYNC_THRESHOLD 10.0

using ImgFmtMap = QMap<QString, AVPixelFormat>;

inline ImgFmtMap initImgFmtMap()
{
    ImgFmtMap rawToFF = {
        // RGB formats
        {"RGB332"  , AV_PIX_FMT_RGB8    },
        {"RGB444"  , AV_PIX_FMT_RGB444LE},
        {"RGB555"  , AV_PIX_FMT_RGB555LE},
        {"RGB565"  , AV_PIX_FMT_RGB565LE},
        {"RGB555BE", AV_PIX_FMT_RGB555BE},
        {"RGB565BE", AV_PIX_FMT_RGB565BE},
        {"BGR"     , AV_PIX_FMT_BGR24   },
        {"RGB"     , AV_PIX_FMT_RGB24   },
        {"BGR0"    , AV_PIX_FMT_BGR0    },
        {"BGRX"    , AV_PIX_FMT_RGB0    },
        {"RGBX"    , AV_PIX_FMT_0RGB    },
        {"ARGB"    , AV_PIX_FMT_ARGB    },
        {"RGBA"    , AV_PIX_FMT_RGBA    },

        // Grey formats
        {"Y800"  , AV_PIX_FMT_GRAY8    },
        {"GRAY8" , AV_PIX_FMT_GRAY8    },
        {"GRAY16", AV_PIX_FMT_GRAY16LE },
        {"B1W0"  , AV_PIX_FMT_MONOWHITE},
        {"B0W1"  , AV_PIX_FMT_MONOBLACK},

        // Palette formats
        {"PAL8", AV_PIX_FMT_PAL8},

        // Luminance+Chrominance formats
        {"YVU9", AV_PIX_FMT_YUV410P},
        {"YV12", AV_PIX_FMT_YUV420P},
        {"I420", AV_PIX_FMT_YUV420P},
        {"YUYV", AV_PIX_FMT_YUYV422},
        {"YYUV", AV_PIX_FMT_YUV422P},
        {"Y42B", AV_PIX_FMT_YUV422P},
        {"UYVY", AV_PIX_FMT_UYVY422},
        {"VYUY", AV_PIX_FMT_YUV422P},
        {"422P", AV_PIX_FMT_YUV422P},
        {"411P", AV_PIX_FMT_YUV411P},
        {"Y41P", AV_PIX_FMT_YUV411P},
        {"YUY2", AV_PIX_FMT_YUYV422},
        {"Y444", AV_PIX_FMT_YUV444P},
        {"444P", AV_PIX_FMT_YUV444P},
        {"YUV9", AV_PIX_FMT_YUV410P},
        {"YU12", AV_PIX_FMT_YUV420P},

        // two planes -- one Y, one Cr + Cb interleaved
        {"NV12", AV_PIX_FMT_NV12},
        {"NV21", AV_PIX_FMT_NV21},
        {"NV16", AV_PIX_FMT_NV16},

        // Bayer formats
        {"SBGGR8", AV_PIX_FMT_BAYER_BGGR8},
        {"SGBRG8", AV_PIX_FMT_BAYER_GBRG8},
        {"SGRBG8", AV_PIX_FMT_BAYER_GRBG8},
        {"SRGGB8", AV_PIX_FMT_BAYER_RGGB8},

        // 10bit raw bayer, expanded to 16 bits
        {"SBGGR16", AV_PIX_FMT_BAYER_BGGR16LE}
    };

    return rawToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(ImgFmtMap, rawToFF, (initImgFmtMap()))

using V4l2CodecMap = QMap<QString, AVCodecID>;

inline V4l2CodecMap initCompressedMap()
{
    V4l2CodecMap compressedToFF = {
        // compressed formats
        {"MJPG", AV_CODEC_ID_MJPEG     },
        {"JPEG", AV_CODEC_ID_MJPEG     },
        {"dvsd", AV_CODEC_ID_DVVIDEO   },
        {"H264", AV_CODEC_ID_H264      },
        {"AVC1", AV_CODEC_ID_H264      },
        {"M264", AV_CODEC_ID_H264      },
        {"H263", AV_CODEC_ID_H263      },
        {"MPG1", AV_CODEC_ID_MPEG1VIDEO},
        {"MPG2", AV_CODEC_ID_MPEG2VIDEO},
        {"MPG4", AV_CODEC_ID_MPEG4     },
        {"XVID", AV_CODEC_ID_MPEG4     },
        {"VC1G", AV_CODEC_ID_VC1       },
        {"VC1L", AV_CODEC_ID_VC1       },
        {"VP80", AV_CODEC_ID_VP8       },

        //  Vendor-specific formats
        {"CPIA", AV_CODEC_ID_CPIA}
    };

    return compressedToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(V4l2CodecMap, compressedToFF, (initCompressedMap()))

using FramePtr = QSharedPointer<AVFrame>;

class ConvertVideoFFmpegPrivate
{
    public:
        ConvertVideoFFmpeg *self {nullptr};
        SwsContext *m_scaleContext {nullptr};
        AVDictionary *m_codecOptions {nullptr};
        AVCodecContext *m_codecContext {nullptr};
        qint64 m_maxPacketQueueSize {15 * 1024 * 1024};
        QThreadPool m_threadPool;
        QMutex m_packetMutex;
        QMutex m_dataMutex;
        QWaitCondition m_packetQueueNotEmpty;
        QWaitCondition m_packetQueueNotFull;
        QWaitCondition m_dataQueueNotEmpty;
        QWaitCondition m_dataQueueNotFull;
        QQueue<AkPacket> m_packets;
        QQueue<FramePtr> m_frames;
        qint64 m_packetQueueSize {0};
        QFuture<void> m_packetLoopResult;
        QFuture<void> m_dataLoopResult;
        qint64 m_id {-1};
        Clock m_globalClock;
        AkFrac m_fps;
        qreal m_lastPts {0};
        int m_maxData {3};
        bool m_showLog {false};
        bool m_runPacketLoop {false};
        bool m_runDataLoop {false};

        explicit ConvertVideoFFmpegPrivate(ConvertVideoFFmpeg *self):
            self(self)
        {
        }

        static void packetLoop(ConvertVideoFFmpeg *stream);
        static void dataLoop(ConvertVideoFFmpeg *stream);
        static void deleteFrame(AVFrame *frame);
        void processData(const FramePtr &frame);
        void convert(const FramePtr &frame);
        void convert(const AVFrame *frame);
        void log(qreal diff);
        AVFrame *copyFrame(AVFrame *frame) const;
};

ConvertVideoFFmpeg::ConvertVideoFFmpeg(QObject *parent):
    ConvertVideo(parent)
{
    this->d = new ConvertVideoFFmpegPrivate(this);

#ifndef QT_DEBUG
    av_log_set_level(AV_LOG_QUIET);
#endif

    if (this->d->m_threadPool.maxThreadCount() < 2)
        this->d->m_threadPool.setMaxThreadCount(2);
}

ConvertVideoFFmpeg::~ConvertVideoFFmpeg()
{
    this->uninit();
    delete this->d;
}

qint64 ConvertVideoFFmpeg::maxPacketQueueSize() const
{
    return this->d->m_maxPacketQueueSize;
}

bool ConvertVideoFFmpeg::showLog() const
{
    return this->d->m_showLog;
}

void ConvertVideoFFmpeg::packetEnqueue(const AkPacket &packet)
{
    this->d->m_packetMutex.lock();

    if (this->d->m_packetQueueSize >= this->d->m_maxPacketQueueSize)
        this->d->m_packetQueueNotFull.wait(&this->d->m_packetMutex);

    this->d->m_packets.enqueue(packet);
    this->d->m_packetQueueSize += packet.buffer().size();
    this->d->m_packetQueueNotEmpty.wakeAll();
    this->d->m_packetMutex.unlock();
}

void ConvertVideoFFmpeg::dataEnqueue(AVFrame *frame)
{
    this->d->m_dataMutex.lock();

    if (this->d->m_frames.size() >= this->d->m_maxData)
        this->d->m_dataQueueNotFull.wait(&this->d->m_dataMutex);

    this->d->m_frames.enqueue(FramePtr(frame,
                                       ConvertVideoFFmpegPrivate::deleteFrame));
    this->d->m_dataQueueNotEmpty.wakeAll();
    this->d->m_dataMutex.unlock();
}

bool ConvertVideoFFmpeg::init(const AkCaps &caps)
{
    QString fourcc = caps.property("fourcc").toString();

    if (!rawToFF->contains(fourcc)
        && !compressedToFF->contains(fourcc))
        return false;

    AVCodec *codec = avcodec_find_decoder(compressedToFF->value(fourcc, AV_CODEC_ID_RAWVIDEO));

    if (!codec)
        return false;

    this->d->m_codecContext = avcodec_alloc_context3(codec);

    if (!this->d->m_codecContext)
        return false;

    if (codec->capabilities & AV_CODEC_CAP_TRUNCATED)
        this->d->m_codecContext->flags |= AV_CODEC_FLAG_TRUNCATED;

#ifdef CODEC_FLAG_EMU_EDGE
    if (codec->capabilities & CODEC_CAP_DR1)
        this->d->m_codecContext->flags |= CODEC_FLAG_EMU_EDGE;
#endif

    this->d->m_codecContext->pix_fmt = rawToFF->value(fourcc, AV_PIX_FMT_NONE);
    this->d->m_codecContext->width = caps.property("width").toInt();
    this->d->m_codecContext->height = caps.property("height").toInt();
    this->d->m_fps = caps.property("fps").toString();
    this->d->m_codecContext->framerate.num = int(this->d->m_fps.num());
    this->d->m_codecContext->framerate.den = int(this->d->m_fps.den());
    this->d->m_codecContext->workaround_bugs = 1;
    this->d->m_codecContext->idct_algo = FF_IDCT_AUTO;
    this->d->m_codecContext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;

    this->d->m_codecOptions = nullptr;
    av_dict_set(&this->d->m_codecOptions, "refcounted_frames", "0", 0);

    if (avcodec_open2(this->d->m_codecContext,
                      codec,
                      &this->d->m_codecOptions) < 0) {
        avcodec_free_context(&this->d->m_codecContext);

        return false;
    }

    this->d->m_packets.clear();
    this->d->m_frames.clear();
    this->d->m_lastPts = 0;
    this->d->m_id = Ak::id();
    this->d->m_packetQueueSize = 0;
    this->d->m_runPacketLoop = true;
    this->d->m_runDataLoop = true;
    this->d->m_globalClock.setClock(0.);
    this->d->m_packetLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              ConvertVideoFFmpegPrivate::packetLoop,
                              this);
    this->d->m_dataLoopResult =
            QtConcurrent::run(&this->d->m_threadPool,
                              ConvertVideoFFmpegPrivate::dataLoop,
                              this);

    return true;
}

void ConvertVideoFFmpeg::uninit()
{
    this->d->m_runPacketLoop = false;
    this->d->m_packetLoopResult.waitForFinished();

    this->d->m_runDataLoop = false;
    this->d->m_dataLoopResult.waitForFinished();

    this->d->m_packets.clear();
    this->d->m_frames.clear();

    if (this->d->m_scaleContext) {
        sws_freeContext(this->d->m_scaleContext);
        this->d->m_scaleContext = nullptr;
    }

    if (this->d->m_codecOptions)
        av_dict_free(&this->d->m_codecOptions);

    if (this->d->m_codecContext)
        avcodec_free_context(&this->d->m_codecContext);
}

void ConvertVideoFFmpegPrivate::packetLoop(ConvertVideoFFmpeg *stream)
{
    while (stream->d->m_runPacketLoop) {
        stream->d->m_packetMutex.lock();

        if (stream->d->m_packets.isEmpty())
            stream->d->m_packetQueueNotEmpty.wait(&stream->d->m_packetMutex,
                                                  THREAD_WAIT_LIMIT);

        if (!stream->d->m_packets.isEmpty()) {
            AkPacket packet = stream->d->m_packets.dequeue();

            auto videoPacket = av_packet_alloc();
            videoPacket->data = reinterpret_cast<uint8_t *>(packet.buffer().data());
            videoPacket->size = packet.buffer().size();
            videoPacket->pts = packet.pts();

            if (avcodec_send_packet(stream->d->m_codecContext, videoPacket) >= 0)
                forever {
                    auto iFrame = av_frame_alloc();
                    int r = avcodec_receive_frame(stream->d->m_codecContext, iFrame);

                    if (r >= 0) {
                        iFrame->pts = iFrame->best_effort_timestamp;
                        stream->dataEnqueue(stream->d->copyFrame(iFrame));
                    }

                    av_frame_free(&iFrame);

                    if (r < 0)
                        break;
                }

            av_packet_free(&videoPacket);
            stream->d->m_packetQueueSize -= packet.buffer().size();

            if (stream->d->m_packetQueueSize < stream->d->m_maxPacketQueueSize)
                stream->d->m_packetQueueNotFull.wakeAll();
        }

        stream->d->m_packetMutex.unlock();
    }
}

void ConvertVideoFFmpegPrivate::dataLoop(ConvertVideoFFmpeg *stream)
{
    while (stream->d->m_runDataLoop) {
        stream->d->m_dataMutex.lock();

        if (stream->d->m_frames.isEmpty())
            stream->d->m_dataQueueNotEmpty.wait(&stream->d->m_dataMutex,
                                                THREAD_WAIT_LIMIT);

        if (!stream->d->m_frames.isEmpty()) {
            FramePtr frame = stream->d->m_frames.dequeue();
            stream->d->processData(frame);

            if (stream->d->m_frames.size() < stream->d->m_maxData)
                stream->d->m_dataQueueNotFull.wakeAll();
        }

        stream->d->m_dataMutex.unlock();
    }
}

void ConvertVideoFFmpegPrivate::deleteFrame(AVFrame *frame)
{
    av_freep(&frame->data[0]);
    frame->data[0] = nullptr;
    av_frame_unref(frame);
    av_frame_free(&frame);
}

void ConvertVideoFFmpegPrivate::processData(const FramePtr &frame)
{
    forever {
        AkFrac timeBase = this->m_fps.invert();
        qreal pts = frame->pts * timeBase.value();
        qreal diff = pts - this->m_globalClock.clock();
        qreal delay = pts - this->m_lastPts;

        // skip or repeat frame. We take into account the
        // delay to compute the threshold. I still don't know
        // if it is the best guess
        qreal syncThreshold = qBound(AV_SYNC_THRESHOLD_MIN,
                                     delay,
                                     AV_SYNC_THRESHOLD_MAX);

        if (!qIsNaN(diff)
            && qAbs(diff) < AV_NOSYNC_THRESHOLD
            && delay < AV_SYNC_FRAMEDUP_THRESHOLD) {
            // video is backward the external clock.
            if (diff <= -syncThreshold) {
                this->m_lastPts = pts;

                break;
            }

            if (diff > syncThreshold) {
                // video is ahead the external clock.
                QThread::usleep(ulong(1e6 * (diff - syncThreshold)));

                continue;
            }
        } else {
            this->m_globalClock.setClock(pts);
        }

        this->convert(frame);
        this->log(diff);
        this->m_lastPts = pts;

        break;
    }
}

void ConvertVideoFFmpegPrivate::convert(const FramePtr &frame)
{
    this->convert(frame.data());
}

void ConvertVideoFFmpegPrivate::convert(const AVFrame *frame)
{
    AVPixelFormat outPixFormat = AV_PIX_FMT_RGB24;

    // Initialize rescaling context.
    this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                frame->width,
                                                frame->height,
                                                AVPixelFormat(frame->format),
                                                frame->width,
                                                frame->height,
                                                outPixFormat,
                                                SWS_FAST_BILINEAR,
                                                nullptr,
                                                nullptr,
                                                nullptr);

    if (!this->m_scaleContext)
        return;

    // Create oPicture
    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));

    int frameSize = av_image_get_buffer_size(outPixFormat,
                                             frame->width,
                                             frame->height,
                                             1);

    if (frameSize < 1)
        return;

    QByteArray oBuffer(frameSize, Qt::Uninitialized);

    if (av_image_alloc(oFrame.data,
                   oFrame.linesize,
                   frame->width,
                   frame->height,
                   outPixFormat,
                   1) < 1)
        return;

    // Convert picture format
    sws_scale(this->m_scaleContext,
              frame->data,
              frame->linesize,
              0,
              frame->height,
              oFrame.data,
              oFrame.linesize);
    memcpy(oBuffer.data(), oFrame.data[0], frameSize);
    av_freep(&oFrame.data[0]);

    // Create packet
    AkVideoPacket oPacket;
    oPacket.caps() = {AkVideoCaps::Format_rgb24,
                     frame->width,
                     frame->height,
                     this->m_fps};
    oPacket.buffer() = oBuffer;
    oPacket.id() = this->m_id;
    oPacket.pts() = frame->pts;
    oPacket.timeBase() = this->m_fps.invert();
    oPacket.index() = 0;

    emit self->frameReady(oPacket);
}

void ConvertVideoFFmpegPrivate::log(qreal diff)
{
    if (!this->m_showLog)
        return;

    QString logFmt("%1 %2: %3 vq=%5KB");

    QString log = logFmt.arg(this->m_globalClock.clock(), 7, 'f', 2)
                        .arg("M-V")
                        .arg(-diff, 7, 'f', 3)
                        .arg(this->m_packetQueueSize / 1024, 5);

    qDebug() << log.toStdString().c_str();
}

AVFrame *ConvertVideoFFmpegPrivate::copyFrame(AVFrame *frame) const
{
    auto oFrame = av_frame_alloc();
    oFrame->width = frame->width;
    oFrame->height = frame->height;
    oFrame->format = frame->format;
    oFrame->pts = frame->pts;

    av_image_alloc(oFrame->data,
                   oFrame->linesize,
                   oFrame->width,
                   oFrame->height,
                   AVPixelFormat(oFrame->format),
                   1);
    av_frame_copy(oFrame, frame);
    av_frame_copy_props(oFrame, frame);

    return oFrame;
}

void ConvertVideoFFmpeg::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->d->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->d->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void ConvertVideoFFmpeg::setShowLog(bool showLog)
{
    if (this->d->m_showLog == showLog)
        return;

    this->d->m_showLog = showLog;
    emit this->showLogChanged(showLog);
}

void ConvertVideoFFmpeg::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void ConvertVideoFFmpeg::resetShowLog()
{
    this->setShowLog(false);
}

#include "moc_convertvideoffmpeg.cpp"
