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
    this->m_codecContext = NULL;
}

ConvertVideo::~ConvertVideo()
{
    if (this->m_scaleContext)
        sws_freeContext(this->m_scaleContext);

    if (this->m_codecContext)
        avcodec_close(this->m_codecContext);
}

AkPacket ConvertVideo::convert(const AkPacket &packet)
{
    // Create iPicture.
    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));
    QString fourcc = packet.caps().property("fourcc").toString();
    iFrame.width = packet.caps().property("width").toInt();
    iFrame.height = packet.caps().property("height").toInt();

    if (rawToFF->contains(fourcc)) {
        iFrame.format = rawToFF->value(fourcc);

        if (av_image_fill_arrays((uint8_t **) iFrame.data,
                                 iFrame.linesize,
                                 (const uint8_t *) packet.buffer().constData(),
                                 AVPixelFormat(iFrame.format),
                                 iFrame.width,
                                 iFrame.height,
                                 1) < 0)
            return AkPacket();
    } else if (compressedToFF->contains(fourcc)) {
        if (!this->m_codecContext)
            return AkPacket();

        AVPacket videoPacket;
        av_init_packet(&videoPacket);
        videoPacket.data = (uint8_t *) packet.buffer().constData();
        videoPacket.size = packet.buffer().size();
        int gotFrame;

        avcodec_decode_video2(this->m_codecContext, &iFrame, &gotFrame, &videoPacket);

        if (!gotFrame)
            return AkPacket();
    } else
        return AkPacket();

    // Initialize rescaling context.
    this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                iFrame.width,
                                                iFrame.height,
                                                AVPixelFormat(iFrame.format),
                                                iFrame.width,
                                                iFrame.height,
                                                AV_PIX_FMT_BGRA,
                                                SWS_FAST_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);

    if (!this->m_scaleContext)
        return AkPacket();

    // Create oPicture
    int frameSize = av_image_get_buffer_size(AV_PIX_FMT_BGRA,
                                             iFrame.width,
                                             iFrame.height,
                                             1);

    QByteArray oBuffer(frameSize, Qt::Uninitialized);
    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));

    if (av_image_fill_arrays((uint8_t **) oFrame.data,
                             oFrame.linesize,
                             (const uint8_t *) oBuffer.constData(),
                             AV_PIX_FMT_BGRA,
                             iFrame.width,
                             iFrame.height,
                             1) < 0)
        return AkPacket();

    // Convert picture format
    sws_scale(this->m_scaleContext,
              iFrame.data,
              iFrame.linesize,
              0,
              iFrame.height,
              oFrame.data,
              oFrame.linesize);

    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = AkVideoCaps::Format_bgra;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = iFrame.width;
    caps.height() = iFrame.height;
    caps.fps() = packet.caps().property("fps").toString();

    // Create packet
    AkVideoPacket oPacket(packet);
    oPacket.caps() = caps;
    oPacket.buffer() = oBuffer;

    return oPacket.toPacket();
}

bool ConvertVideo::init(const AkCaps &caps)
{
    if (this->m_caps == caps)
        return true;

    QString fourcc = caps.property("fourcc").toString();

    if (rawToFF->contains(fourcc)) {
        this->m_caps = caps;

        return true;
    }

    if (!compressedToFF->contains(fourcc))
        return false;

    if (this->m_codecContext) {
        avcodec_close(this->m_codecContext);
        this->m_codecContext = NULL;
    }

    AVCodec *codec = avcodec_find_decoder(compressedToFF->value(fourcc));

    if (!codec)
        return false;

    this->m_codecContext = avcodec_alloc_context3(codec);

    if (!this->m_codecContext)
        return false;

    if (codec->capabilities & AV_CODEC_CAP_TRUNCATED)
        this->m_codecContext->flags |= AV_CODEC_FLAG_TRUNCATED;

    this->m_codecContext->width = caps.property("width").toInt();
    this->m_codecContext->height = caps.property("height").toInt();

    if (avcodec_open2(this->m_codecContext, codec, NULL) < 0) {
        avcodec_close(this->m_codecContext);
        this->m_codecContext = NULL;

        return false;
    }

    this->m_caps = caps;

    return true;
}
