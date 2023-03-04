/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#include <akaudiocaps.h>

#include "abstractstream.h"

class AudioStreamPrivate;

class AudioStream: public AbstractStream
{
    Q_OBJECT

    public:
        AudioStream(AMediaExtractor *mediaExtractor=nullptr,
                    uint index=0,
                    qint64 id=-1,
                    Clock *globalClock=nullptr,
                    bool sync=true,
                    QObject *parent=nullptr);
        ~AudioStream();

        Q_INVOKABLE AkCaps caps() const override;
        Q_INVOKABLE bool eos() const override;
        Q_INVOKABLE EnqueueResult decodeData() override;
        Q_INVOKABLE static AkAudioCaps::SampleFormat sampleFormatFromEncoding(int32_t encoding);
        Q_INVOKABLE static AkAudioCaps::ChannelLayout layoutFromChannelMask(int32_t channelMask);

    protected:
        void processData(const AkPacket &packet) override;

    private:
        AudioStreamPrivate *d;
};

#endif // AUDIOSTREAM_H
