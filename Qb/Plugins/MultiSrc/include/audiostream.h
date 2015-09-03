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

#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

extern "C"
{
    #include <libswresample/swresample.h>
}

#include "abstractstream.h"
#include "framebuffer.h"

class AudioStream: public AbstractStream
{
    Q_OBJECT

    public:
        explicit AudioStream(const AVFormatContext *formatContext=NULL,
                             uint index=-1, qint64 id=-1, bool noModify=false,
                             QObject *parent=NULL);
        ~AudioStream();

        Q_INVOKABLE QbCaps caps() const;

    protected:
        void processPacket(AVPacket *packet);

    private:
        bool m_fst;
        qint64 m_pts;
        qint64 m_duration;
        SwrContext *m_resampleContext;
        FrameBuffer m_frameBuffer;
        bool m_run;
        QThreadPool m_threadPool;

        QbPacket convert(AVFrame *iFrame);
};

#endif // AUDIOSTREAM_H
