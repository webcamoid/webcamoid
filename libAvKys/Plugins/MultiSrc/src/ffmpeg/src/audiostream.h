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

#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

extern "C"
{
    #include <libavcodec/avcodec.h>
}

#include "abstractstream.h"

class AudioStream: public AbstractStream
{
    Q_OBJECT

    public:
        explicit AudioStream(const AVFormatContext *formatContext=NULL,
                             uint index=-1, qint64 id=-1,
                             Clock *globalClock=NULL,
                             bool noModify=false,
                             QObject *parent=NULL);
        ~AudioStream();

        Q_INVOKABLE AkCaps caps() const;

    protected:
        void processPacket(AVPacket *packet);
        void processData(AVFrame *frame);

    private:
        qint64 m_pts;
        AkElementPtr m_audioConvert;

        qreal audioDiffCum; // used for AV difference average computation
        qreal audioDiffAvgCoef;
        int audioDiffAvgCount;

        bool compensate(AVFrame *oFrame, AVFrame *iFrame, int wantedSamples);
        AkPacket frameToPacket(AVFrame *iFrame);
        AkPacket convert(AVFrame *iFrame);
};

#endif // AUDIOSTREAM_H
