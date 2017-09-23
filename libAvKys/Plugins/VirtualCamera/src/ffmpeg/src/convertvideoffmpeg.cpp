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

#include <QMetaEnum>

#include "convertvideoffmpeg.h"

ConvertVideoFFmpeg::ConvertVideoFFmpeg(QObject *parent):
    ConvertVideo(parent)
{
    avcodec_register_all();

    this->m_scaleContext = nullptr;

#ifndef QT_DEBUG
    av_log_set_level(AV_LOG_QUIET);
#endif
}

ConvertVideoFFmpeg::~ConvertVideoFFmpeg()
{
    if (this->m_scaleContext)
        sws_freeContext(this->m_scaleContext);
}

AkPacket ConvertVideoFFmpeg::convert(const AkPacket &packet, const AkCaps &oCaps)
{
    AkVideoPacket videoPacket(packet);
    AkVideoCaps oVideoCaps(oCaps);

    // Convert input format.
    QString format = AkVideoCaps::pixelFormatToString(videoPacket.caps().format());
    AVPixelFormat iFormat = av_get_pix_fmt(format.toStdString().c_str());

    format = AkVideoCaps::pixelFormatToString(oVideoCaps.format());
    AVPixelFormat oFormat = av_get_pix_fmt(format.toStdString().c_str());

    if (oFormat == AV_PIX_FMT_NONE)
        return AkPacket();

    // Initialize rescaling context.
    this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                videoPacket.caps().width(),
                                                videoPacket.caps().height(),
                                                iFormat,
                                                oVideoCaps.width(),
                                                oVideoCaps.height(),
                                                oFormat,
                                                SWS_FAST_BILINEAR,
                                                nullptr,
                                                nullptr,
                                                nullptr);

    if (!this->m_scaleContext)
        return AkPacket();

    // Create iPicture.
    AVFrame iFrame;
    memset(&iFrame, 0, sizeof(AVFrame));

    if (av_image_check_size(uint(videoPacket.caps().width()),
                            uint(videoPacket.caps().height()),
                            0,
                            nullptr) < 0)
        return AkPacket();

    if (av_image_fill_linesizes(iFrame.linesize,
                                iFormat,
                                videoPacket.caps().width()) < 0)
        return AkPacket();

    if (av_image_fill_pointers(reinterpret_cast<uint8_t **>(iFrame.data),
                               iFormat,
                               videoPacket.caps().height(),
                               reinterpret_cast<uint8_t *>(videoPacket.buffer().data()),
                               iFrame.linesize) < 0) {
        return AkPacket();
    }

    // Create oPicture
    AVFrame oFrame;
    memset(&oFrame, 0, sizeof(AVFrame));

    if (av_image_fill_linesizes(oFrame.linesize,
                                oFormat,
                                oVideoCaps.width()) < 0)
        return AkPacket();

    uint8_t *data[4];
    memset(data, 0, 4 * sizeof(uint8_t *));
    int frameSize = av_image_fill_pointers(data,
                                           oFormat,
                                           oVideoCaps.height(),
                                           nullptr,
                                           oFrame.linesize);

    QByteArray oBuffer(frameSize, 0);

    av_image_fill_pointers(oFrame.data,
                           oFormat,
                           oVideoCaps.height(),
                           reinterpret_cast<uint8_t *>(oBuffer.data()),
                           oFrame.linesize);

    // Convert picture format
    sws_scale(this->m_scaleContext,
              iFrame.data,
              iFrame.linesize,
              0,
              videoPacket.caps().height(),
              oFrame.data,
              oFrame.linesize);

    // Create packet
    AkVideoPacket oPacket(packet);
    oPacket.caps() = oVideoCaps;
    oPacket.buffer() = oBuffer;

    return oPacket.toPacket();
}
