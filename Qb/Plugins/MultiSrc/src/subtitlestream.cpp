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
    this->m_fst = true;
}

SubtitleStream::SubtitleStream(AVFormatContext *formatContext, uint index):
    AbstractStream(formatContext, index)
{
    this->m_fst = true;
}

QbCaps SubtitleStream::caps() const
{
    return QbCaps("application/x-subtitle");
}

QbPacket SubtitleStream::readPacket(AVPacket *packet)
{
    if (!this->isValid())
        return QbPacket();

    AVSubtitle iFrame;
    memset(&iFrame, 0, sizeof(AVSubtitle));
    iFrame.pts = AV_NOPTS_VALUE;

    int gotFrame;

    avcodec_decode_subtitle2(this->codecContext(),
                             &iFrame,
                             &gotFrame,
                             packet);

    if (!gotFrame)
        return QbPacket();

    // http://ffmpeg.org/doxygen/trunk/structAVSubtitle.html
    //
    // "application/x-subtitle,type=bitmap,stime=%2,etime=%3"
    // "application/x-subtitle,type=text,stime=%2,etime=%3"
    // "application/x-subtitle,type=ass,stime=%2,etime=%3"

    return QbPacket();
}
