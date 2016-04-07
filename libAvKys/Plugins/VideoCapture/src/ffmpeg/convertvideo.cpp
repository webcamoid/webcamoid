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
 * Web-Site: http://webcamoid.github.io/
 */

#include <QMetaEnum>

#include "convertvideo.h"

#define THREAD_WAIT_LIMIT 500

// no AV sync correction is done if below the minimum AV sync threshold
#define AV_SYNC_THRESHOLD_MIN 0.01

// AV sync correction is done if above the maximum AV sync threshold
#define AV_SYNC_THRESHOLD_MAX 0.1

// If a frame duration is longer than this, it will not be duplicated to compensate AV sync
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

// no AV correction is done if too big error
#define AV_NOSYNC_THRESHOLD 10.0

typedef QMap<QString, AVPixelFormat> V4l2PixFmtMap;

inline V4l2PixFmtMap initV4l2PixFmtMap()
{
    V4l2PixFmtMap rawToFF;

    // RGB formats
    rawToFF["RGB1"] = AV_PIX_FMT_RGB8;
    rawToFF["R444"] = AV_PIX_FMT_RGB444LE;
    rawToFF["RGBO"] = AV_PIX_FMT_RGB555LE;
    rawToFF["RGBP"] = AV_PIX_FMT_RGB565LE;
    rawToFF["RGBQ"] = AV_PIX_FMT_RGB555BE;
    rawToFF["RGBR"] = AV_PIX_FMT_RGB565BE;
    rawToFF["BGR3"] = AV_PIX_FMT_BGR24;
    rawToFF["RGB3"] = AV_PIX_FMT_RGB24;
    rawToFF["BGR4"] = AV_PIX_FMT_BGR0;
    rawToFF["RGB4"] = AV_PIX_FMT_0RGB;

    // Grey formats
    rawToFF["GREY"] = AV_PIX_FMT_GRAY8;
    rawToFF["Y16 "] = AV_PIX_FMT_GRAY16LE;

    // Palette formats
    rawToFF["PAL8"] = AV_PIX_FMT_PAL8;

    // Luminance+Chrominance formats
    rawToFF["YVU9"] = AV_PIX_FMT_YUV410P;
    rawToFF["YV12"] = AV_PIX_FMT_YUV420P;
    rawToFF["YUYV"] = AV_PIX_FMT_YUYV422;
    rawToFF["YYUV"] = AV_PIX_FMT_YUV422P;
    rawToFF["UYVY"] = AV_PIX_FMT_UYVY422;
    rawToFF["VYUY"] = AV_PIX_FMT_YUV422P;
    rawToFF["422P"] = AV_PIX_FMT_YUV422P;
    rawToFF["411P"] = AV_PIX_FMT_YUV411P;
    rawToFF["Y41P"] = AV_PIX_FMT_YUV411P;
    rawToFF["Y444"] = AV_PIX_FMT_YUV444P;
    rawToFF["YUV9"] = AV_PIX_FMT_YUV410P;
    rawToFF["YU12"] = AV_PIX_FMT_YUV420P;

    // two planes -- one Y, one Cr + Cb interleaved
    rawToFF["NV12"] = AV_PIX_FMT_NV12;
    rawToFF["NV21"] = AV_PIX_FMT_NV21;
    rawToFF["NV16"] = AV_PIX_FMT_NV16;

    // Bayer formats
    rawToFF["BA81"] = AV_PIX_FMT_BAYER_BGGR8;
    rawToFF["GBRG"] = AV_PIX_FMT_BAYER_GBRG8;
    rawToFF["GRBG"] = AV_PIX_FMT_BAYER_GRBG8;
    rawToFF["RGGB"] = AV_PIX_FMT_BAYER_RGGB8;

    // 10bit raw bayer, expanded to 16 bits
    rawToFF["BYR2"] = AV_PIX_FMT_BAYER_BGGR16LE;

    return rawToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(V4l2PixFmtMap, rawToFF, (initV4l2PixFmtMap()))

typedef QMap<QString, AVCodecID> V4l2CodecMap;

inline V4l2CodecMap initCompressedMap()
{
    V4l2CodecMap compressedToFF;

    // compressed formats
    compressedToFF["MJPG"] = AV_CODEC_ID_MJPEG;
    compressedToFF["JPEG"] = AV_CODEC_ID_MJPEG;
    compressedToFF["dvsd"] = AV_CODEC_ID_DVVIDEO;
    //compressedToFF["MPEG"] = "";
    compressedToFF["H264"] = AV_CODEC_ID_H264;
    compressedToFF["AVC1"] = AV_CODEC_ID_H264;
    compressedToFF["M264"] = AV_CODEC_ID_H264;
    compressedToFF["H263"] = AV_CODEC_ID_H263;
    compressedToFF["MPG1"] = AV_CODEC_ID_MPEG1VIDEO;
    compressedToFF["MPG2"] = AV_CODEC_ID_MPEG2VIDEO;
    compressedToFF["MPG4"] = AV_CODEC_ID_MPEG4;
    compressedToFF["XVID"] = AV_CODEC_ID_MPEG4;
    compressedToFF["VC1G"] = AV_CODEC_ID_VC1;
    compressedToFF["VC1L"] = AV_CODEC_ID_VC1;
    compressedToFF["VP80"] = AV_CODEC_ID_VP8;

    //  Vendor-specific formats
    compressedToFF["CPIA"] = AV_CODEC_ID_CPIA;

    return compressedToFF;
}

Q_GLOBAL_STATIC_WITH_ARGS(V4l2CodecMap, compressedToFF, (initCompressedMap()))

ConvertVideo::ConvertVideo(QObject *parent):
    QObject(parent)
{
    this->m_scaleContext = NULL;
    this->m_codecOptions = NULL;
    this->m_codecContext = NULL;
    this->m_packetQueueSize = 0;
    this->m_maxPacketQueueSize = 15 * 1024 * 1024;
    this->m_maxData = 3;
    this->m_id = -1;
    this->m_lastPts = 0;
    this->m_showLog = false;
}

ConvertVideo::~ConvertVideo()
{
    this->uninit();
}

qint64 ConvertVideo::maxPacketQueueSize() const
{
    return this->m_maxPacketQueueSize;
}

bool ConvertVideo::showLog() const
{
    return this->m_showLog;
}

void ConvertVideo::packetEnqueue(const AkPacket &packet)
{
    this->m_packetMutex.lock();

    if (this->m_packetQueueSize >= this->m_maxPacketQueueSize)
        this->m_packetQueueNotFull.wait(&this->m_packetMutex);

    this->m_packets.enqueue(packet);
    this->m_packetQueueSize += packet.buffer().size();
    this->m_packetQueueNotEmpty.wakeAll();
    this->m_packetMutex.unlock();
}

void ConvertVideo::dataEnqueue(AVFrame *frame)
{
    this->m_dataMutex.lock();

    if (this->m_frames.size() >= this->m_maxData)
        this->m_dataQueueNotFull.wait(&this->m_dataMutex);

    this->m_frames.enqueue(FramePtr(frame, this->deleteFrame));
    this->m_dataQueueNotEmpty.wakeAll();
    this->m_dataMutex.unlock();
}

bool ConvertVideo::init(const AkCaps &caps)
{
    QString fourcc = caps.property("fourcc").toString();

    if (!rawToFF->contains(fourcc)
        && !compressedToFF->contains(fourcc))
        return false;

    AVCodec *codec = avcodec_find_decoder(compressedToFF->value(fourcc, AV_CODEC_ID_RAWVIDEO));

    if (!codec)
        return false;

    this->m_codecContext = avcodec_alloc_context3(codec);

    if (!this->m_codecContext)
        return false;

    if (codec->capabilities & AV_CODEC_CAP_TRUNCATED)
        this->m_codecContext->flags |= AV_CODEC_FLAG_TRUNCATED;

    if (codec->capabilities & CODEC_CAP_DR1)
        this->m_codecContext->flags |= CODEC_FLAG_EMU_EDGE;

    this->m_codecContext->pix_fmt = rawToFF->value(fourcc, AV_PIX_FMT_NONE);
    this->m_codecContext->width = caps.property("width").toInt();
    this->m_codecContext->height = caps.property("height").toInt();
    AkFrac fps = caps.property("fps").toString();
    this->m_codecContext->framerate.num = fps.num();
    this->m_codecContext->framerate.den = fps.den();
    this->m_codecContext->workaround_bugs = 1;
    this->m_codecContext->idct_algo = FF_IDCT_AUTO;
    this->m_codecContext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;

    this->m_codecOptions = NULL;
    av_dict_set_int(&this->m_codecOptions, "refcounted_frames", 1, 0);

    if (avcodec_open2(this->m_codecContext, codec, &this->m_codecOptions) < 0) {
        avcodec_close(this->m_codecContext);
        this->m_codecContext = NULL;

        return false;
    }

    this->m_lastPts = 0;
    this->m_id = Ak::id();
    this->m_packetQueueSize = 0;
    this->m_runPacketLoop = true;
    this->m_runDataLoop = true;
    this->m_globalClock.setClock(0.);
    this->m_packetLoopResult = QtConcurrent::run(&this->m_threadPool, this->packetLoop, this);
    this->m_dataLoopResult = QtConcurrent::run(&this->m_threadPool, this->dataLoop, this);

    return true;
}

void ConvertVideo::uninit()
{
    this->m_runPacketLoop = false;
    this->m_packetLoopResult.waitForFinished();
    this->m_packets.clear();

    this->m_runDataLoop = false;
    this->m_dataLoopResult.waitForFinished();
    this->m_frames.clear();

    if (this->m_scaleContext) {
        sws_freeContext(this->m_scaleContext);
        this->m_scaleContext = NULL;
    }

    if (this->m_codecOptions)
        av_dict_free(&this->m_codecOptions);

    if (this->m_codecContext) {
        avcodec_close(this->m_codecContext);
        this->m_codecContext = NULL;
    }
}

void ConvertVideo::packetLoop(ConvertVideo *stream)
{
    while (stream->m_runPacketLoop) {
        stream->m_packetMutex.lock();

        if (stream->m_packets.isEmpty())
            stream->m_packetQueueNotEmpty.wait(&stream->m_packetMutex,
                                               THREAD_WAIT_LIMIT);

        if (!stream->m_packets.isEmpty()) {
            AkPacket packet = stream->m_packets.dequeue();
            AVFrame *iFrame = av_frame_alloc();

            AVPacket videoPacket;
            av_init_packet(&videoPacket);
            videoPacket.data = (uint8_t *) packet.buffer().constData();
            videoPacket.size = packet.buffer().size();
            videoPacket.pts = packet.pts();
            int gotFrame;

            avcodec_decode_video2(stream->m_codecContext, iFrame, &gotFrame, &videoPacket);

            if (gotFrame)
                stream->dataEnqueue(iFrame);

            stream->m_packetQueueSize -= packet.buffer().size();

            if (stream->m_packetQueueSize < stream->m_maxPacketQueueSize)
                stream->m_packetQueueNotFull.wakeAll();
        }

        stream->m_packetMutex.unlock();
    }
}

void ConvertVideo::dataLoop(ConvertVideo *stream)
{
    while (stream->m_runDataLoop) {
        stream->m_dataMutex.lock();

        if (stream->m_frames.isEmpty())
            stream->m_dataQueueNotEmpty.wait(&stream->m_dataMutex,
                                             THREAD_WAIT_LIMIT);

        if (!stream->m_frames.isEmpty()) {
            FramePtr frame = stream->m_frames.dequeue();
            stream->processData(frame);

            if (stream->m_frames.size() < stream->m_maxData)
                stream->m_dataQueueNotFull.wakeAll();
        }

        stream->m_dataMutex.unlock();
    }
}

void ConvertVideo::deleteFrame(AVFrame *frame)
{
    av_frame_unref(frame);
    av_frame_free(&frame);
}

void ConvertVideo::processData(const FramePtr &frame)
{
    forever {
        AkFrac timeBase(this->m_codecContext->framerate.den,
                        this->m_codecContext->framerate.num);
        qreal pts = av_frame_get_best_effort_timestamp(frame.data())
                    * timeBase.value();
        qreal diff = pts - this->m_globalClock.clock();
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
            this->m_globalClock.setClock(pts);

        this->convert(frame);
        this->log(diff);
        this->m_lastPts = pts;

        break;
    }
}

void ConvertVideo::convert(const FramePtr &frame)
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
                                                  NULL,
                                                  NULL,
                                                  NULL);

    if (!this->m_scaleContext)
        return;

    // Create oPicture
    int frameSize = av_image_get_buffer_size(outPixFormat,
                                             frame->width,
                                             frame->height,
                                             1);

    QByteArray oBuffer(frameSize, Qt::Uninitialized);
    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));

    if (av_image_fill_arrays((uint8_t **) oFrame.data,
                             oFrame.linesize,
                             (const uint8_t *) oBuffer.constData(),
                             outPixFormat,
                             frame->width,
                             frame->height,
                             1) < 0) {
        return;
    }

    // Convert picture format
    sws_scale(this->m_scaleContext,
              frame->data,
              frame->linesize,
              0,
              frame->height,
              oFrame.data,
              oFrame.linesize);

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = AkVideoCaps::Format_bgr24;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = frame->width;
    caps.height() = frame->height;
    caps.fps() = AkFrac(this->m_codecContext->framerate.num,
                        this->m_codecContext->framerate.den);

    // Create packet
    AkVideoPacket oPacket;
    oPacket.caps() = caps;
    oPacket.buffer() = oBuffer;
    oPacket.id() = this->m_id;
    oPacket.pts() = av_frame_get_best_effort_timestamp(frame.data());
    oPacket.timeBase() = caps.fps().invert();
    oPacket.index() = 0;

    emit this->frameReady(oPacket.toPacket());
}

void ConvertVideo::log(qreal diff)
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

void ConvertVideo::setMaxPacketQueueSize(qint64 maxPacketQueueSize)
{
    if (this->m_maxPacketQueueSize == maxPacketQueueSize)
        return;

    this->m_maxPacketQueueSize = maxPacketQueueSize;
    emit this->maxPacketQueueSizeChanged(maxPacketQueueSize);
}

void ConvertVideo::setShowLog(bool showLog)
{
    if (this->m_showLog == showLog)
        return;

    this->m_showLog = showLog;
    emit this->showLogChanged(showLog);
}

void ConvertVideo::resetMaxPacketQueueSize()
{
    this->setMaxPacketQueueSize(15 * 1024 * 1024);
}

void ConvertVideo::resetShowLog()
{
    this->setShowLog(false);
}
