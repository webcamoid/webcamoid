/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "subtitlestream.h"

SubtitleStream::SubtitleStream(QObject *parent): AbstractStream(parent)
{
}

SubtitleStream::SubtitleStream(AVFormatContext *formatContext, uint index):
    AbstractStream(formatContext, index)
{
}

QbCaps SubtitleStream::caps() const
{
    return QbCaps("application/x-subtitle");
}

QList<QbPacket> SubtitleStream::readPackets(AVPacket *packet)
{
    QList<QbPacket> packets;

    if (!this->isValid())
        return packets;

    AVSubtitle subtitle;
    int gotSubtitle;

    avcodec_decode_subtitle2(this->codecContext(),
                             &subtitle,
                             &gotSubtitle,
                             packet);

    if (gotSubtitle)
    {
        for (uint i = 0; i < subtitle.num_rects; i++)
        {
            QbCaps caps(this->caps());
            QSharedPointer<uchar> oBuffer;
            int dataLenght = 0;

            if (subtitle.rects[i]->type == SUBTITLE_BITMAP)
            {
                AVPixelFormat pixFmt;
                const char *format;

                if (subtitle.rects[i]->nb_colors == 4)
                {
                    pixFmt = AV_PIX_FMT_ARGB;
                    format = av_get_pix_fmt_name(pixFmt);
                }
                else
                    break;

                caps.setProperty("type", "bitmap");
                caps.setProperty("x", subtitle.rects[i]->x);
                caps.setProperty("y", subtitle.rects[i]->y);
                caps.setProperty("width", subtitle.rects[i]->w);
                caps.setProperty("height", subtitle.rects[i]->h);
                caps.setProperty("format", format);

                int frameSize = subtitle.rects[i]->nb_colors *
                                subtitle.rects[i]->w *
                                subtitle.rects[i]->h;

                QSharedPointer<uchar> oBuffer(new uchar[frameSize]);

                if (!oBuffer)
                    break;

                avpicture_layout((AVPicture *) &subtitle.rects[i]->pict,
                                 pixFmt,
                                 subtitle.rects[i]->w,
                                 subtitle.rects[i]->h,
                                 (uint8_t *) oBuffer.data(),
                                 frameSize);

                dataLenght = frameSize;
            }
            else if (subtitle.rects[i]->type == SUBTITLE_TEXT)
            {
                caps.setProperty("type", "text");
                int textLenght = sizeof(subtitle.rects[i]->text);

                oBuffer = QSharedPointer<uchar>(new uchar[textLenght]);

                if (!oBuffer)
                    break;

                memcpy(oBuffer.data(), subtitle.rects[i]->text, textLenght);
                dataLenght = textLenght;
            }
            else if (subtitle.rects[i]->type == SUBTITLE_ASS)
            {
                caps.setProperty("type", "ass");
                int assLenght = sizeof(subtitle.rects[i]->ass);

                oBuffer = QSharedPointer<uchar>(new uchar[assLenght]);

                if (!oBuffer)
                    break;

                memcpy(oBuffer.data(), subtitle.rects[i]->ass, assLenght);
                dataLenght = assLenght;
            }

            QbPacket oPacket(caps,
                             oBuffer,
                             dataLenght);

            oPacket.setPts(packet->pts);
            oPacket.setDuration(packet->duration);
            oPacket.setTimeBase(this->timeBase());
            oPacket.setIndex(this->index());

            packets << oPacket;
        }

        avsubtitle_free(&subtitle);
    }
    else
    {
        // Some subtitles seams to have a problem when decoding.
        QbCaps caps(this->caps());
        QSharedPointer<uchar> oBuffer;

        caps.setProperty("type", "ass");
        int assLenght = packet->size;

        oBuffer = QSharedPointer<uchar>(new uchar[assLenght]);

        if (oBuffer)
        {
            memcpy(oBuffer.data(), packet->data, assLenght);

            QbPacket oPacket(caps,
                             oBuffer,
                             assLenght);

            oPacket.setPts(packet->pts);
            oPacket.setDuration(packet->duration);
            oPacket.setTimeBase(this->timeBase());
            oPacket.setIndex(this->index());

            packets << oPacket;
        }
    }

    return packets;
}
