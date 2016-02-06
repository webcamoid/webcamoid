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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
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
    memcpy(oBuffer.data(), packet->data, packet->size);

    AkPacket oPacket(caps, oBuffer);

    oPacket.setPts(packet->pts);
    oPacket.setTimeBase(this->timeBase());
    oPacket.setIndex(this->index());
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
                break;

            caps.setProperty("type", "bitmap");
            caps.setProperty("x", subtitle->rects[i]->x);
            caps.setProperty("y", subtitle->rects[i]->y);
            caps.setProperty("width", subtitle->rects[i]->w);
            caps.setProperty("height", subtitle->rects[i]->h);
            caps.setProperty("format", format);

            int frameSize = subtitle->rects[i]->nb_colors *
                            subtitle->rects[i]->w *
                            subtitle->rects[i]->h;

            oBuffer.resize(frameSize);

            av_image_copy_to_buffer((uint8_t *) oBuffer.data(),
                                    frameSize,
                                    subtitle->rects[i]->pict.data,
                                    subtitle->rects[i]->pict.linesize,
                                    pixFmt,
                                    subtitle->rects[i]->w,
                                    subtitle->rects[i]->h,
                                    1);
        } else if (subtitle->rects[i]->type == SUBTITLE_TEXT) {
            caps.setProperty("type", "text");
            int textLenght = sizeof(subtitle->rects[i]->text);

            oBuffer.resize(textLenght);
            memcpy(oBuffer.data(), subtitle->rects[i]->text, textLenght);
        } else if (subtitle->rects[i]->type == SUBTITLE_ASS) {
            caps.setProperty("type", "ass");
            int assLenght = sizeof(subtitle->rects[i]->ass);

            oBuffer.resize(assLenght);
            memcpy(oBuffer.data(), subtitle->rects[i]->ass, assLenght);
        }

        AkPacket oPacket(caps, oBuffer);
        oPacket.setPts(subtitle->pts);
        oPacket.setTimeBase(this->timeBase());
        oPacket.setIndex(this->index());
        oPacket.setId(this->id());

        emit this->oStream(oPacket);
    }
}
