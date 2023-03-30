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

#include <QColor>
#include <QRect>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <aksubtitlecaps.h>
#include <aksubtitlepacket.h>

#include "subtitlestream.h"

SubtitleStream::SubtitleStream(const AVFormatContext *formatContext,
                               uint index,
                               qint64 id,
                               Clock *globalClock,
                               bool sync,
                               bool noModify,
                               QObject *parent):
    AbstractStream(formatContext,
                   index,
                   id,
                   globalClock,
                   sync,
                   noModify,
                   parent)
{
    this->m_maxData = 16;
}

AkCaps SubtitleStream::caps() const
{
    return AkSubtitleCaps();
}

bool SubtitleStream::decodeData()
{
    if (!this->isValid())
        return false;

    return false;
}

void SubtitleStream::processPacket(AVPacket *packet)
{
    if (!this->isValid())
        return;

    if (!packet) {
        this->subtitleEnqueue(nullptr);

        return;
    }

    auto subtitle = new AVSubtitle;
    int gotSubtitle;

    avcodec_decode_subtitle2(this->codecContext(),
                             subtitle,
                             &gotSubtitle,
                             packet);

    if (gotSubtitle) {
        this->subtitleEnqueue(subtitle);

        return;
    }

    // Some subtitles seams to have a problem when decoding.
    this->processData(subtitle);
    delete subtitle;
}

void SubtitleStream::processData(AVSubtitle *subtitle)
{
    for (uint i = 0; i < subtitle->num_rects; i++) {
        AkSubtitleCaps caps;
        QByteArray oBuffer;

        if (subtitle->rects[i]->type == SUBTITLE_BITMAP) {
            caps.setFormat(AkSubtitleCaps::SubtitleFormat_bitmap);
            caps.setRect({subtitle->rects[i]->x,
                          subtitle->rects[i]->y,
                          subtitle->rects[i]->w,
                          subtitle->rects[i]->h});
            oBuffer.resize(subtitle->rects[i]->w
                           * subtitle->rects[i]->h
                           * sizeof(QRgb));
            auto bitmapData = subtitle->rects[i]->data[0];
            auto paletteData = subtitle->rects[i]->data[1];
            size_t iLineSize = subtitle->rects[i]->linesize[0];
            size_t oLineSize = sizeof(QRgb)
                               * subtitle->rects[i]->w;

            for (int y = 0; y < subtitle->rects[i]->h; y++) {
                auto src_line = reinterpret_cast<const quint8 *>(bitmapData
                                                                 + y * iLineSize);
                auto dst_line = reinterpret_cast<QRgb *>(oBuffer.data()
                                                         + y * oLineSize);

                for (int x = 0; x < subtitle->rects[i]->w; x++)
                    dst_line[x] = paletteData[src_line[x]];
            }
        } else if (subtitle->rects[i]->type == SUBTITLE_TEXT) {
            caps.setFormat(AkSubtitleCaps::SubtitleFormat_text);
            int textLenght = sizeof(subtitle->rects[i]->text);
            oBuffer.resize(textLenght);
            memcpy(oBuffer.data(), subtitle->rects[i]->text, size_t(textLenght));
        } else if (subtitle->rects[i]->type == SUBTITLE_ASS) {
            caps.setFormat(AkSubtitleCaps::SubtitleFormat_ass);
            int assLenght = sizeof(subtitle->rects[i]->ass);
            oBuffer.resize(assLenght);
            memcpy(oBuffer.data(), subtitle->rects[i]->ass, size_t(assLenght));
        }

        AkSubtitlePacket oPacket(caps, oBuffer.size());
        memcpy(oPacket.data(), oBuffer.constData(), oBuffer.size());
        oPacket.setPts(subtitle->pts);
        oPacket.setTimeBase(this->timeBase());
        oPacket.setIndex(int(this->index()));
        oPacket.setId(this->id());

        emit this->oStream(oPacket);
    }
}

#include "moc_subtitlestream.cpp"
