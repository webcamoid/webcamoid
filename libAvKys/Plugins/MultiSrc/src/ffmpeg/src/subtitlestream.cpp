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

#include "subtitlestream.h"

SubtitleStream::SubtitleStream(const AVFormatContext *formatContext,
                               uint index, qint64 id, Clock *globalClock,
                               bool noModify,
                               QObject *parent):
    AbstractStream(formatContext, index, id, globalClock, noModify, parent)
{
    this->m_maxData = 16;
}

AkCaps SubtitleStream::caps() const
{
    return AkCaps("text/x-raw");
}

void SubtitleStream::processPacket(AVPacket *packet)
{
    if (!this->isValid())
        return;

    if (!packet) {
        this->dataEnqueue(reinterpret_cast<AVSubtitle *>(NULL));

        return;
    }

    AVSubtitle *subtitle = new AVSubtitle();
    int gotSubtitle;

    avcodec_decode_subtitle2(this->codecContext(),
                             subtitle,
                             &gotSubtitle,
                             packet);

    if (gotSubtitle) {
        this->dataEnqueue(subtitle);

        return;
    }

    // Some subtitles seams to have a problem when decoding.
    AkCaps caps(this->caps());
    caps.setProperty("type", "ass");

    QByteArray oBuffer(packet->size, Qt::Uninitialized);
    memcpy(oBuffer.data(), packet->data, size_t(packet->size));

    AkPacket oPacket(caps, oBuffer);

    oPacket.setPts(packet->pts);
    oPacket.setTimeBase(this->timeBase());
    oPacket.setIndex(int(this->index()));
    oPacket.setId(this->id());

    emit this->oStream(oPacket);
    delete subtitle;
}

void SubtitleStream::processData(AVSubtitle *subtitle)
{
    for (uint i = 0; i < subtitle->num_rects; i++) {
        AkCaps caps(this->caps());
        QByteArray oBuffer;

        if (subtitle->rects[i]->type == SUBTITLE_BITMAP) {
            AVPixelFormat pixFmt;
            const char *format;

            if (subtitle->rects[i]->nb_colors == 4) {
                pixFmt = AV_PIX_FMT_ARGB;
                format = av_get_pix_fmt_name(pixFmt);
            } else
                continue;

            caps.setProperty("type", "bitmap");
            caps.setProperty("x", subtitle->rects[i]->x);
            caps.setProperty("y", subtitle->rects[i]->y);
            caps.setProperty("width", subtitle->rects[i]->w);
            caps.setProperty("height", subtitle->rects[i]->h);
            caps.setProperty("format", format);

            AVFrame frame;
            memset(&frame, 0, sizeof(AVFrame));

            if (av_image_check_size(uint(subtitle->rects[i]->w),
                                    uint(subtitle->rects[i]->h),
                                    0,
                                    NULL) < 0)
                continue;

            if (av_image_fill_linesizes(frame.linesize,
                                        pixFmt,
                                        subtitle->rects[i]->h) < 0)
                continue;

            uint8_t *data[4];
            memset(data, 0, 4 * sizeof(uint8_t *));
            int frameSize = av_image_fill_pointers(data,
                                                   pixFmt,
                                                   subtitle->rects[i]->h,
                                                   NULL,
                                                   frame.linesize);


            oBuffer.resize(frameSize);

            if (av_image_fill_pointers(reinterpret_cast<uint8_t **>(frame.data),
                                       pixFmt,
                                       subtitle->rects[i]->h,
                                       reinterpret_cast<uint8_t *>(oBuffer.data()),
                                       frame.linesize) < 0) {
                continue;
            }

            av_image_copy(frame.data,
                          frame.linesize,
#ifdef HAVE_SUBTITLEDATA
                          const_cast<const uint8_t **>(subtitle->rects[i]->data),
                          subtitle->rects[i]->linesize,
#else
                          const_cast<const uint8_t **>(subtitle->rects[i]->pict.data),
                          subtitle->rects[i]->pict.linesize,
#endif
                          pixFmt,
                          subtitle->rects[i]->w,
                          subtitle->rects[i]->h);
        } else if (subtitle->rects[i]->type == SUBTITLE_TEXT) {
            caps.setProperty("type", "text");
            int textLenght = sizeof(subtitle->rects[i]->text);

            oBuffer.resize(textLenght);
            memcpy(oBuffer.data(), subtitle->rects[i]->text, size_t(textLenght));
        } else if (subtitle->rects[i]->type == SUBTITLE_ASS) {
            caps.setProperty("type", "ass");
            int assLenght = sizeof(subtitle->rects[i]->ass);

            oBuffer.resize(assLenght);
            memcpy(oBuffer.data(), subtitle->rects[i]->ass, size_t(assLenght));
        }

        AkPacket oPacket(caps, oBuffer);
        oPacket.setPts(subtitle->pts);
        oPacket.setTimeBase(this->timeBase());
        oPacket.setIndex(int(this->index()));
        oPacket.setId(this->id());

        emit this->oStream(oPacket);
    }
}
