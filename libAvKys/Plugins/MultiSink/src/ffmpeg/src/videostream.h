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

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include <akvideopacket.h>

extern "C"
{
    #include <libswscale/swscale.h>
}

#include "abstractstream.h"

class VideoStream: public AbstractStream
{
    Q_OBJECT

    public:
        VideoStream(const AVFormatContext *formatContext=NULL,
                    uint index=0, int streamIndex=-1,
                    const QVariantMap &configs={},
                    const QMap<QString, QVariantMap> &codecOptions={},
                    MediaWriterFFmpeg *mediaWriter=NULL,
                    QObject *parent=nullptr);
        ~VideoStream();

    private:
        AVFrame *m_frame;
        SwsContext *m_scaleContext;
        QMutex m_frameMutex;
        int64_t m_lastPts;
        int64_t m_refPts;

        QImage swapChannels(const QImage &image) const;

    protected:
        void convertPacket(const AkPacket &packet);
        PacketStatus encodeData(AVFrame *frame);
        AVFrame *dequeueFrame();
};

#endif // VIDEOSTREAM_H
