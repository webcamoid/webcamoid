/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

extern "C"
{
    #include <libswscale/swscale.h>
}

#include "abstractstream.h"
#include "framebuffer.h"

class VideoStream: public AbstractStream
{
    Q_OBJECT

    public:
        explicit VideoStream(const AVFormatContext *formatContext=NULL,
                             uint index=-1, qint64 id=-1, bool noModify=false,
                             QObject *parent=NULL);
        ~VideoStream();

        Q_INVOKABLE QbCaps caps() const;

    protected:
        void processPacket(AVPacket *packet);

    private:
        SwsContext *m_scaleContext;
        FrameBuffer m_frameBuffer;
        bool m_run;
        QThreadPool m_threadPool;

        QbFrac fps() const;
        QbPacket convert(AVFrame *iFrame);
};

#endif // VIDEOSTREAM_H
